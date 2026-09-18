#include "mainWidget.h"
#include <QScreen>
#include "serverWidget.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

mainWidget::mainWidget(
    QWidget *parent,
    const std::string &title,
    chatClient &web_api
) : QWidget(parent)
{
    //mainWidget 窗口设置
    this->setWindowTitle(title.c_str());
    this->setMinimumSize(800, 600);

    // 聊天窗口必须同时呈现历史、接收者和输入区；原来的绝对定位会让输入框与按钮重叠。
    auto *main_layout = new QVBoxLayout(this);
    message_history = new QPlainTextEdit(this);
    message_history->setReadOnly(true);
    message_history->setPlaceholderText("聊天记录会显示在这里");

    auto *receiver_layout = new QHBoxLayout();
    auto *receiver_label = new QLabel("接收者：", this);
    receiver_input = new QLineEdit(this);
    receiver_input->setText("root");
    receiver_input->setPlaceholderText("输入对方的用户名");
    receiver_layout->addWidget(receiver_label);
    receiver_layout->addWidget(receiver_input);

    message_input = new QPlainTextEdit(this);
    message_input->setPlaceholderText("输入消息");
    message_input->setMaximumHeight(120);
    button_send = new QPushButton("发送", this);

    main_layout->addWidget(message_history, 1);
    main_layout->addLayout(receiver_layout);
    main_layout->addWidget(message_input);
    main_layout->addWidget(button_send, 0, Qt::AlignRight);

    // WSLg/Wayland 下，鼠标位置可能暂时不属于 Qt 已识别的任何屏幕。
    const QScreen *current_screen = QGuiApplication::screenAt(QCursor::pos());
    if (current_screen == nullptr)
    {
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

    //连接登录请求
    connect(_loginWidget, &loginWidget::loginRequested, this, [&web_api, this]()
    {
        if (pending_request != pending_request_type::none)
        {
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

    //连接发送消息请求
    connect(button_send, &QPushButton::clicked, this, [this, &web_api]()
    {
        nlohmann::json json_data;

        const std::string receiver = receiver_input->text().trimmed().toStdString();
        const std::string text = message_input->toPlainText().trimmed().toStdString();
        if (receiver.empty() || text.empty())
        {
            messageBox::popup(this, "接收者和消息都不能为空", messageBox::Type::Error);
            return;
        }

        json_data["receiver"] = receiver;
        json_data["text"] = text;
        message outgoing_message{.data = json_data.dump(), .type = message_type::text};
        web_api.write(outgoing_message);

        // 本地回显表示消息已进入客户端发送队列；对方收到后会看到服务端转发的记录。
        message_history->appendPlainText(
            QString::fromStdString("[我 -> " + receiver + "]: " + text)
        );
        message_input->clear();
    });

    //连接创建用户请求
    connect(_loginWidget, &loginWidget::createUserRequested, this, [this]()
    {
        _loginWidget->close();
        _createUserWidget->show();
    });

    //连接取消按请求
    connect(_loginWidget, &loginWidget::cancelRequested, this, [this]()
    {
        _loginWidget->close();
    });

    //连接确认创建用户请求
    connect(_createUserWidget, &createUserWidget::createUserRequested, this, [&web_api, this]()
    {
        if (pending_request != pending_request_type::none)
        {
            messageBox::popup(_createUserWidget, "正在等待上一个请求", messageBox::Type::Info);
            return;
        }

        nlohmann::json json_data;
        json_data["userName"] = _createUserWidget->getUserName();
        json_data["password"] = _createUserWidget->getPassword();
        message outgoing_message{.data = json_data.dump(), .type = message_type::create_user_requested};
        web_api.write(outgoing_message);
        pending_request = pending_request_type::create_user;
        request_timeout_timer->start(5000);
    });

    //连接取消创建用户请求
    connect(_createUserWidget, &createUserWidget::cancelRequested, this, [this]()
    {
        _createUserWidget->close();
        _loginWidget->show();
    });

    _loginWidget->show();
    request_timeout_timer = new QTimer(this);
    request_timeout_timer->setSingleShot(true);
    connect(request_timeout_timer, &QTimer::timeout, this, [this]()
    {
        QWidget *request_window = pending_request == pending_request_type::create_user
                                      ? static_cast<QWidget *>(_createUserWidget)
                                      : static_cast<QWidget *>(_loginWidget);
        pending_request = pending_request_type::none;
        messageBox::popup(request_window, "请求超时", messageBox::Type::Error);
    });

    // 20 ms 轮询不会阻塞 GUI，也避免原先 0 ms 自递归持续占用一个 CPU 核心。
    message_poll_timer = new QTimer(this);
    connect(message_poll_timer, &QTimer::timeout, this, [&web_api, this]()
    {
        process_received_messages(web_api);
    });
    message_poll_timer->start(20);
}

void mainWidget::process_received_messages(chatClient &web_api)
{
    nlohmann::json read_message;
    while (web_api.try_pop_message(read_message))
    {
        const std::string type = read_message.value("type", std::string{});
        if (type == "text")
        {
            const auto data = read_message.value("data", nlohmann::json::object());
            const std::string sender = data.value("sender", std::string{});
            const std::string text = data.value("text", std::string{});

            // 收到的聊天内容应留在主窗口中，短暂的桌面提示不能充当聊天记录。
            message_history->appendPlainText(QString::fromStdString("[" + sender + "]: " + text));
            continue;
        }

        const std::string data = read_message.value("data", std::string{});
        if (type == "mysqlLoginFeedBack")
        {
            request_timeout_timer->stop();
            pending_request = pending_request_type::none;
            const bool success = data == "success";
            messageBox::popup(
                _loginWidget,
                success ? "登录成功" : "登录失败",
                success ? messageBox::Type::Success : messageBox::Type::Error
            );
            if (success)
            {
                show();
                _loginWidget->close();
            }
            continue;
        }

        if (type == "mysqlCreateUserFeedBack")
        {
            request_timeout_timer->stop();
            pending_request = pending_request_type::none;
            const bool success = data == "success";
            messageBox::popup(
                _createUserWidget,
                success ? "注册成功" : "注册失败：用户名已存在",
                success ? messageBox::Type::Success : messageBox::Type::Error
            );
            if (success)
            {
                _createUserWidget->close();
                _loginWidget->show();
            }
        }
    }
}
