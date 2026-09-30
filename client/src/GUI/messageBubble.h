#pragma once
#include <QLabel>
#include <QString>
#include <QWidget>

//聊天气泡: 继承 QLabel,把"发送人 + 发送时间 + 消息正文"三段用富文本排在一个气泡里
//抬头(发送人/时间)是小字,正文是正常字号;配色按 userType 区分自己/别人
class messageBubble : public QLabel {
    Q_OBJECT

public:
    //这条消息是谁发的,决定气泡的配色和靠哪一侧
    //放在这里是为了让 messageArea 和 messageBubble 不互相包含(避免循环依赖)
    enum class userType {
        currentUser,
        otherUser
    };

    messageBubble(const QString &sender, const QString &sendTime, const QString &text,
                  userType type, QWidget *parent = nullptr);
};
