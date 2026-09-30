#include "messageBubble.h"

//聊天气泡
messageBubble::messageBubble(const QString &sender, const QString &sendTime, const QString &text,
                             const userType type, QWidget *parent)
    : QLabel(parent) {
    const bool mine = (type == userType::currentUser);
    //抬头的小字颜色:
    const QString metaColor = mine ? QStringLiteral("#dceefb") : QStringLiteral("#7f8c8d");
    const QString bodyColor = mine ? QStringLiteral("#ffffff") : QStringLiteral("#333333");

    //富文本里的 < & 等字符必须转义,换行要写成 <br>,否则会被当成标签/空白折叠掉
    QString body = text;
    body = body.toHtmlEscaped();
    body.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));

    const QString html =
            QStringLiteral(
                "<span style='font-size:11px; color:%1;'>%2&nbsp;&nbsp;%3</span><br/>"
                "<span style='font-size:14px; color:%4;'>%5</span>")
            .arg(metaColor,
                 sender.toHtmlEscaped(),
                 sendTime.toHtmlEscaped(),
                 bodyColor,
                 body);

    setTextFormat(Qt::RichText);
    setText(html);
    setWordWrap(true);
    //气泡里允许选中复制
    setTextInteractionFlags(Qt::TextSelectableByMouse);

    //自己发的：主色底 + 白字；别人发的：浅底 + 深字 + 描边
    if (mine) {
        setStyleSheet("QLabel { background-color: #3498db; color: white;"
            " border-radius: 8px; padding: 6px 10px; }");
    } else {
        setStyleSheet("QLabel { background-color: #f0f2f5; color: #333333;"
            " border: 1px solid #d0d7de; border-radius: 8px; padding: 6px 10px; }");
    }
}
