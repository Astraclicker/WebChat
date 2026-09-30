#pragma once
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <string>

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
