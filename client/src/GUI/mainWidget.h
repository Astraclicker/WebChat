#pragma once
#include "serverWidget.h"
#include <QWidget>
#include <QPropertyAnimation>
#include <QApplication>
#include <QPlainTextEdit>
#include "../Web/client.h"


class mainWidget : public QWidget {
    Q_OBJECT

protected:
    //屏幕长宽
    int screenW;
    int screenH;

    //投入qt循环函数
    bool step(chatClient &webAPI, QWidget *parent);

    //多行文本输入框
    QPlainTextEdit *textEdit;

    //发送按钮
    QPushButton *btnSend;

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

    static bool checkReturnLoop(chatClient &webAPI, QWidget *parent);
};
