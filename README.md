# cpp 聊天室

## 如何使用

### client(确保安装Qt,windows 端需要配置client/CmakeLists.txt下的Qt路径)

#### step1

```url
git clone --recurse-submodules https://github.com/Astraclicker/WebChat.git && cd WebChat/client
```

#### step2

```url
Windows

cmake -S . -B build -A x64
```

```url
Linux

cmake -S . -B build -G "Ninja"
```

#### step3

```url
cmake --build build --config Release -v
```

### server(确保安装mysqlcppconn，MySQL，Redis)

#### step1

```url
git clone --recurse-submodules https://github.com/Astraclicker/WebChat.git && cd WebChat/client
```

#### step2

```url
cmake -S . -B build -G "Ninja"
```

#### step3

```url
cmake --build build --config Release -v
```
