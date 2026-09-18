#pragma once
#include "serverWidget.h"
#include <QWidget>
#include <QPropertyAnimation>
#include <QApplication>
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QTimer>
#include "../Web/client.h"


class mainWidget : public QWidget {
    Q_OBJECT

protected:
    enum class pending_request_type
    {
        none,
        login,
        create_user
    };

    //屏幕长宽
    int screenW;
    int screenH;

    // Qt 定时器只做一次非阻塞取队列，不在 GUI 线程中忙等网络响应。
    void process_received_messages(chatClient &web_api);
    QTimer *message_poll_timer;
    QTimer *request_timeout_timer;
    pending_request_type pending_request = pending_request_type::none;

    // 聊天记录与消息输入必须分开，否则发送后窗口没有任何可观察结果。
    QPlainTextEdit *message_history;
    QPlainTextEdit *message_input;

    // 服务端按用户名路由消息，客户端需要允许用户明确指定接收者。
    QLineEdit *receiver_input;
    QPushButton *button_send;

public:
    mainWidget(
        QWidget *parent,
        const std::string &title,
        chatClient &web_api
    );

    //子窗口
    loginWidget *_loginWidget;
    createUserWidget *_createUserWidget;

};
