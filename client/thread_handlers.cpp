#include "thread_handlers.hpp"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include "client.hpp"
#include "../shared/message.hpp"

/* 此 function 會開一個 socket 並聽在給定的 port (client 會傳 my_listen_port) */
int create_listening_socket(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket(listen)");
        return -1;
    }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind(listen)");
        close(fd);
        return -1;
    }

    if (listen(fd, 5) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

/* 此 function 會聽 Relay Mode 的訊息 */
void* server_listener_thread_func(void* arg) {
    Client* client = static_cast<Client*>(arg);

    while (client->is_running()) {
        Message msg;
        int r = read(client->get_server_fd(), &msg, sizeof(msg));
        if (r <= 0) {
            std::cerr << "Disconnected from server.\n";
            break;
        }

        switch (msg.msg_type) {
            case CHAT:
                std::cout << "Received chat: " << msg.payload << "\n";
                break;
            case PEER_INFO:
                std::cout << "Peer info: " << msg.payload << "\n";
                break;
            default:
                std::cout << "Unknown message type: " << msg.msg_type << "\n";
        }
    }

    return nullptr;
}

/* 此 function 會聽 Direct Mode 的訊息 */
void* direct_listener_thread_func(void* arg) {
    Client* client = static_cast<Client*>(arg);

    while (client->is_running()) {
        struct sockaddr_in peer_addr;
        socklen_t len = sizeof(peer_addr);
        int peer_fd = accept(client->get_direct_listen_fd(), (struct sockaddr*)&peer_addr, &len);
        if (peer_fd < 0 && client->is_running()) {
            perror("accept");
            continue;
        }

        Message msg;
        int r = read(peer_fd, &msg, sizeof(msg));
        if (r <= 0) {
            close(peer_fd);
            continue;
        }

        if (msg.msg_type == DIRECT_MSG) {
            std::cout << "Direct message received: " << msg.payload << "\n";
        }

        close(peer_fd);
    }

    return nullptr;
}
