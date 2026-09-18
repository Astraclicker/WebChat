#include "serverWidget.h"
#include <QGraphicsDropShadowEffect>
#include <QEvent>
#include <QGuiApplication>
#include <QPropertyAnimation>
#include <QTimer>
#include <QLabel>
#include <QHBoxLayout>
#include <QCursor>
#include <QScreen>

loginWidget::loginWidget(QWidget *parent, const int width, const int height) : QWidget(parent) {
    this->setWindowTitle("登录");
    this->setStyleSheet("QWidget { background-color: #f0f2f5; }");
    this->setFixedWidth(width / 5);
    this->setFixedHeight(height / 2);

    // 创建登录卡片容器
    auto *card = new QFrame(this);
    card->setObjectName("loginCard");
    card->setStyleSheet(R"(
    #loginCard {
        background-color: white;
        border-radius: 8px;
    }
)");

    // 添加阴影效果
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    // 卡片内部布局
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(50, 30, 50, 25);
    cardLayout->setSpacing(15);

    // 标题
    auto *titleLabel = new QLabel("登录", card);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #333; margin-bottom: 10px;");
    cardLayout->addWidget(titleLabel);

    // 输入框
    userNameEdit = new QLineEdit(card);
    userNameEdit->setPlaceholderText("用户名");
    userNameEdit->setMinimumHeight(40);
    userNameEdit->setStyleSheet(inputStyle);

    passwordEdit = new QLineEdit(card);
    passwordEdit->setPlaceholderText("密码");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMinimumHeight(40);
    passwordEdit->setStyleSheet(inputStyle);

    cardLayout->addWidget(userNameEdit);
    cardLayout->addWidget(passwordEdit);

    // 按钮行（确定 + 取消）
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    btnLogin = new QPushButton("登录", card);
    btnLogin->setMinimumHeight(40);
    btnLogin->setCursor(Qt::PointingHandCursor);
    btnLogin->setStyleSheet(primaryButtonStyle); // 主按钮样式

    btnCancel = new QPushButton("取消", card);
    btnCancel->setMinimumHeight(40);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setStyleSheet(secondaryButtonStyle); // 次要按钮样式

    buttonLayout->addWidget(btnLogin);
    buttonLayout->addWidget(btnCancel);
    cardLayout->addLayout(buttonLayout);

    // 创建用户链接（扁平按钮）
    btnCreateUser = new QPushButton("创建新用户", card);
    btnCreateUser->setCursor(Qt::PointingHandCursor);
    btnCreateUser->setFlat(true);
    btnCreateUser->setStyleSheet(
        "QPushButton { color: #3498db; font-size: 13px; border: none; background: transparent; }"
        "QPushButton:hover { color: #2980b9; text-decoration: underline; }");
    cardLayout->addWidget(btnCreateUser);

    // 将卡片居中放在主窗口
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch(); // 上方弹性空间
    mainLayout->addWidget(card, 0, Qt::AlignHCenter); // 水平居中
    mainLayout->addStretch(); // 下方弹性空间
    mainLayout->setContentsMargins(20, 20, 20, 20);

    //连接按钮与请求
    connect(btnLogin, &QPushButton::clicked, [this]() {
        this->password = passwordEdit->text();
        this->userName = userNameEdit->text();
        userNameEdit->clear();
        passwordEdit->clear();
        emit loginRequested();
    });

    connect(btnCancel, &QPushButton::clicked, [this]() {
        emit cancelRequested();
    });

    connect(btnCreateUser, &QPushButton::clicked, [this]() {
        emit createUserRequested();
    });
}

