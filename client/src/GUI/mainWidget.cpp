#include "mainWidget.h"
#include <QScreen>
#include "serverWidget.h"
#include <fstream>
#include <cstdlib>
#include <filesystem>

mainWidget::mainWidget(
    QWidget *parent,
    const std::string &title,
    chatClient &web_api
) : QWidget(parent) {
    //mainWidget 窗口设置
    this->setWindowTitle(title.c_str());
    this->setMinimumSize(800, 600);

    //聊天记录显示区（只读）：放在窗口上半部分
    chat_display = new QTextEdit(this);
    chat_display->setReadOnly(true);
    chat_display->setGeometry(10, 10, 780, 380);
    chat_display->show();

    //TODO 发送界面绘制 astraclicker
    message_input = new QPlainTextEdit(this);
    message_input->setGeometry(10, 400, 780, 120);
    button_send = new QPushButton(this);
    button_send->setText("发送");
    button_send->move(690, 530);
    button_send->setFixedSize(100, 40);
    button_send->show();

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

        // TODO 选择接收者(作为拓展功能) astraclicker
        json_data["receiver"] = "root";
        json_data["text"] = text;
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
        process_received_messages(web_api);
    });
}

//把一条聊天消息追加写入当前用户的本地记录文件
void mainWidget::append_chat_history(const std::string &sender, const std::string &text) {
    //没登录就不记录，避免写到莫名其妙的地方
    if (current_user.empty()) {
        return;
    }

    //组装要存储的内容（和网络协议一样用 JSON）
    nlohmann::json record;
    record["sender"] = sender;
    record["text"] = text;

    //拼出目标路径： $HOME/talk_history/<用户名>.jsonl
    const std::filesystem::path dir =
        std::filesystem::path(std::getenv("HOME")) / "talk_history";
    std::filesystem::create_directories(dir);

    const std::filesystem::path file = dir / (current_user + ".jsonl");

    //追加写入（app 模式保证不覆盖已有内容）
    std::ofstream out(file, std::ios::app);
    out << record.dump() << '\n';
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

            //聊天框 凯
            chat_display->append(QString::fromStdString("[" + sender + "]: " + text));
            //写入聊天记录 凯
            append_chat_history(sender, text);
            continue;
        }

        const std::string data = read_message.value("data", std::string{});
        if (type == "mysqlLoginFeedBack") {
            request_timeout_timer->stop();
            pending_request = pending_request_type::none;
            const bool success = data == "success";
            messageBox::popup(
                _loginWidget,
                success ? "登录成功" : "登录失败",
                success ? messageBox::Type::Success : messageBox::Type::Error
                //TODO 登录成功后读取本地json加载聊天记录 astraclicker
            );
            if (success) {
                current_user = _loginWidget->getUserName();
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
