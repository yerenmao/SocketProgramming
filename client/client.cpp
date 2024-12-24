#include "client.hpp"
#include "thread_handlers.hpp"
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include "../shared/message.hpp"
#include "../shared/ssl.hpp"

bool Client::running = true;

Client::Client(const std::string& server_ip, int server_port, int my_listen_port)
    : server_ip(server_ip), server_port(server_port), my_listen_port(my_listen_port) {}

Client::~Client() {
    cleanup();
}

/* 連線到 Server */
bool Client::connect_to_server() {
    // 初始化 OpenSSL
    init_openssl();

    // 建立 SSL_CTX，載入 CA (用來驗證 server)
    client_ctx = create_client_context("./client/keys/ca.crt");
    if (!client_ctx) {
        std::cerr << "Failed to create SSL context\n";
        return false;
    }


    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        SSL_CTX_free(client_ctx);
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
        SSL_CTX_free(client_ctx);
        return false;
    }

    if (connect(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(server_fd);
        SSL_CTX_free(client_ctx);
        return false;
    }

    // 4) 建立 SSL 並綁定 socket
    server_ssl = SSL_new(client_ctx);
    SSL_set_fd(server_ssl, server_fd);

    // 5) Client 端握手
    if (SSL_connect(server_ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        std::cout << "SSL handshake success (Client)!\n";
    }

    std::cout << "Connected to server at " << server_ip << ":" << server_port << "\n";

    /* Server 期待 Client 一開始連線時先傳一個 JOIN Message，並告訴 Server 自己的 listen port */
    Message join_msg;
    memset(&join_msg, 0, sizeof(join_msg));
    join_msg.msg_type = JOIN;
    snprintf(join_msg.payload, MAX_PAYLOAD_SIZE, "%d", my_listen_port);
    join_msg.payload_size = (int)strlen(join_msg.payload);
    if (SSL_write(server_ssl, &join_msg, sizeof(join_msg)) < 0) {
        perror("write(JOIN)");
        SSL_shutdown(server_ssl);
        SSL_free(server_ssl);
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
    // 建立 SSL_CTX，載入憑證與私鑰
    server_ctx = create_server_context("./client/keys/server.crt", "./client/keys/server.key");
    if (!server_ctx) {
        std::cerr << "Failed to create (server)SSL context\n";
        return 1;
    }

    direct_listen_fd = create_listening_socket(server_ctx, my_listen_port);
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

        if (cmd == "whoami") {
            // Format: whoami
            if (this->logged_in) {
                std::cout << this->username << "\n";
            } else {
                std::cout << "Nobody\n";
            }
            
        } else if (cmd == "register") {
            // Format: register <username> <password>
            size_t first_space = line.find(' ');
            size_t second_space = line.find(' ', first_space + 1);
            if (first_space == std::string::npos || second_space == std::string::npos) {
                std::cout << "Usage: register <username> <password>\n";
                continue;
            }

            std::string username = line.substr(first_space + 1, second_space - first_space - 1);
            std::string password = line.substr(second_space + 1);
            register_user(username, password);
        } else if (cmd == "login") {
            // Format: login <username> <password>
            size_t first_space = line.find(' ');
            size_t second_space = line.find(' ', first_space + 1);
            if (first_space == std::string::npos || second_space == std::string::npos) {
                std::cout << "Usage: login <username> <password>\n";
                continue;
            }

            std::string username = line.substr(first_space + 1, second_space - first_space - 1);
            std::string password = line.substr(second_space + 1);
            login(username, password);
        } else if (cmd == "logout") {
            // Format: logout
            logout();
        } else if (cmd == "chat") {
            // Format: chat <to_id> <message>
            size_t first_space = line.find(' ');
            size_t second_space = line.find(' ', first_space + 1);
            if (first_space == std::string::npos || second_space == std::string::npos) {
                std::cout << "Usage: chat <to_id> <message>\n";
                continue;
            }

            int to_id = std::stoi(line.substr(first_space + 1, second_space - first_space - 1));
            std::string message = line.substr(second_space + 1);
            chat(to_id, message);
        } else if (cmd == "request_peer") {
            // Format: request_peer <to_id>
            size_t first_space = line.find(' ');
            if (first_space == std::string::npos) {
                std::cout << "Usage: request_peer <to_id>\n";
                continue;
            }

            int to_id = std::stoi(line.substr(first_space + 1));
            request_peer(to_id);
        } else if (cmd == "direct_send") {
            // direct_send <ip> <port> <message>
            // Connect directly to another client and send a message
            // This simulates what you'd do after getting PEER_INFO from server.
            size_t first_space = line.find(' ');
            size_t second_space = line.find(' ', first_space + 1);
            size_t third_space = line.find(' ', second_space + 1);
            if (first_space == std::string::npos || second_space == std::string::npos || third_space == std::string::npos) {
                std::cout << "Usage: direct_send <ip> <port> <message>\n";
                continue;
            }


            
            std::string peer_ip = line.substr(first_space + 1, second_space - first_space - 1);
            int peer_port = std::stoi(line.substr(second_space + 1, third_space - second_space - 1));
            std::string message = line.substr(third_space + 1);
            direct_send(peer_ip, peer_port, message);
        } else if (cmd == "direct_send_file") {
            // direct_send <ip> <port> <filename>
            // Connect directly to another client and send a file
            size_t first_space = line.find(' ');
            size_t second_space = line.find(' ', first_space + 1);
            size_t third_space = line.find(' ', second_space + 1);
            if (first_space == std::string::npos || second_space == std::string::npos || third_space == std::string::npos) {
                std::cout << "Usage: direct_send_file <ip> <port> <filename>\n";
                continue;
            }

            std::string peer_ip = line.substr(first_space + 1, second_space - first_space - 1);
            int peer_port = std::stoi(line.substr(second_space + 1, third_space - second_space - 1));
            std::string filename = line.substr(third_space + 1);
            direct_send_file(peer_ip, peer_port, filename);
        } else if (cmd == "relay_send_file") {
            // Format: relay_send_file <to_id> <filename>
            size_t first_space = line.find(' ');
            size_t second_space = line.find(' ', first_space + 1);
            if (first_space == std::string::npos || second_space == std::string::npos) {
                std::cout << "Usage: relay_send_file <to_id> <filename>\n";
                continue;
            }

            int to_id = std::stoi(line.substr(first_space + 1, second_space - first_space - 1));
            std::string filename = line.substr(second_space + 1);
            relay_send_file(to_id, filename);
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

void Client::login(const std::string& username, const std::string& password) {
    if (logged_in) {
        std::cout << "Already logged in as " << this->username << ".\n";
        return;
    }

    Message login_msg{};
    login_msg.msg_type = LOGIN;
    snprintf(login_msg.payload, MAX_PAYLOAD_SIZE, "%s %s", username.c_str(), password.c_str());
    login_msg.payload_size = strlen(login_msg.payload);

    if (SSL_write(server_ssl, &login_msg, sizeof(login_msg)) < 0) {
        perror("write(login)");
        return;
    }
}

void Client::successful_login(const std::string& username) {
    this->logged_in = true;
    this->username = username;
    std::cout << "Success\n";
}

void Client::logout() {
    if (!logged_in) {
        std::cout << "Not logged in.\n";
        return;
    }

    Message logout_msg{};
    logout_msg.msg_type = LOGOUT;
    logout_msg.payload_size = snprintf(logout_msg.payload, MAX_PAYLOAD_SIZE, "%s", username.c_str());

    if (SSL_write(server_ssl, &logout_msg, sizeof(logout_msg)) < 0) {
        perror("write(logout)");
        return;
    }

    // Clear client-side login state
    this->username.clear();
    this->logged_in = false;
    std::cout << "Success\n";
}

void Client::register_user(const std::string& username, const std::string& password) {
    Message register_msg{};
    register_msg.msg_type = REGISTER;
    snprintf(register_msg.payload, MAX_PAYLOAD_SIZE, "%s %s", username.c_str(), password.c_str());
    register_msg.payload_size = strlen(register_msg.payload);

    if (SSL_write(server_ssl, &register_msg, sizeof(register_msg)) < 0) {
        perror("write(register)");
        return;
    }
}

void Client::chat(int to_id, const std::string& message) {
    Message chat_msg{};
    chat_msg.msg_type = CHAT;
    chat_msg.to_id = to_id;
    std::strncpy(chat_msg.payload, message.c_str(), MAX_PAYLOAD_SIZE);
    chat_msg.payload_size = message.size();

    if (SSL_write(server_ssl, &chat_msg, sizeof(chat_msg)) < 0) {
        perror("write(chat)");
    }
}

void Client::request_peer(int to_id) {
    Message req_msg{};
    req_msg.msg_type = REQUEST_PEER;
    req_msg.to_id = to_id;

    if (SSL_write(server_ssl, &req_msg, sizeof(req_msg)) < 0) {
        perror("write(request_peer)");
    }
}

void Client::direct_send(const std::string& peer_ip, int peer_port, const std::string& message) {
    // Create socket and connect
    int peer_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (peer_fd < 0) {
        perror("socket(direct_send)");
        return;
    }

    struct sockaddr_in peer_addr;
    memset(&peer_addr, 0, sizeof(peer_addr));
    peer_addr.sin_family = AF_INET;
    peer_addr.sin_port = htons(peer_port);
    if (inet_pton(AF_INET, peer_ip.c_str(), &peer_addr.sin_addr) <= 0) {
        perror("inet_pton(direct_send)");
        close(peer_fd);
        return;
    }

    if (connect(peer_fd, (struct sockaddr*)&peer_addr, sizeof(peer_addr)) < 0) {
        perror("connect(direct_send)");
        close(peer_fd);
        return;
    }

    // 建立 SSL 並綁定 socket
    SSL* peer_ssl = SSL_new(client_ctx);
    if (!peer_ssl) {
        std::cerr << "Failed to create SSL object." << std::endl;
    }

    if (SSL_set_fd(peer_ssl, peer_fd) != 1) {
        std::cerr << "Failed to bind SSL to socket." << std::endl;
    }

    // Client 端握手
    if (SSL_connect(peer_ssl) <= 0) {
        ERR_print_errors_fp(stderr);
    } else {
        std::cout << "SSL handshake success (P2P Client)!\n";
    }

    // Send a message in the same Message format
    Message direct_msg;
    memset(&direct_msg, 0, sizeof(direct_msg));
    direct_msg.msg_type = DIRECT_MSG;
    strncpy(direct_msg.payload, message.c_str(), MAX_PAYLOAD_SIZE);
    direct_msg.payload_size = (int)strlen(direct_msg.payload);

    if (SSL_write(peer_ssl, &direct_msg, sizeof(direct_msg)) < 0) {
        perror("write(direct_msg)");
    }

    int shutdown_result = SSL_shutdown(peer_ssl);
    if (shutdown_result == 0) {
        // 對端尚未回應 close_notify，進行第二次關閉
        shutdown_result = SSL_shutdown(peer_ssl);
    }
    if (shutdown_result < 0) {
        int err_code = SSL_get_error(peer_ssl, shutdown_result);
        std::cerr << "SSL_shutdown failed, error code: " << err_code << std::endl;
        ERR_print_errors_fp(stderr); // 打印詳細錯誤資訊
    } else if (shutdown_result == 0) {
        std::cout << "SSL_shutdown incomplete, waiting for peer's close_notify." << std::endl;
    }

    SSL_free(peer_ssl);

    // 檢查釋放過程中是否有錯誤
    if (ERR_peek_error()) {
        std::cerr << "Errors occurred during SSL_free:" << std::endl;
        ERR_print_errors_fp(stderr);
    }

    close(peer_fd);
}

void direct_send_file(const std::string& peer_ip, int peer_port, const std::string& filename){
    // TODO
    return;
}

void relay_send_file(int to_id, const std::string& filename){
    // TODO
    return;
}