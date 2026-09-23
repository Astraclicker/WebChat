#pragma once
#include <QLineEdit>
#include <QWidget>
#include <QPushButton>
#include <QString>
#include <QFrame>
#include <QLabel>
#include <QTimer>
#include <QPointer>
#include <QVector>
#include <QScrollArea>
#include <QEasingCurve>
#include <QGraphicsOpacityEffect>
#include <QVBoxLayout>

//登录界面
class loginWidget : public QWidget {
    Q_OBJECT

protected:
    // 输入框样式
    QString inputStyle = R"(
    QLineEdit {
        background-color: #f5f5f5;
        border: 1px solid #d0d7de;
        border-radius: 6px;
        padding: 8px 12px;
        font-size: 14px;
        color: #333;
    }
    QLineEdit:focus {
        border: 1px solid #3498db;
        background-color: white;
    }
)";

    // 主按钮样式
    QString primaryButtonStyle = R"(
    QPushButton {
        background-color: #3498db;
        color: white;
        border: none;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 14px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #2980b9;
    }
    QPushButton:pressed {
        background-color: #1f6da0;
    }
)";

    // 次要按钮样式
    QString secondaryButtonStyle = R"(
    QPushButton {
        background-color: #e9ecef;
        color: #333;
        border: 1px solid #ced4da;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 14px;
    }
    QPushButton:hover {
        background-color: #dee2e6;
    }
    QPushButton:pressed {
        background-color: #ced4da;
    }
)";


    //账号密码输入框
    QLineEdit *userNameEdit;
    QLineEdit *passwordEdit;

    //按钮
    QPushButton *btnLogin;
    QPushButton *btnCancel;
    QPushButton *btnCreateUser;

    //账号密码等数据
    QString userName;
    QString password;

public:
    //构造函数
    loginWidget(QWidget *parent, int width, int height);

    //获取账号密码
    [[nodiscard]] std::string getUserName() const {
        return userName.toStdString();
    }

    [[nodiscard]] std::string getPassword() const {
        return password.toStdString();
    }

signals:
    //登录请求
    void loginRequested();

    //创建用户请求
    void createUserRequested();

    //关闭窗口请求
    void cancelRequested();
};

//注册用户界面
class createUserWidget : public QWidget {
    Q_OBJECT

protected:
    // 输入框样式
    QString inputStyle = R"(
    QLineEdit {
        background-color: #f5f5f5;
        border: 1px solid #d0d7de;
        border-radius: 6px;
        padding: 8px 12px;
        font-size: 14px;
        color: #333;
    }
    QLineEdit:focus {
        border: 1px solid #3498db;
        background-color: white;
    }
)";

    // 主按钮样式
    QString primaryButtonStyle = R"(
    QPushButton {
        background-color: #3498db;
        color: white;
        border: none;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 14px;
        font-weight: bold;
    }
    QPushButton:hover {
        background-color: #2980b9;
    }
    QPushButton:pressed {
        background-color: #1f6da0;
    }
)";

    // 次要按钮样式
    QString secondaryButtonStyle = R"(
    QPushButton {
        background-color: #e9ecef;
        color: #333;
        border: 1px solid #ced4da;
        border-radius: 6px;
        padding: 8px 16px;
        font-size: 14px;
    }
    QPushButton:hover {
        background-color: #dee2e6;
    }
    QPushButton:pressed {
        background-color: #ced4da;
    }
)";


    //账号密码输入框
    QLineEdit *userNameEdit;
    QLineEdit *passwordEdit;

    //按钮
    QPushButton *btnCreateUser;
    QPushButton *btnCancel;

    //账号密码等数据
    QString userName;
    QString password;

public:
    //构造函数
    createUserWidget(QWidget *parent, int width, int height);

    //获取账号密码
    [[nodiscard]] std::string getUserName() const {
        return userName.toStdString();
    }

    [[nodiscard]] std::string getPassword() const {
        return password.toStdString();
    }

signals:
    //确认创建请求
    void createUserRequested();

    //关闭窗口请求
    void cancelRequested();
};

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

//信息展示界面
class messageArea : public QScrollArea {
protected:
    QWidget *content;
    QVBoxLayout *contentLayout;

public:
    enum class userType {
        currentUser,
        otherUser
    };

    messageArea();

    //添加内容
    void addContent(const std::string &text,userType type);
};
