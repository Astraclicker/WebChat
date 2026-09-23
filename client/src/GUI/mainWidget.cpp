#include "mainWidget.h"
#include <QScreen>
#include <QVBoxLayout>
#include "serverWidget.h"
#include <fstream>
#include <iostream>
#include <filesystem>

mainWidget::mainWidget(
    QWidget *parent,
    const std::string &title,
    chatClient &web_api
) : QWidget(parent) {
    //mainWidget 窗口设置
    this->setWindowTitle(title.c_str());
    this->setMinimumSize(800, 600);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(12, 12, 12, 12);
    rootLayout->setSpacing(8);

    _messageArea = new messageArea;
    rootLayout->addWidget(_messageArea, 1);

    auto *sendBar = new QHBoxLayout();
    message_input = new QPlainTextEdit(this);
    message_input->setPlaceholderText("输入消息…");
    message_input->setMinimumHeight(60);
    message_input->setMaximumHeight(120);

    button_send = new QPushButton("发送", this);
    button_send->setMinimumSize(96, 36);
    button_send->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    sendBar->addWidget(message_input, 1);
    rootLayout->addWidget(button_send, 0, Qt::AlignBottom);
    rootLayout->addLayout(sendBar, 0);
    //获取屏幕长宽
    const QScreen *current_screen = QGuiApplication::screenAt(QCursor::pos());
    if (current_screen == nullptr) {
        current_screen = QGuiApplication::primaryScreen();
    }
    const QRect screen_geometry = current_screen == nullptr
                                      ? QRect(0, 0, 1920, 1080)
                                      : current_screen->availableGeometry();
    this->screenH = screen_geometry.height();
    this->screenW = screen_geometry.width();

    //创建子窗口
    _loginWidget = new loginWidget(nullptr, screenW, screenH);
    _createUserWidget = new createUserWidget(nullptr, screenW, screenH);

    //请求超时设置
    request_timeout_timer = new QTimer(this);
    request_timeout_timer->setSingleShot(true);

    //轮询检查接收队列间隔时间
    message_poll_timer = new QTimer(this);
    message_poll_timer->start(20);

    initConnect(web_api);

    //展示登录界面
    _loginWidget->show();
}

void mainWidget::initConnect(chatClient &web_api) {
    //连接登录按钮与发送登录请求
    connect(_loginWidget, &loginWidget::loginRequested, this, [&web_api, this]() {
        if (pending_request != pending_request_type::none) {
            messageBox::popup(_loginWidget, "正在等待上一个请求", messageBox::Type::Info);
            return;
        }

        nlohmann::json json_data;
        json_data["userName"] = _loginWidget->getUserName();
        json_data["password"] = _loginWidget->getPassword();
        message outgoing_message{.data = json_data.dump(), .type = message_type::login_requested};
        web_api.write(outgoing_message);
        pending_request = pending_request_type::login;
        request_timeout_timer->start(5000);
    });

    //连接发送按钮与发送消息请求
    connect(button_send, &QPushButton::clicked, this, [this, &web_api]() {
        nlohmann::json json_data;
        const std::string text = message_input->toPlainText().trimmed().toStdString();
        if (text.empty()) {
            messageBox::popup(this, "消息不能为空", messageBox::Type::Error);
            return;
        }

        // TODO 选择接收者(作为拓展功能)
        json_data["receiver"] = "root";
        json_data["text"] = text;
        _messageArea->addContent("[" + current_user + "]: " + text, messageArea::userType::currentUser);

        try {
            append_chat_history(current_user, text);
        } catch (std::exception &error) {
            std::cerr << error.what() << std::endl;
        }

        message outgoing_message{.data = json_data.dump(), .type = message_type::text};
        web_api.write(outgoing_message);
        message_input->clear();
    });

    //连接创建用户按钮与打开创建用户界面
    connect(_loginWidget, &loginWidget::createUserRequested, this, [this]() {
        _loginWidget->close();
        _createUserWidget->show();
    });

    //连接取消按与关闭窗口
    connect(_loginWidget, &loginWidget::cancelRequested, this, [this]() {
        _loginWidget->close();
    });

    //连接确认创建用户与发送创建用户请求
    connect(_createUserWidget, &createUserWidget::createUserRequested, this, [&web_api, this]() {
        if (pending_request != pending_request_type::none) {
            messageBox::popup(_createUserWidget, "正在等待上一个请求", messageBox::Type::Info);
            return;
        }

        nlohmann::json json_data;
        json_data["userName"] = _createUserWidget->getUserName();
        json_data["password"] = _createUserWidget->getPassword();
        message outgoing_message{.data = json_data.dump(), .type = message_type::create_user_requested};
        web_api.write(outgoing_message);
        pending_request = pending_request_type::createUser;
        request_timeout_timer->start(5000);
    });

    //连接取消创建用户与关闭创建用户界面
    connect(_createUserWidget, &createUserWidget::cancelRequested, this, [this]() {
        _createUserWidget->close();
        _loginWidget->show();
    });

    //连接请求超时与处理
    connect(request_timeout_timer, &QTimer::timeout, this, [this]() {
        QWidget *request_window = pending_request == pending_request_type::createUser
                                      ? static_cast<QWidget *>(_createUserWidget)
                                      : static_cast<QWidget *>(_loginWidget);
        pending_request = pending_request_type::none;
        messageBox::popup(request_window, "请求超时", messageBox::Type::Error);
    });

    //设置轮询检查接收队列
    connect(message_poll_timer, &QTimer::timeout, this, [&web_api, this]() {
        try {
            process_received_messages(web_api);
        } catch (const std::exception &error) {
            std::cerr << "process message failed: " << error.what() << std::endl;
        }
    });
}

