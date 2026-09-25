# WebChat

一个使用 C++20、Boost.Asio、Qt 6 和 MySQL 编写的练习型聊天室。

```text
Qt 客户端
    │ TCP 127.0.0.1:9191
    ▼
聊天服务端
    ├─ MySQL Connector/C++ ──→ MySQL 127.0.0.1:3306
    │                            ├─ ChatServer.users        账号
    │                            └─ ChatServer.chat_history 聊天记录
    └─ Redis++ ──────────────→ Redis 127.0.0.1:6379
                                 └─ login_cache:<用户名>    登录缓存
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

当前构建使用系统安装的 Boost，服务端数据链路只构建 MySQL 模块，客户端也没有接入 SQL++。因此，编译和运行当前程序只需要克隆主仓库，不必下载仓库中的子模块。

```bash
git clone https://github.com/anarchycuriosity/web_chat_test.git WebChat
cd WebChat
```

仓库仍保留了 Redis++、hiredis 和 SQLiteCpp 的封装源码，供后续扩展和阅读。如果需要阅读这些模块并获得完整的 IntelliSense 跳转，再按需初始化对应子模块：

```bash
git submodule update --init --recursive -- \
    client/lib/SQL++/lib/SQLiteCpp \
    client/lib/SQL++/lib/hiredis \
    client/lib/SQL++/lib/redis-plus-plus \
    server/lib/SQL++/lib/SQLiteCpp \
    server/lib/SQL++/lib/hiredis \
    server/lib/SQL++/lib/redis-plus-plus
```

上述命令不会下载 `client/lib/boost` 和 `server/lib/boost`。如果主仓库已经克隆完成，也不需要重新克隆；进入仓库根目录后单独执行该命令即可。如果你正在本仓库的未推送工作树中验证修改，也不要重新克隆远端旧版本。

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
    qt6-base-dev \
    redis-server \
    redis-tools
```

确认关键工具版本：

```bash
cmake --version
g++ --version
```

项目最低要求 CMake 3.28。Ubuntu 24.04 官方仓库提供的 3.28.3 已经足够，不需要另外安装 CMake 4.x。

### 3. 生成编译数据库并配置 IntelliSense

在仓库根目录先为客户端和服务端生成 `compile_commands.json`：

```bash
cmake -S client -B client/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

cmake -S server -B server/build \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
```

生成结果分别位于：

```text
client/build/compile_commands.json
server/build/compile_commands.json
```

`.vscode/c_cpp_properties.json` 中的 `compileCommands` 已指向这两个文件。真正加入 CMake target 的 `.cpp` 会使用编译数据库中的精确参数；没有加入 target 的源码不会凭空产生编译条目。

例如，当前服务端只在 `server/lib/SQL++/CMakeLists.txt` 中加入了 `MySQL++`，没有加入 `Redis++`。因此，打开 `server/lib/SQL++/Redis++/Redispp.cpp` 时，C/C++ 扩展会回退到 `c_cpp_properties.json` 的基础配置。若只想阅读这部分代码而不改变实际构建目标，请在现有 `WSL` 配置中保留 `compileCommands`，并追加：

```json
"compileCommands": [
  "${workspaceFolder}/client/build/compile_commands.json",
  "${workspaceFolder}/server/build/compile_commands.json"
],
"includePath": [
  "${workspaceFolder}/server/lib/SQL++/lib/redis-plus-plus/src",
  "${workspaceFolder}/server/lib/SQL++/lib/redis-plus-plus/src/sw/redis++/no_tls",
  "${workspaceFolder}/server/lib/SQL++/lib/redis-plus-plus/src/sw/redis++/cxx17",
  "${workspaceFolder}/server/lib/SQL++/lib",
  "${workspaceFolder}/client/build/SQL++/redis-plus-plus/src"
]
```

`includePath` 不是“包含源码的目录清单”，而是一组头文件搜索起点。可以先用下面这个近似模型理解一次查找：

```text
候选文件 = includePath 中的某个起点 + #include 中写出的路径
```

例如，`${workspaceFolder}/server/lib/SQL++/lib` 并不能直接找到 `#include <sw/redis++/redis++.h>`，因为把两段路径拼起来会得到不存在的 `server/lib/SQL++/lib/sw/redis++/redis++.h`。真正的文件中间还隔着 `redis-plus-plus/src`，所以搜索起点必须下沉到该 `src` 目录。

这几条路径分别对应 Redis++ 源码中真实存在的包含关系：

