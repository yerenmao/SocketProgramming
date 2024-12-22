#include "client.hpp"
#include "thread_handlers.hpp"
#include <iostream>
#include <string>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstdlib>

int main(int argc, char* argv[]) {
    /* 讀 terminal input */
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <my_listen_port>\n";
        return 1;
    }
    std::string server_ip = argv[1];            // server 的 ip address
    int server_port = std::atoi(argv[2]);       // server 的 port
    int my_listen_port = std::atoi(argv[3]);    // direct mode 下自己的 listen port

    /* 都寫在 client.cpp */
    Client client(server_ip, server_port, my_listen_port);
    if (!client.connect_to_server()) {      // 連到 server
        return 1;
    }

    /* 開兩個 thread：
    一個用來聽 Relay Mode，也就是從 Server 傳過來的 Message
    一個用來聽 Direct Mode，也就是直接從其他 Client 傳過來的 Message
    */
    if (!client.start_threads()) {
        return 1;
    }

    /* 跟 Server 溝通的 while true (quit 時會 break) */
    client.command_loop();

    /* clean up 包括：close file descriptors 以及 join threads */
    client.cleanup();
    return 0;
}