createUserWidget::createUserWidget(QWidget *parent, int width, int height) : QWidget(parent) {
    this->setWindowTitle("注册");
    this->setStyleSheet("QWidget { background-color: #f0f2f5; }");
    this->setFixedWidth(width / 5);
    this->setFixedHeight(height / 2);

    // 创建登录卡片容器
    auto *card = new QFrame(this);
    card->setObjectName("loginCard");
    card->setStyleSheet(R"(
    #loginCard {
        background-color: white;
        border-radius: 8px;
    }
)");

    // 添加阴影效果
    auto *shadow = new QGraphicsDropShadowEffect(card);
    shadow->setBlurRadius(20);
    shadow->setColor(QColor(0, 0, 0, 60));
    shadow->setOffset(0, 4);
    card->setGraphicsEffect(shadow);

    // 卡片内部布局
    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(30, 30, 30, 25);
    cardLayout->setSpacing(15);

    // 标题
    auto *titleLabel = new QLabel("注册", card);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 22px; font-weight: bold; color: #333; margin-bottom: 10px;");
    cardLayout->addWidget(titleLabel);

    // 输入框
    userNameEdit = new QLineEdit(card);
    userNameEdit->setPlaceholderText("用户名");
    userNameEdit->setMinimumHeight(40);
    userNameEdit->setStyleSheet(inputStyle);

    passwordEdit = new QLineEdit(card);
    passwordEdit->setPlaceholderText("密码");
    passwordEdit->setEchoMode(QLineEdit::Password);
    passwordEdit->setMinimumHeight(40);
    passwordEdit->setStyleSheet(inputStyle);

    cardLayout->addWidget(userNameEdit);
    cardLayout->addWidget(passwordEdit);

    // 按钮行（确定 + 取消）
    auto *buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    btnCreateUser = new QPushButton("注册", card);
    btnCreateUser->setMinimumHeight(40);
    btnCreateUser->setCursor(Qt::PointingHandCursor);
    btnCreateUser->setStyleSheet(primaryButtonStyle); // 主按钮样式

    btnCancel = new QPushButton("取消", card);
    btnCancel->setMinimumHeight(40);
    btnCancel->setCursor(Qt::PointingHandCursor);
    btnCancel->setStyleSheet(secondaryButtonStyle); // 次要按钮样式

    buttonLayout->addWidget(btnCreateUser);
    buttonLayout->addWidget(btnCancel);
    cardLayout->addLayout(buttonLayout);

    // 将卡片居中放在主窗口
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addStretch(); // 上方弹性空间
    mainLayout->addWidget(card, 0, Qt::AlignHCenter); // 水平居中
    mainLayout->addStretch(); // 下方弹性空间
    mainLayout->setContentsMargins(20, 20, 20, 20);

    //连接请求
    connect(btnCreateUser, &QPushButton::clicked, this, [this]() {
        userName = userNameEdit->text();
        password = passwordEdit->text();
        userNameEdit->clear();
        passwordEdit->clear();
        emit createUserRequested();
    });

    connect(btnCancel, &QPushButton::clicked, this, [this]() {
        emit cancelRequested();
    });
}

namespace {
    constexpr int kToastMargin = 16;    //距离父窗口右上角的外边距
    constexpr int kToastSpacing = 10;   //多个提示之间的纵向间距
    constexpr int kShadowPad = 12;      //卡片四周给阴影留的空白
    constexpr int kCardPadH = 14;       //卡片内左右留白
    constexpr int kCardPadV = 12;       //卡片内上下留白
    constexpr int kCardMaxWidth = 300;  //卡片最大宽度
    constexpr int kCardMinWidth = 160;  //卡片最小宽度

    //按提示类型取配色
    QString colorOf(const messageBox::Type type) {
        switch (type) {
            case messageBox::Type::Success:
                return "#27ae60";
            case messageBox::Type::Error:
                return "#e74c3c";
            case messageBox::Type::Info:
            default:
                return "#3498db";
        }
    }
}

//每个父窗口正在显示的提示列表
QHash<QWidget *, QVector<QPointer<messageBox> > > &messageBox::activeBoxes() {
    static QHash<QWidget *, QVector<QPointer<messageBox> > > boxes;
    return boxes;
}