| 源码中的 `#include` | 搜索起点 | 最终命中的文件 | 为什么单独列出 |
| --- | --- | --- | --- |
| `<sw/redis++/redis++.h>` | `redis-plus-plus/src` | `src/sw/redis++/redis++.h` | Redis++ 普通公开头文件的根目录 |
| `"sw/redis++/tls.h"` | `redis-plus-plus/src/sw/redis++/no_tls` | `no_tls/sw/redis++/tls.h` | 当前配置明确选择“无 TLS”实现；不能同时模糊搜索 `tls` 和 `no_tls` |
| `"sw/redis++/cxx_utils.h"` | `redis-plus-plus/src/sw/redis++/cxx17` | `cxx17/sw/redis++/cxx_utils.h` | 当前配置明确选择 C++17 版本；同级还可能存在 C++11 版本 |
| `<hiredis/hiredis.h>` | `server/lib/SQL++/lib` | `lib/hiredis/hiredis.h` | `#include` 本身已经带有 `hiredis/`，所以起点停在它的父目录 `lib` |
| `"sw/redis++/hiredis_features.h"` | `client/build/SQL++/redis-plus-plus/src` | 构建目录中的同名头文件 | 该文件不是仓库源码，而是 CMake 检查 hiredis 能力后生成的配置头文件 |

因此，不是每个 `.cpp` 或 `.h` 所在目录都要加入 `includePath`。只需加入能够让实际 `#include` 路径正确落到目标头文件上的搜索根，以及构建配置选中的覆盖目录。`${workspaceFolder}/server/lib/SQL++/lib/hiredis` 对当前包含关系是重复项：`lib` 已经可以解析 `<hiredis/hiredis.h>`，所以这里不再列出它。

虽然可以写 `${workspaceFolder}/server/lib/SQL++/lib/**` 让扩展递归收集目录，但不建议用于这里。Redis++ 同时保存了 `tls/no_tls`、`cxx11/cxx17` 等互斥实现；递归加入会模糊本应由构建配置作出的选择，还会扫描测试和示例目录。显式列出搜索根更接近真实编译命令。

最后一项提供 Redis++ 由 CMake 生成的 `hiredis_features.h`。它可以复用，是因为本仓库客户端与服务端的 `redis-plus-plus` 子模块固定在同一提交；若以后两边版本不同，应为服务端 Redis++ 单独生成对应配置，不能继续混用。

保存配置后，在 VS Code 命令面板依次执行：

1. `C/C++: Reset IntelliSense Database`
2. `Developer: Reload Window`

不要为了消除编辑器红线而把 `Redis++` 加进服务端 CMake；那会改变真实构建图、依赖和链接结果，不再只是 IntelliSense 配置。

### 4. 启动并初始化 MySQL

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

只需保证 `ChatServer` 这个库存在。`users`（账号）和 `chat_history`（聊天记录）两张表都由服务端在客户端连接时用
`create table if not exists` 自动建立，这里手写 `CREATE TABLE` 只是为了让库一开始就可见；如果不写，服务端首次接受连接时也会补上。

`chat_history` 没有出现在上面的 SQL 里，它完全由服务端代码维护，定义在 `session::session()` 中。

### 5. 编译服务端

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

### 6. 编译客户端

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

### 7. 启动服务端

打开第一个终端，进入仓库根目录：

#### 使用systemctl启动redis服务器,redis健康检查 
```bash
sudo systemctl enable --now redis-server
redis cli ping
```
#### 若wsl2没有systemmd
```bash
sudo service redis-server start
redis-cli ping
```

#### 运行服务器

因为组长硬编码了,所以就不使用环境变量了

```bash
sudo mysql <<'SQL'
CREATE DATABASE IF NOT EXISTS ChatServer
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_unicode_ci;

CREATE USER IF NOT EXISTS 'astraclicker'@'127.0.0.1'
    IDENTIFIED WITH mysql_native_password BY '1108372699a@A';
ALTER USER 'astraclicker'@'127.0.0.1'
    IDENTIFIED WITH mysql_native_password BY '1108372699a@A';
GRANT ALL PRIVILEGES ON ChatServer.* TO 'astraclicker'@'127.0.0.1';

CREATE USER IF NOT EXISTS 'astraclicker'@'localhost'
    IDENTIFIED WITH mysql_native_password BY '1108372699a@A';
ALTER USER 'astraclicker'@'localhost'
    IDENTIFIED WITH mysql_native_password BY '1108372699a@A';
GRANT ALL PRIVILEGES ON ChatServer.* TO 'astraclicker'@'localhost';

FLUSH PRIVILEGES;
SQL
```

服务端**必须传入配置文件路径**（`server/main.cpp` 会检查 `argv[1]`，不传会直接抛 `must provide config file`）。

```bash
cat > server/config.json <<'JSON'
{
  "Web": { "port": 9191 },
  "MySQL": {
    "address": "127.0.0.1",
    "port": 3306,
    "userName": "astraclicker",
    "password": "把上面 CREATE USER 里设的密码填在这里"
  },
  "Redis": {
    "address": "127.0.0.1",
    "port": 6379,
    "userName": "",
    "password": ""
  }
}
JSON

./server/bin/Release/server server/config.json
```

`userName` / `password` 要和上面 `CREATE USER` 建立的那个 MySQL 账号一致，否则服务端会在建立连接时报
`Access denied`。**这个文件里是明文密码，不要提交到仓库**（已加进 `.gitignore`）。

