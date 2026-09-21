#pragma once
#include "serverWidget.h"
#include <QWidget>
#include <QPropertyAnimation>
#include <QApplication>
#include <QPlainTextEdit>
#include <QTextEdit>
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

    //当前登录的用户名，用于定位该用户的聊天记录文件
    std::string current_user;

    //聊天记录显示区（只读，用于展示收到的消息）
    QTextEdit *chat_display;

    //多行文本输入框
    QPlainTextEdit *message_input;

    //发送按钮
    QPushButton *button_send;

    //初始化信号与槽的连接
    void initConnect(chatClient &web_api);

    //把一条聊天消息追加写入当前用户的本地记录文件
    void append_chat_history(const std::string &sender, const std::string &text);

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
