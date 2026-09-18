#pragma once
#include "serverWidget.h"
#include <QWidget>
#include <QPropertyAnimation>
#include <QApplication>
#include <QPlainTextEdit>
#include <QLineEdit>
#include "../Web/client.h"


class mainWidget : public QWidget {
    Q_OBJECT

protected:
    //屏幕长宽
    int screenW;
    int screenH;

    //投入qt循环函数
    bool step(chatClient &webAPI, QWidget *parent);

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
        chatClient &webAPI
    );

    //子窗口
    loginWidget *_loginWidget;
    createUserWidget *_createUserWidget;

    static bool checkReturn(chatClient &webAPI, QWidget *parent);

    bool checkReturnLoop(chatClient &webAPI, QWidget *parent);
};
