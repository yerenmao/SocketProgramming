# SocketProgramming Project

B11902173 陳順霙 B10705009 邱一新

## Compilation Instructions

執行的主機需要事先安裝好 `openssl` 和 `opencv`，且需要在 `Makefile` 中分別定義兩個套件 include 和 library 的路徑，目前的 `Makefile` 中的路徑為我們主機上的路徑，助教需要自行修改。

執行 

```bash
make
```

即可編譯整個專案，產生執行檔 `server_app` 和 `client_app`。
## Usage Guide

執行 server

```bash
./server_app <port>
```

可以查看 Log。

執行 client

```bash
./client_app <server_ip> <server_port> <port>
```

client 程式會提示使用者各項操作的指令，詳細請見 Demo 影片。


