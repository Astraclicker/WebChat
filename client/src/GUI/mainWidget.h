#pragma once
#include "serverWidget.h"
#include <QWidget>
#include <QPropertyAnimation>
#include <QApplication>
#include <QPlainTextEdit>
#include <QTimer>
#include "../Web/client.h"


class mainWidget : public QWidget {
    Q_OBJECT

protected:
    enum class pending_request_type {
        none,
        login,
        createUser
    };

    //屏幕长宽
    int screenW;
    int screenH;

    // Qt 定时器只做一次非阻塞取队列，不在 GUI 线程中忙等网络响应。
    void process_received_messages(chatClient &web_api);

    QTimer *message_poll_timer;
    QTimer *request_timeout_timer;
    pending_request_type pending_request = pending_request_type::none;

    //多行文本输入框
    QPlainTextEdit *message_input;

    //发送按钮
    QPushButton *button_send;

    //初始化信号与槽的连接
    void initConnect(chatClient &web_api);

public:
    //构造函数
    mainWidget(
        QWidget *parent,
        const std::string &title,
        chatClient &web_api
    );

    //子窗口
    loginWidget *_loginWidget;
    createUserWidget *_createUserWidget;
};