`Web.port` 和 `MySQL.port` 必须是**数字**，不能写成 `"9191"`：服务端用 `config["Web"]["port"]` 直接构造
`tcp::endpoint`，JSON 字符串无法隐式转成整数。
本机 Redis 没有设密码，所以 `Redis.userName` 和 `Redis.password` 留空即可；`Redispp` 只有在 `userName`
非空时才会执行 `AUTH`。

看到下面这行表示服务端已经监听成功：

```text
Chat server started on port 9191
```

### 8. 启动客户端

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

### 9. 验证聊天记录已落库

服务端收到 `text` 消息时，除了广播给其他会话，还会把这条记录写进 `ChatServer.chat_history`。

表结构（由服务端自动创建）：

| 字段 | 类型 | 说明 |
| --- | --- | --- |
| `uid` | `int` | 主键，自增 |
| `sender` | `varchar(50)` | 发送方，取自服务端会话里登录成功后的用户名，**不由客户端传入** |
| `receiver` | `varchar(50)` | 接收方，来自客户端消息体的 `receiver` |
| `text` | `text` | 消息正文 |
| `sendTime` | `datetime` | **服务端**写库时生成的时间戳，不接受客户端传入 |

在客户端界面里发几条消息，然后直接查库（**命令写成一行，不要用反斜杠断开**：反斜杠后面
一旦多出空格，续行就会失效，命令被拆成两条，报错信息会指向 `-e: not found` 这种完全无关的地方）：

```bash
mysql -h 127.0.0.1 -u astraclicker -p ChatServer -e "select uid,sender,receiver,text,sendTime from chat_history order by uid;"
```

回车后会提示 `Enter password:`，**输入时屏幕上一个字符都不会显示，这是正常的**，打完直接回车。

预期能看到刚发出的消息，且 `sendTime` 是当前时间。

`sendTime` 由服务端生成而不是由客户端传入，是因为客户端时钟不可信，记录的落地时间不该由发送方决定。

## 常见问题

### IntelliSense 提示找不到 `sw/redis++/redis++.h`

先检查 redis-plus-plus 子模块是否已经初始化：

```bash
git submodule status -- \
    client/lib/SQL++/lib/redis-plus-plus \
    server/lib/SQL++/lib/redis-plus-plus
```

如果输出行以 `-` 开头，表示主仓库记录了子模块版本，但这部分可选源码尚未下载。回到第 1 步执行列出的 `git submodule update` 命令补全源码。

如果头文件已经存在但仍有红线，还要区分源码是否进入了 CMake target。客户端 Redis++ 已参与客户端配置，可以从 `client/build/compile_commands.json` 获得精确参数；服务端 Redis++ 当前没有参与服务端构建，必须使用第 3 步中的 `includePath` 回退配置。完成后再执行 `C/C++: Reset IntelliSense Database`。

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

这表示服务端已经连到 MySQL，但数据库账号、来源地址或密码不匹配。重新执行第 4 步，并确认启动服务端的同一个终端里存在正确变量：

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

聊天记录落库部分的已知边界：

- **只写不读**。本轮只把消息写入 `chat_history`，还没有查询接口；客户端 `talk_history/<用户名>.json`
  仍是独立的本地记录，两者目前互不影响。
- **落库失败不影响聊天**。写库异常会被捕获并记进服务端日志（`save chat history failed`），
  消息照常广播，连接也不会断，服务端也不会崩。已知会触发的场景：消息正文超过 65535 字节
  （`Data too long for column 'text'`）、`receiver` 超过 50 字符、表被删掉、MySQL 连接中断。
- **MySQL 连接断了不会自动重连**。每个会话在建立时创建一个 MySQL 连接并一直用；如果连接中途断掉
  （数据库重启、超时），该会话之后的消息都写不进去，重启客户端重连才能恢复。这是服务端现有的
  连接管理方式，登录和注册走的是同一条路。
- **表在客户端连接时创建**。建表语句写在 `session::session()` 里，所以如果运行期间表被外部删掉，
  要等下一个新连接才会重建。
- **未登录的会话不写库**。`sender` 取自会话登录后的用户名；未登录就发 `text` 的会话没有身份可记，
  会跳过写库，避免产生 `sender` 为空的记录。
- **`receiver` 只是原样记录，不参与投递**。服务端目前把消息广播给除自己外的**所有**会话，不看
  `receiver`；客户端也固定发 `root`（`mainWidget.cpp` 里标了 `TODO 选择接收者`）。
  所以查库时**不要假设 `receiver` 就是实际收件人**。空字符串的正文也照原样入库，不做校验。
- **没有 `(sender, receiver)` 索引**。`SQL++` 的 `mysqlCreateTable` 只支持主键和唯一键，
  加不了普通索引；将来做「拉取某人聊天记录」时需要先扩展这个接口。
