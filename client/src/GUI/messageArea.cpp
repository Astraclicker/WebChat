#include "messageArea.h"
#include <QFrame>
#include <QHBoxLayout>
#include <QLayout>
#include <QVBoxLayout>

messageArea::messageArea() {
    this->setWidgetResizable(true);

    //内容Widget
    content = new QWidget;
    contentLayout = new QVBoxLayout(content);
    this->setWidget(content);
    setFrameShape(QFrame::NoFrame);
    contentLayout->setContentsMargins(8, 8, 8, 8);
    contentLayout->setSpacing(6);
}

void messageArea::addContent(const std::string &sender, const std::string &sendTime,
                             const std::string &text, const messageBubble::userType type) {
    //气泡自己负责显示"发送人 + 时间 + 正文"和配色,这里只管摆放
    auto *bubble = new messageBubble(QString::fromStdString(sender),
                                     QString::fromStdString(sendTime),
                                     QString::fromStdString(text),
                                     type, content);

    //长消息的换行宽度上限：不超过视口的 3/4，否则气泡会把整行撑满，就不像气泡了
    bubble->setMaximumWidth(qMax(200, viewport()->width() * 3 / 4));

    //气泡靠左/靠右：外面套一层 QHBoxLayout，用弹性空白把气泡顶到对应一侧
    auto *row = new QHBoxLayout();
    row->setContentsMargins(0, 0, 0, 0);
    if (type == messageBubble::userType::currentUser) {
        row->addStretch(1); //自己发的：左边留白，气泡贴右
        row->addWidget(bubble, 0);
    } else {
        row->addWidget(bubble, 0); //别人发的：气泡贴左，右边留白
        row->addStretch(1);
    }

    contentLayout->addLayout(row);
}

//清空消息区(按时间顺序重新渲染前调用)
void messageArea::clearContent() {
    //每一行是一个 QHBoxLayout,里面的气泡是 content 的子控件,
    //删布局不会连带删掉控件,所以两边都要处理
    while (QLayoutItem *item = contentLayout->takeAt(0)) {
        if (QLayout *row = item->layout()) {
            while (QLayoutItem *child = row->takeAt(0)) {
                if (QWidget *bubble = child->widget()) {
                    bubble->setParent(nullptr);
                    bubble->deleteLater();
                }
                delete child;
            }
            //布局对象本身就是 item(QLayout 继承自 QLayoutItem,addLayout 存的就是这个指针),
            //所以必须 continue 跳过末尾的 delete item,否则同一块内存被释放两次
            delete row;
            continue;
        }
        if (QWidget *widget = item->widget()) {
            widget->setParent(nullptr);
            widget->deleteLater();
        }
        delete item;
    }
}
