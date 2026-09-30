#pragma once
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPointer>
#include <QTimer>
#include <QVector>
#include <QWidget>
#include <string>

//提示框
class messageBox : public QWidget {
    Q_OBJECT

public:
    //提示类型，决定配色
    enum class Type {
        Success, //绿色：成功
        Error, //红色：失败
        Info //蓝色：普通提示
    };

    //构造即弹出；durationMs <= 0 表示不自动消失
    explicit messageBox(QWidget *parent, const std::string &text,
                        Type type = Type::Info, int durationMs = 3000);

    //便捷入口，等价于 new messageBox(...)
    static void popup(QWidget *parent, const std::string &text,
                      Type type = Type::Info, int durationMs = 3000);

    ~messageBox() override;

    //监听父窗口尺寸变化，把自己重新贴回右上角
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    //自绘卡片阴影：整棵提示子树只允许挂一个 QGraphicsEffect(子控件模式的 _fade)，
    void paintEvent(QPaintEvent *event) override;

    //把同一组提示重新排到右上角（多个并存时自上而下堆叠）
    //有父窗口时贴父窗口右上角；parent 为空时贴屏幕可用区域右上角（避开任务栏）
    void relayout() const;

    //淡入淡出：子控件走 QGraphicsOpacityEffect，独立窗口走 windowOpacity
    void animateOpacity(qreal from, qreal to, int duration,
                        QEasingCurve::Type curve, bool deleteWhenFinished);

    //淡出并销毁自己
    void dismiss();

    QFrame *_card = nullptr;
    QLabel *_label = nullptr;
    QGraphicsOpacityEffect *_fade = nullptr;
    QTimer *_closeTimer = nullptr;
    bool _closing = false;
    bool _isWindow = false; //parent 为空：作为独立的无边框窗口弹在桌面上

    //每个父窗口当前正在显示的提示
    static QHash<QWidget *, QVector<QPointer<messageBox> > > &activeBoxes();
};
