# WebChat

一个使用 C++20、Boost.Asio、Qt 6 和 MySQL 编写的练习型聊天室。

```text
Qt 客户端
    │ TCP 127.0.0.1:9191
    ▼
聊天服务端
    │ MySQL Connector/C++
    ▼
MySQL 127.0.0.1:3306 / ChatServer.users
```

本文档的 Ubuntu/WSL2 流程已在以下环境完成“干净配置、编译、启动、协议层注册与登录”验证：

- Ubuntu 24.04
- CMake 3.28.3
- GCC 13.3
- Qt 6.4.2
- Boost 1.83
- MySQL Connector/C++ 1.1.12
- MySQL 8.0

> 当前程序仅适合本地学习：密码仍以明文写入数据库，不要部署到公网或保存真实密码。

## 从零开始：Ubuntu 24.04 / WSL2

### 1. 克隆仓库

当前构建使用系统安装的 Boost，且服务端只构建实际使用的 MySQL 模块，因此不需要下载体积很大的递归子模块。

```bash
git clone https://github.com/anarchycuriosity/web_chat_test.git WebChat
cd WebChat
```

如果你正在本仓库的未推送工作树中验证修改，请不要重新克隆远端旧版本；保持在当前仓库根目录，直接从第 2 步开始。

### 2. 安装编译和运行依赖

```bash
sudo apt update
sudo apt install -y \
    build-essential \
    cmake \
    ninja-build \
    libboost-dev \
    libmysqlcppconn-dev \
    mysql-server \
    qt6-base-dev
```

确认关键工具版本：

```bash
cmake --version
g++ --version
```

项目最低要求 CMake 3.28。Ubuntu 24.04 官方仓库提供的 3.28.3 已经足够，不需要另外安装 CMake 4.x。

### 3. 启动并初始化 MySQL

先启动 MySQL：

```bash
sudo systemctl enable --now mysql
```

如果你的 WSL 没有启用 systemd，改用：

```bash
sudo service mysql start
```

创建本地开发账号、数据库和用户表。下面的密码只用于本机练习；你可以修改，但后面的环境变量必须保持一致。

```bash
sudo mysql <<'SQL'
CREATE DATABASE IF NOT EXISTS ChatServer
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

CREATE USER IF NOT EXISTS 'webchat'@'127.0.0.1'
    IDENTIFIED WITH mysql_native_password BY 'webchat_dev_password';
ALTER USER 'webchat'@'127.0.0.1'
    IDENTIFIED WITH mysql_native_password BY 'webchat_dev_password';
GRANT ALL PRIVILEGES ON ChatServer.* TO 'webchat'@'127.0.0.1';

CREATE TABLE IF NOT EXISTS ChatServer.users
(
    userName VARCHAR(64) PRIMARY KEY,
    password VARCHAR(255) NOT NULL
);
SQL
```

### 4. 编译服务端

在仓库根目录执行：

```bash
cmake -S server -B server/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release
cmake --build server/build --parallel
```

成功后生成：

```text
server/bin/Release/server
```

### 5. 编译客户端

仍在仓库根目录执行：

```bash
cmake -S client -B client/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release
cmake --build client/build --parallel
```

成功后生成：

```text
client/bin/Release/client
```

### 6. 启动服务端

打开第一个终端，进入仓库根目录：

```bash
export WEBCHAT_DB_HOST='127.0.0.1'
export WEBCHAT_DB_PORT='3306'
export WEBCHAT_DB_USER='webchat'
export WEBCHAT_DB_PASSWORD='webchat_dev_password'
export WEBCHAT_DB_NAME='ChatServer'

./server/bin/Release/server
```

看到下面这行表示服务端已经监听成功：

```text
Chat server started on port 9191
```

这些环境变量只对当前终端有效，关闭终端后不会污染系统配置。

### 7. 启动客户端

打开第二个终端，进入同一个仓库根目录：

```bash
./client/bin/Release/client
```

客户端固定连接 `127.0.0.1:9191`。先在登录窗口创建用户，再使用同一组用户名和密码登录。

WSL2 运行图形界面需要 WSLg。可以先检查：

```bash
printf '%s\n' "$DISPLAY"
```

若输出为空，说明当前 WSL 没有可用的图形显示环境；请启用 WSLg，或在原生 Linux 桌面环境运行客户端。

## 常见问题

### `apt` 找不到更高版本的 CMake

这是旧版项目配置造成的误导。项目原先错误地声明需要 CMake 4.2/4.2.3，但实际没有使用 4.x 功能；现在最低版本已改为 3.28。Ubuntu 24.04 直接安装官方包即可：

```bash
sudo apt install cmake
```

不要为了这个项目混用 `snap`、`pip` 和手工覆盖 `/usr/bin/cmake`，否则以后更难判断究竟调用了哪一套 CMake。

### CMake 仍然报告旧的 4.2 版本要求

先确认自己位于新克隆的仓库中，再清除旧配置缓存并重新配置：

```bash
cmake --fresh -S server -B server/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release
```

客户端同理，把 `server` 换成 `client`。

### `Could NOT find Boost`

```bash
sudo apt install libboost-dev
```

### `未找到 MySQL Connector/C++`

```bash
sudo apt install libmysqlcppconn-dev
```

### `Access denied for user 'webchat'`

这表示服务端已经连到 MySQL，但数据库账号、来源地址或密码不匹配。重新执行第 3 步，并确认启动服务端的同一个终端里存在正确变量：

```bash
printenv WEBCHAT_DB_HOST
printenv WEBCHAT_DB_USER
printenv WEBCHAT_DB_NAME
```

不要打印 `WEBCHAT_DB_PASSWORD`，避免把密码留在终端截图或日志里。

### 客户端提示连接失败

先确认服务端终端没有退出，再检查 9191 端口：

```bash
ss -ltn | grep ':9191'
```

如果没有结果，说明服务端尚未启动成功；优先查看服务端终端打印的错误，而不是反复重启客户端。

### 输出连接成功后仍然立即“段错误”

先确认已经重新编译当前源码，而不是继续运行修复前生成的旧程序：

```bash
cmake --build client/build --parallel
```

旧版本曾在 WSLg/Wayland 下直接使用“鼠标所在屏幕”的查询结果；该查询允许返回空值，随后访问空对象会导致段错误。当前版本已经增加主屏幕和默认尺寸两层兜底。

## 已知边界

- 当前地址和聊天端口固定为 `127.0.0.1:9191`，只适合本机测试。
- 当前登录密码按明文保存，仅用于理解客户端、异步网络和数据库调用链。
- 本 README 的完整流程验证目标是 Ubuntu 24.04/WSL2；Windows 构建尚未纳入本轮验证。
