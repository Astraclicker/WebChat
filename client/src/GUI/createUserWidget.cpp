#include "createUserWidget.h"
#include <QColor>
#include <QGraphicsDropShadowEffect>
#include <QVBoxLayout>

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
