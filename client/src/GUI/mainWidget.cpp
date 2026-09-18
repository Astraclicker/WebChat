#include "mainWidget.h"
#include <QScreen>
#include <iostream>
#include <ctime>
#include "serverWidget.h"
#include <QTime>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

mainWidget::mainWidget(
    QWidget *parent,
    const std::string &title,
    chatClient &webAPI
) : QWidget(parent) {
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
    connect(_loginWidget, &loginWidget::loginRequested, this, [&webAPI, this]() {
        nlohmann::json jsonData;
        jsonData["userName"] = _loginWidget->getUserName();
        jsonData["password"] = _loginWidget->getPassword();
        message outgoing_message{.data = jsonData.dump(), .type = message_type::login_requested};
        webAPI.write(outgoing_message);
        if (checkReturn(webAPI, this)) {
            this->show();
            _loginWidget->close();
        }
    });

    //连接发送消息请求
    connect(button_send, &QPushButton::clicked, this, [this, &webAPI]() {
        nlohmann::json jsonData;

        const std::string receiver = receiver_input->text().trimmed().toStdString();
        const std::string text = message_input->toPlainText().trimmed().toStdString();
        if (receiver.empty() || text.empty())
        {
            messageBox::popup(this, "接收者和消息都不能为空", messageBox::Type::Error);
            return;
        }

        jsonData["receiver"] = receiver;
        jsonData["text"] = text;
        message outgoing_message{.data = jsonData.dump(), .type = message_type::text};
        webAPI.write(outgoing_message);

        // 本地回显表示消息已进入客户端发送队列；对方收到后会看到服务端转发的记录。
        message_history->appendPlainText(
            QString::fromStdString("[我 -> " + receiver + "]: " + text)
        );
        message_input->clear();
    });

    //连接创建用户请求
    connect(_loginWidget, &loginWidget::createUserRequested, this, [this]() {
        _loginWidget->close();
        _createUserWidget->show();
    });

    //连接取消按请求
    connect(_loginWidget, &loginWidget::cancelRequested, this, [this]() {
        _loginWidget->close();
    });

    //连接确认创建用户请求
    connect(_createUserWidget, &createUserWidget::createUserRequested, this, [&webAPI, this]() {
        nlohmann::json jsonData;
        jsonData["userName"] = _createUserWidget->getUserName();
        jsonData["password"] = _createUserWidget->getPassword();
        message outgoing_message{.data = jsonData.dump(), .type = message_type::create_user_requested};
        webAPI.write(outgoing_message);
        if (checkReturn(webAPI, _createUserWidget)) {
            _createUserWidget->close();
            _loginWidget->show();
        }
    });

    //连接取消创建用户请求
    connect(_createUserWidget, &createUserWidget::cancelRequested, this, [this]() {
        _createUserWidget->close();
        _loginWidget->show();
    });

    _loginWidget->show();
    step(webAPI, this);
}

bool mainWidget::step(chatClient &webAPI, QWidget *parent) {
    const bool feedback = checkReturnLoop(webAPI, parent);
    QTimer::singleShot(0, this, [&]() { step(webAPI, parent); });
    return feedback;
}

// 作为槽函数，服务于登录，注册请求
bool mainWidget::checkReturn(chatClient &webAPI, QWidget *parent) {
    const clock_t start = clock();
    clock_t end = clock();
    while ((end - start) / CLOCKS_PER_SEC < 5) {
        if (!webAPI.requestedDeque.empty()) {
            const auto readMsg = webAPI.requestedDeque.front();

            const auto type = readMsg.value("type", std::string{});
            const auto data = readMsg.value("data", std::string{});;
            if (type == "mysqlLoginFeedBack") {
                messageBox::popup(nullptr, data == "success" ? "登录成功" : "登录失败",
                                  data == "success" ? messageBox::Type::Success : messageBox::Type::Error);
            }
            if (type == "mysqlCreateUserFeedBack") {
                messageBox::popup(nullptr, data == "success" ? "注册成功" : "注册失败:用户名已存在",
                                  data == "success" ? messageBox::Type::Success : messageBox::Type::Error);
            }
            webAPI.requestedDeque.pop_front();
            return data == "success";
        }
        end = clock();
    }
    new messageBox(parent, "请求超时", messageBox::Type::Error);
    return false;
}

// 投递在qt循环中，用于处理接收消息
bool mainWidget::checkReturnLoop(chatClient &webAPI, QWidget *parent) {
    if (!webAPI.requestedDeque.empty()) {
        const auto readMsg = webAPI.requestedDeque.front();

        const auto type = readMsg.value("type", std::string{});
        const auto data = readMsg.value("data", nlohmann::json::object());
        const auto sender = data.value("sender", std::string{});
        const auto text = data.value("text", std::string{});

        if (type == "text") {
            // 收到的聊天内容应留在主窗口中，短暂的桌面提示不能充当聊天记录。
            message_history->appendPlainText(QString::fromStdString("[" + sender + "]: " + text));
            webAPI.requestedDeque.pop_front();
            return true;
        }
    }
    return false;
}
