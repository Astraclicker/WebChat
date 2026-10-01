#include "mainWidget.h"
#include <QScreen>
#include <QVBoxLayout>
#include "messageBox.h"
#include "messageBubble.h"
#include <algorithm>
#include <iostream>
#include <string>
#include <vector>
#include <log.h>
#include "../SQLite/SQLite.h"

mainWidget::mainWidget(
    QWidget *parent,
    const std::string &title,
    chatClient &web_api
) : QWidget(parent), chatHistory(nullptr) {
    //mainWidget 窗口设置
    this->setWindowTitle(title.c_str());
    this->setMinimumSize(800, 600);

    //消息发送及显示GUI
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

        if (!web_api.is_connected()) {
            messageBox::popup(this, "未连接到服务端，消息未发送", messageBox::Type::Error);
            return;
        }
        json_data["text"] = text;

        const std::string send_time = nowLocalTimestamp();
        json_data["sendTime"] = send_time;

        _messageArea->addContent(current_user, send_time, text, messageBubble::userType::currentUser);

        if (sqliteDB != nullptr) {
            sqliteDB->chatDataInsert(current_user, text, send_time);
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
            LOG(astra_log::Level::Error, "process message failed: ", error.what());
        }
    });
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
            //用服务端回传的发送时间(取不到才退回本机时间): 两端存同一个值
            const std::string send_time = data.value("sendTime", nowLocalTimestamp());

            _messageArea->addContent(sender, send_time, text, messageBubble::userType::otherUser);
            if (sqliteDB != nullptr) {
                sqliteDB->chatDataInsert(sender, text, send_time);
            }
            continue;
        }

        if (type == "syncResponse") {
            handle_sync_response(read_message.value("data", nlohmann::json::object()));
            continue;
        }

        if (type == "error") {
            messageBox::popup(this, read_message.value("data", std::string{}), messageBox::Type::Error);
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
                //创建聊天记录数据库
                const long long uid = read_message.value("uid", 0LL);
                sqliteDB = new my_SQLite(uid, current_user, _loginWidget->getPassword());
                //读取SQLite聊天记录到前端
                load_chat_history();
                //与服务器对比同步最新聊天记录
                request_sync(web_api);
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
            );
            if (success) {
                _createUserWidget->close();
                _loginWidget->show();
            }
        }
    }
}

//登录成功后，把本地 SQLite 里当前用户的聊天记录写入前端
void mainWidget::load_chat_history() const {
    if (sqliteDB == nullptr || _messageArea == nullptr) {
        return;
    }

    try {
        const auto history = sqliteDB->chatDataSearch();
        const auto senders = history.value("sender", nlohmann::json::array());
        const auto texts = history.value("text", nlohmann::json::array());
        const auto send_times = history.value("sendTime", nlohmann::json::array());

        struct historyRow {
            std::string sender;
            std::string text;
            std::string sendTime;
        };
        std::vector<historyRow> rows;

        const std::size_t count = std::min({senders.size(), texts.size(), send_times.size()});
        rows.reserve(count);
        for (std::size_t i = 0; i < count; ++i) {
            if (!senders[i].is_string() || !texts[i].is_string() || !send_times[i].is_string()) {
                continue;
            }
            rows.push_back({
                senders[i].get<std::string>(),
                texts[i].get<std::string>(),
                send_times[i].get<std::string>()
            });
        }

        //按时间显示:"YYYY-MM-DD HH:MM:SS"
        std::ranges::stable_sort(rows, [](const historyRow &a, const historyRow &b) {
            return a.sendTime < b.sendTime;
        });

        _messageArea->clearContent();
        for (const auto &row: rows) {
            const auto type = row.sender == current_user
                                  ? messageBubble::userType::currentUser
                                  : messageBubble::userType::otherUser;
            _messageArea->addContent(row.sender, row.sendTime, row.text, type);
        }

        LOG(astra_log::Level::Info, "load chat history done, uid=", sqliteDB->getUid(),
            ", records=", rows.size());
    } catch (const std::exception &error) {
        LOG(astra_log::Level::Error, "load chat history failed: ", error.what());
    }
}

void mainWidget::request_sync(chatClient &web_api) const {
    if (sqliteDB == nullptr) {
        return;
    }

    const auto local_last = sqliteDB->lastMessage();

    nlohmann::json request;
    request["lastSender"] = local_last.value("sender", std::string{});
    request["lastText"] = local_last.value("text", std::string{});
    request["lastTime"] = local_last.value("sendTime", std::string{});

    message outgoing_message{.data = request.dump(), .type = message_type::sync_requested};
    web_api.write(outgoing_message);
}

void mainWidget::handle_sync_response(const nlohmann::json &data) const {
    if (sqliteDB == nullptr || _messageArea == nullptr) {
        return;
    }

    if (data.value("matched", true)) {
        //服务端最新一条和本地一致
        LOG(astra_log::Level::Info, "sync skipped: local and server agree");
        return;
    }

    const auto messages = data.value("messages", nlohmann::json::array());
    if (!messages.is_array()) {
        return;
    }

    int added = 0;
    for (const auto &one: messages) {
        if (!one.is_object()) {
            continue;
        }

        const std::string sender = one.value("sender", std::string{});
        const std::string text = one.value("text", std::string{});
        const std::string send_time = one.value("sendTime", std::string{});
        if (sender.empty() || text.empty() || send_time.size() != 19) {
            continue;
        }

        if (sqliteDB->hasMessage(sender, text, send_time)) {
            continue;
        }

        sqliteDB->chatDataInsert(sender, text, send_time);
        ++added;
    }

    LOG(astra_log::Level::Info, "sync done: server sent ", messages.size(),
        " records, filled ", added);

    if (added > 0) {
        load_chat_history();
    }
}
