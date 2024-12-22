#include "client.hpp"
#include "thread_handlers.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "../shared/message.hpp"

bool Client::running = true;

Client::Client(const std::string& server_ip, int server_port, int my_listen_port)
    : server_ip(server_ip), server_port(server_port), my_listen_port(my_listen_port) {}

Client::~Client() {
    cleanup();
}

/* 連線到 Server */
bool Client::connect_to_server() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return false;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(server_fd);
        return false;
    }

    if (connect(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(server_fd);
        return false;
    }

    std::cout << "Connected to server at " << server_ip << ":" << server_port << "\n";

    /* Server 期待 Client 一開始連線時先傳一個 JOIN Message，並告訴 Server 自己的 listen port */
    Message join_msg;
    memset(&join_msg, 0, sizeof(join_msg));
    join_msg.msg_type = JOIN;
    snprintf(join_msg.payload, MAX_PAYLOAD_SIZE, "%d", my_listen_port);
    join_msg.payload_size = (int)strlen(join_msg.payload);
    if (write(server_fd, &join_msg, sizeof(join_msg)) < 0) {
        perror("write(JOIN)");
        close(server_fd);
        return false;
    }

    return true;
}

/* 開兩個 thread
create_listening_socket 會開一個用來聽 Direct Mode 的 Socket
server_listener_thread_func 是用來 handle Relay Mode 的 Thread
direct_listener_thread_func 是用來 handle Direct Mode 的 Thread
*/
bool Client::start_threads() {
    direct_listen_fd = create_listening_socket(my_listen_port);
    if (direct_listen_fd < 0) {
        std::cerr << "Failed to create listening socket.\n";
        return false;
    }

    if (pthread_create(&server_listener_thread, nullptr, server_listener_thread_func, this) != 0) {
        perror("pthread_create(server_listener)");
        return false;
    }

    if (pthread_create(&direct_listener_thread, nullptr, direct_listener_thread_func, this) != 0) {
        perror("pthread_create(direct_listener)");
        return false;
    }

    return true;
}

/* 處理 Client 的 User Input */
void Client::command_loop() {
    while (running) {
        std::string line;
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "quit") {
            running = false;
            break;
        }

        // Parse the command
        std::string cmd;
        size_t space_pos = line.find(' ');
        if (space_pos == std::string::npos) {
            cmd = line;
        } else {
            cmd = line.substr(0, space_pos);
        }

        if (cmd == "chat") {
            // Format: chat <to_id> <message>
            size_t first_space = line.find(' ');
            size_t second_space = line.find(' ', first_space + 1);
            if (first_space == std::string::npos || second_space == std::string::npos) {
                std::cout << "Usage: chat <to_id> <message>\n";
                continue;
            }

            std::string to_id_str = line.substr(first_space + 1, second_space - first_space - 1);
            std::string msg_str = line.substr(second_space + 1);

            int to_id = std::stoi(to_id_str);
            Message chat_msg{};
            chat_msg.msg_type = CHAT;
            chat_msg.to_id = to_id;
            std::strncpy(chat_msg.payload, msg_str.c_str(), MAX_PAYLOAD_SIZE);
            chat_msg.payload_size = msg_str.size();

            if (write(server_fd, &chat_msg, sizeof(chat_msg)) < 0) {
                perror("write(chat)");
            }
        } else if (cmd == "request_peer") {
            // Format: request_peer <to_id>
            size_t first_space = line.find(' ');
            if (first_space == std::string::npos) {
                std::cout << "Usage: request_peer <to_id>\n";
                continue;
            }

            std::string to_id_str = line.substr(first_space + 1);
            int to_id = std::stoi(to_id_str);

            Message req_msg{};
            req_msg.msg_type = REQUEST_PEER;
            req_msg.to_id = to_id;

            if (write(server_fd, &req_msg, sizeof(req_msg)) < 0) {
                perror("write(request_peer)");
            }
        } else if (cmd == "direct_send") {
            // direct_send <ip> <port> <message>
            // Connect directly to another client and send a message
            // This simulates what you'd do after getting PEER_INFO from server.
            int first_space = (int)line.find(' ');
            int second_space = (int)line.find(' ', first_space+1);
            int third_space = (int)line.find(' ', second_space+1);

            if (first_space == -1 || second_space == -1 || third_space == -1) {
                std::cout << "Usage: direct_send <ip> <port> <message>\n";
                continue;
            }

            std::string peer_ip = line.substr(first_space+1, second_space - first_space - 1);
            std::string port_str = line.substr(second_space+1, third_space - second_space - 1);
            int peer_port = atoi(port_str.c_str());
            std::string direct_msg_str = line.substr(third_space+1);

            // Create socket and connect
            int peer_fd = socket(AF_INET, SOCK_STREAM, 0);
            if (peer_fd < 0) {
                perror("socket(direct_send)");
                continue;
            }

            struct sockaddr_in peer_addr;
            memset(&peer_addr, 0, sizeof(peer_addr));
            peer_addr.sin_family = AF_INET;
            peer_addr.sin_port = htons(peer_port);
            if (inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr) <= 0) {
                perror("inet_pton(direct_send)");
                close(peer_fd);
                continue;
            }

            if (connect(peer_fd, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
                perror("connect(direct_send)");
                close(peer_fd);
                continue;
            }

            // Send a message in the same Message format
            Message direct_msg;
            memset(&direct_msg, 0, sizeof(direct_msg));
            direct_msg.msg_type = DIRECT_MSG;
            strncpy(direct_msg.payload, direct_msg_str.c_str(), MAX_PAYLOAD_SIZE);
            direct_msg.payload_size = (int)strlen(direct_msg.payload);

            if (write(peer_fd, &direct_msg, sizeof(direct_msg)) < 0) {
                perror("write(direct_msg)");
            }
            close(peer_fd);
        } else {
            std::cout << "Unknown command: " << cmd << "\n";
        }
    }
}

void Client::cleanup() {
    running = false;
    close(server_fd);
    close(direct_listen_fd);

    pthread_join(server_listener_thread, nullptr);
    pthread_join(direct_listener_thread, nullptr);
}