//把一条聊天消息追加写入当前用户的本地记录文件
void mainWidget::append_chat_history(const std::string &sender, const std::string &text) const {
    //没登录就不记录，避免写到莫名其妙的地方
    if (current_user.empty()) {
        return;
    }

    //组装要存储的内容
    nlohmann::json item;
    item["sender"] = sender;
    item["text"] = text;

    const auto dir = std::filesystem::u8path("talk_history");
    const auto file = dir / std::filesystem::u8path(current_user + ".json");

    std::ifstream inChatFile(file);
    if (!inChatFile) {
        std::error_code ec;
        std::filesystem::create_directories(dir, ec);
        std::ofstream tempFile(file);
        tempFile << "[]";
        tempFile.close();
        if (ec) {
            std::cerr << "create talk_history failed: " << ec.message() << std::endl;
            return;
        }
    }

    //将目标文件写到内存中的json数组
    nlohmann::json array = nlohmann::json::array();
    try {
        inChatFile >> array;
        inChatFile.close();
    } catch (std::exception &error) {
        std::cerr << error.what() << std::endl;
    }

    //向数组中追加数据
    array.push_back(item);

    //回写数据
    std::ofstream outChatFile(file);
    outChatFile << array;
    outChatFile.close();
}

//检查接收队列并打印消息
void mainWidget::process_received_messages(chatClient &web_api) {
    nlohmann::json read_message;
    while (web_api.try_pop_message(read_message)) {
        const std::string type = read_message.value("type", std::string{});
        if (type == "text") {
            const auto data = read_message.value("data", nlohmann::json::object());
            const std::string sender = data.value("sender", std::string{});
            const std::string text = data.value("text", std::string{});

            _messageArea->addContent("[" + sender + "]: " += text, messageArea::userType::otherUser);
            //写入聊天记录到本地json
            append_chat_history(sender, text);
            continue;
        }

        const std::string data = read_message.value("data", std::string{});
        if (type == "mysqlLoginFeedBack") {
            request_timeout_timer->stop();
            pending_request = pending_request_type::none;
            const bool success = data == "success";
            messageBox::popup(
                nullptr,
                success ? "登录成功" : "登录失败",
                success ? messageBox::Type::Success : messageBox::Type::Error
            );
            if (success) {
                current_user = _loginWidget->getUserName();
                //读取本地聊天记录
                const auto dir = std::filesystem::u8path("talk_history");
                const auto file = dir / std::filesystem::u8path(current_user + ".json");
                std::ifstream chatFile(file);
                if (!chatFile) {
                    std::error_code ec;
                    std::filesystem::create_directories(dir, ec);
                    std::ofstream tempFile(file);
                    tempFile << "[]";
                    tempFile.close();
                    if (ec) {
                        std::cerr << "create talk_history failed: " << ec.message() << std::endl;
                        return;
                    }
                }

                nlohmann::json array = nlohmann::json::array();

                try {
                    chatFile >> array;
                } catch (std::exception &error) {
                    std::cerr << error.what() << std::endl;
                }

                for (const auto &item: array) {
                    auto sender = item.value("sender", std::string{});
                    if (sender == current_user) {
                        _messageArea->addContent(
                            "[" + sender + "]: " += item.value("text", std::string{}),
                            messageArea::userType::currentUser);
                    } else {
                        _messageArea->addContent(
                            "[" + sender + "]: " += item.value("text", std::string{}),
                            messageArea::userType::otherUser);
                    }
                }
                std::clog << "chatFile load success" << std::endl;
                show();
                _loginWidget->close();
            }
            continue;
        }

        if (type == "mysqlCreateUserFeedBack") {
            request_timeout_timer->stop();
            pending_request = pending_request_type::none;
            const bool success = data == "success";
            messageBox::popup(
                _createUserWidget,
                success ? "注册成功" : "注册失败：用户名已存在",
                success ? messageBox::Type::Success : messageBox::Type::Error
                //TODO 为新用户创建SQLite my
            );
            if (success) {
                _createUserWidget->close();
                _loginWidget->show();
            }
        }
    }
}
