# SocketProgramming Project


## Team Member

| 姓名      | 學號   |
| --------- | ------ |
| B11902173 | 陳順霙 |
| B10705009 | 邱一新 |


## Environment

兩個人都是使用 MacOS ，因此整份 Project 都是在 MacOS 上開發。


## Compilation Instructions

- 我們的環境是在 MacOS 上，並且使用 `brew` 做為 Package Manager，如果助教是在不同環境需要另外修改。

執行的主機需要事先安裝好 `openssl` 和 `opencv`，指令如下：

```
brew install openssl@3
brew install opencv
```

`brew` 下載下來的 package 理論上會放在 `/opt/homebrew/opt/`，因此在 `Makefile` 可以看到以下內容：

```
# 用於 include openssl 相關的 .h 檔案
-I/opt/homebrew/opt/openssl@3/include
# 用於告訴 linker 哪裡找 openssl 的 shared/static library
-L/opt/homebrew/opt/openssl@3/lib
# 把 library link 進來
-lssl -lcrypto

# 用於 include opencv 相關的 .h 檔案
-I/opt/homebrew/opt/opencv@4/include/opencv4
# 用於告訴 linker 哪裡找 opencv 的 shared/static library
-L/opt/homebrew/opt/opencv@4/lib
# 把 library link 進來
-lopencv_videoio -lopencv_core -lopencv_imgcodecs -lopencv_highgui -lopencv_imgproc
```

如果助教是在不同環境需要自行修改 `Makefile` 中兩個 package 的 include 和 library 路徑。

- 執行以下 `make` 指令：

```bash
make
```

即可編譯整個專案，產生執行檔 `server_app` 和 `client_app`。


## Usage Guide

### Execute
執行 `server_app` 執行檔：
```bash
./server_app <server_port>
```

> `server_port` 為服務開在的 port

執行 `client_app` 執行檔：

```bash
./client_app <server_ip> <server_port> <my_listen_port>
```

> `server_ip` 為 server 的 IP address
> `server_port` 為 server 的 port number
> `my_listen_port` 為 Direct Mode 下，要用哪個 port 來接收

### Help

在「未登入」的狀態下，使用 `help` 可以查看 help message：
```
====================================================
        Welcome to CSIE chatroom        
Register an account
   --> Enter: register <username> <password>
Ready to login
   --> Enter: login <username> <password>
quit
   --> Enter: quit
====================================================
```
- 使用 `register <username> <password>` 可以註冊
- 使用 `login <username> <password>` 可以登入已註冊帳號
- 使用 `quit` 可以結束程式

在「已登入」的情況下，使用 `help` 可以查看 help message：
```
====================================================
                  🎉🎉Hi <username>!!!🎉🎉
Welcome to CSIE Chatroom 
You can now chat with your friends with the
following command.
 
---------------------Relay mode:--------------------
Chat             --> chat <id> <message> 
Send file        --> relay_send_file <id> <filename> 
Video streaming  --> relay_video_streaming <id> <video_filename> 
Audio streaming  ❌❌ relay_audio_streaming <id> <audio_filename> 
Webcam streaming --> relay_webcam_streaming <id> 
 
--------------------Direct mode:--------------------
Chat             --> direct_send <ip> <port> <message> 
Send file        --> direct_send_file <ip> <port> <filename> 
Video streaming  --> direct_video_streaming <ip> <port> <video_filename> 
Audio streaming  --> direct_audio_streaming <ip> <port> <audio_filename> 
Webcam streaming --> direct_webcam_streaming <ip> <port>

--------------------Get Information:-----------------
Type "help" to get information
Type "receive_streaming" to receive video or webCam streaming!!
====================================================
Online user:
<username> ID: <uid> location: <ip>:<port>
====================================================
```

### Receive Streaming
- 在接收 Video / Webcam streaming 前需要指行以下指令：
```
receive_streaming
```

### Relay Mode
- `chat <id> <message>` 傳送訊息
- `relay_send_file <id> <filename>` 傳送檔案
- `relay_video_streaming <id> <video_filename>` 串流影像
- `relay_audio_streaming <id> <audio_filename>` 串流音訊 (此功能目前尚未正常運作，aka 找不到 bug)
- `relay_webcam_streaming <id>` Bonus 功能，webcam 的串流

### Direct Mode
- `direct_send <ip> <port> <message>` 傳送訊息
- `direct_send_file <ip> <port> <filename>` 傳送檔案
- `direct_video_streaming <ip> <port> <video_filename>` 串流影像
- `direct_audio_streaming <ip> <port> <audio_filename>` 串流音訊 (此功能目前音訊效果很差，有很多雜音)
- `direct_webcam_streaming <ip> <port>` Bonus 功能，webcam 的串流

## Demo Video

:link: **[Demo Video](https://youtu.be/FHB96ALy-PY)**