messageBox::messageBox(QWidget *parent, const std::string &text, const Type type, const int durationMs)
    : QWidget(parent) {
    setObjectName("messageBox");
    _isWindow = (parent == nullptr);

    int cardWidth = kCardMaxWidth;
    if (_isWindow) {
        //没有父窗口：作为独立的无边框窗口弹在桌面上
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint
                       | Qt::WindowDoesNotAcceptFocus | Qt::WindowTransparentForInput);
        setAttribute(Qt::WA_TranslucentBackground); //圆角外要透出桌面
        setAttribute(Qt::WA_ShowWithoutActivating); //别抢焦点
        setWindowOpacity(0.0);
    } else {
        //父窗口上的子控件：显式置透明，别被父窗口的 QWidget{...} 样式表糊上底色
        setStyleSheet("messageBox { background: transparent; }");
        //提示是纯展示，不要挡住父窗口上的鼠标操作
        setAttribute(Qt::WA_TransparentForMouseEvents);
        setFocusPolicy(Qt::NoFocus);

        //卡片宽度随父窗口宽度自适应，小窗口里也不会横着撑出去
        const int available = parent->width() - 2 * (kToastMargin + kShadowPad);
        cardWidth = qBound(kCardMinWidth, qMin(kCardMaxWidth, available), kCardMaxWidth);
    }

    //卡片
    _card = new QFrame(this);
    _card->setObjectName("toastCard");
    _card->setFixedWidth(cardWidth);
    _card->setStyleSheet(QString("QFrame#toastCard { background-color: %1; border-radius: 8px; }")
                                 .arg(colorOf(type)));

    //卡片阴影（靠外面的 kShadowPad 留出绘制空间）
    auto *shadow = new QGraphicsDropShadowEffect(_card);
    shadow->setBlurRadius(18);
    shadow->setOffset(0, 3);
    shadow->setColor(QColor(0, 0, 0, 70));
    _card->setGraphicsEffect(shadow);

    //文字
    _label = new QLabel(QString::fromStdString(text), _card);
    _label->setWordWrap(true);
    _label->setFixedWidth(cardWidth - 2 * kCardPadH);
    _label->setStyleSheet("QLabel { color: white; font-size: 14px; background: transparent; }");

    auto *cardLayout = new QVBoxLayout(_card);
    cardLayout->setContentsMargins(kCardPadH, kCardPadV, kCardPadH, kCardPadV);
    cardLayout->addWidget(_label);

    auto *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(kShadowPad, kShadowPad, kShadowPad, kShadowPad);
    rootLayout->addWidget(_card);

    //淡入淡出：子控件走 graphics effect，独立窗口走 windowOpacity
    if (!_isWindow) {
        _fade = new QGraphicsOpacityEffect(this);
        _fade->setOpacity(0.0);
        setGraphicsEffect(_fade);
    }

    //自动消失
    _closeTimer = new QTimer(this);
    _closeTimer->setSingleShot(true);
    connect(_closeTimer, &QTimer::timeout, this, &messageBox::dismiss);

    adjustSize();

    //登记 + 贴到右上角 + 弹出
    if (parent) {
        parent->installEventFilter(this);
    }
    //顶层提示的 key 就是 nullptr，它们自己一组堆叠
    activeBoxes()[parent].append(this);
    relayout();
    show();
    raise();

    animateOpacity(0.0, 1.0, 160, QEasingCurve::OutCubic, false);

    if (durationMs > 0) {
        _closeTimer->start(durationMs);
    }
}

messageBox::~messageBox() {
    //顶层提示的 key 是 nullptr，和构造时保持一致
    QWidget *parent = parentWidget();
    if (parent) {
        parent->removeEventFilter(this);
    }

    auto &boxes = activeBoxes()[parent];
    for (int i = 0; i < boxes.size(); i++) {
        if (boxes.at(i) == this) {
            boxes.remove(i);
            break;
        }
    }
    if (boxes.isEmpty()) {
        activeBoxes().remove(parent);
        return;
    }
    //自己消失后，让下面还活着的提示往上补位
    for (const auto &box: boxes) {
        if (box) {
            box->relayout();
            break;
        }
    }
}

void messageBox::popup(QWidget *parent, const std::string &text, const Type type, const int durationMs) {
    //parent 允许为空：那样会作为无边框窗口弹在桌面上
    new messageBox(parent, text, type, durationMs);
}

bool messageBox::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Resize) {
        relayout(); //父窗口变大变小，重新贴回右上角
    }
    return QWidget::eventFilter(watched, event);
}

void messageBox::relayout() const {
    QWidget *parent = parentWidget();

    //子控件用父窗口的矩形，独立窗口用屏幕可用区域（避开任务栏）
    QRect area;
    if (parent) {
        area = QRect(0, 0, parent->width(), parent->height());
    } else {
        const QScreen *screen = QGuiApplication::screenAt(QCursor::pos());
        if (!screen) {
            screen = QGuiApplication::primaryScreen();
        }
        area = screen ? screen->availableGeometry() : QRect(0, 0, 0, 0);
    }

    int y = area.y() + kToastMargin;
    const auto boxes = activeBoxes().value(parent);
    for (const auto &box: boxes) {
        if (!box) {
            continue;
        }
        const int x = qMax(area.x(), area.x() + area.width() - box->width() - kToastMargin);
        box->move(x, y);
        y += box->height() + kToastSpacing;
    }
}

void messageBox::animateOpacity(const qreal from, const qreal to, const int duration,
                                const QEasingCurve::Type curve, const bool deleteWhenFinished) {
    //子控件用 graphics effect 的 opacity，独立窗口用 windowOpacity
    QObject *target = _isWindow ? static_cast<QObject *>(this) : static_cast<QObject *>(_fade);
    const char *property = _isWindow ? "windowOpacity" : "opacity";

    auto *animation = new QPropertyAnimation(target, property, this);
    animation->setDuration(duration);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setEasingCurve(curve);
    if (deleteWhenFinished) {
        connect(animation, &QPropertyAnimation::finished, this, &QWidget::deleteLater);
    }
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

void messageBox::dismiss() {
    if (_closing) {
        return;
    }
    _closing = true;
    _closeTimer->stop();

    const qreal from = _isWindow ? windowOpacity() : _fade->opacity();
    animateOpacity(from, 0.0, 220, QEasingCurve::InCubic, true);
}
