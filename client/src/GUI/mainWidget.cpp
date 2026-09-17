#include "mainWidget.h"
#include <QScreen>
#include <iostream>
#include <ctime>
#include "serverWidget.h"
#include <QTime>

mainWidget::mainWidget(
    QWidget *parent,
    const std::string &title,
    chatClient &webAPI
) : QWidget(parent) {
    //mainWidget 窗口设置
    this->setWindowTitle(title.c_str());
    this->setMinimumSize(800, 600);

    //mainWidget 组件初始化
    textEdit = new QPlainTextEdit(this);
    btnSend = new QPushButton(this);
    btnSend->setText("发送");
    btnSend->move(100, 100);
    btnSend->setFixedSize(200, 100);
    btnSend->show();

    //获取屏幕长宽
    const QScreen *currentScreen = QGuiApplication::screenAt(QCursor::pos());
    const QRect geom = currentScreen->geometry();
    this->screenH = geom.height();
    this->screenW = geom.width();

    //创建子窗口
    _loginWidget = new loginWidget(nullptr, screenW, screenH);
    _createUserWidget = new createUserWidget(nullptr, screenW, screenH);

    //连接登录请求
    connect(_loginWidget, &loginWidget::loginRequested, this, [&webAPI, this]() {
        nlohmann::json jsonData;
        jsonData["userName"] = _loginWidget->getUserName();
        jsonData["password"] = _loginWidget->getPassword();
        msg temp{.data = jsonData.dump(), .msgType = msgType::loginRequested};
        webAPI.write(temp);
        if (checkReturn(webAPI, this)) {
            this->show();
            _loginWidget->close();
        }
    });

    //连接发送消息请求
    connect(btnSend, &QPushButton::clicked, this, [this, &webAPI]() {
        nlohmann::json jsonData;

        //TODO 指定接收者
        //测试版本，只发给root
        jsonData["receiver"] = "root";
        jsonData["text"] = textEdit->toPlainText().toStdString();
        msg temp{.data = jsonData.dump(), .msgType = msgType::text};
        webAPI.write(temp);
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
        msg temp{.data = jsonData.dump(), .msgType = msgType::createUserRequested};
        webAPI.write(temp);
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
            messageBox::popup(nullptr, "[" + sender + "]: " + text, messageBox::Type::Info);
            webAPI.requestedDeque.pop_front();
            return true;
        }
    }
    return false;
}
