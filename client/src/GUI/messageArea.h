#pragma once
#include <QScrollArea>
#include <QWidget>
#include <string>
#include "messageBubble.h"

class QVBoxLayout;

//信息展示界面
class messageArea : public QScrollArea {
protected:
    QWidget *content;
    QVBoxLayout *contentLayout;

public:
    messageArea();

    //添加一条消息: 发送人、发送时间、正文分开传,由 messageBubble 负责显示
    void addContent(const std::string &sender, const std::string &sendTime,
                    const std::string &text, messageBubble::userType type);

    //清空消息区(按时间顺序重新渲染前调用)
    void clearContent();
};
