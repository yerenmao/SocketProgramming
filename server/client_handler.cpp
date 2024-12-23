#include "client_handler.hpp"

#include <iostream>
#include <sstream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <map>
#include <pthread.h>
#include "../shared/message.hpp"
#include "authentication.hpp"

/* 管理 Client 資訊*/
static std::map<int, ClientInfo> clients;                           // 用於把 client_id 對應到 ClientInfo
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;   // client 的 mutex lock
static int next_client_id = 1;

void handle_message(int client_socket, const Message& msg);
void send_message(int socket_fd, const Message& msg);
bool get_client_info(int client_id, std::string& ip, int& port);
void handle_authentication(const Message& response);

void handle_client(int client_socket) {
    Message msg;
    int bytes_read = read(client_socket, &msg, sizeof(msg));
    if (bytes_read <= 0) {
        close(client_socket);
        return;
    }

    /* 預期使用者的第一個 Message 是 JOIN
    否則直接把 connection close 掉
    */
    if (msg.msg_type == JOIN) {
        int listen_port = atoi(msg.payload);
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);
        getpeername(client_socket, (struct sockaddr*)&addr, &len);
        std::string client_ip = inet_ntoa(addr.sin_addr);

        int assigned_id = -1;
        pthread_mutex_lock(&clients_mutex);
        assigned_id = next_client_id++;
        clients[assigned_id] = {assigned_id, client_socket, client_ip, listen_port, true};
        pthread_mutex_unlock(&clients_mutex);

        std::cout << "Client joined: ID=" << assigned_id << ", IP=" << client_ip
                  << ", Port=" << listen_port << std::endl;
    } else {
        close(client_socket);
        return;
    }

    /* 與 connection 對面的 client 對話 */
    while (true) {
        int r = read(client_socket, &msg, sizeof(msg));
        if (r <= 0) {
            pthread_mutex_lock(&clients_mutex);
            for (auto& kv : clients) {
                if (kv.second.socket_fd == client_socket) {
                    kv.second.online = false;
                    break;
                }
            }
            pthread_mutex_unlock(&clients_mutex);
            close(client_socket);
            return;
        }

        handle_message(client_socket, msg);
    }
}

void register_client(int client_socket, const std::string& ip, int listen_port, int& assigned_id) {
    pthread_mutex_lock(&clients_mutex);
    assigned_id = next_client_id++;
    clients[assigned_id] = {assigned_id, client_socket, ip, listen_port, true};
    pthread_mutex_unlock(&clients_mutex);
}

void handle_message(int client_socket, const Message& msg) {
    switch (msg.msg_type) {
        case REGISTER: {
            // Extract username and password from payload
            std::istringstream payload_stream(msg.payload);
            std::string username, password;
            payload_stream >> username >> password;

            AuthResult result = Authentication::register_user(username, password);
            std::cout << "[register] " << username << " " << password  << " " << auth_result_to_string(result) << std::endl;
            
            // Send response back to client
            Message response{};
            response.msg_type = RESPONSE;
            response.payload_size = snprintf(response.payload, MAX_PAYLOAD_SIZE, "%s", auth_result_to_string(result).c_str());
            send_message(client_socket, response);
            break;
        }

        case LOGIN: {
            // Extract username and password from payload
            std::istringstream payload_stream(msg.payload);
            std::string username, password;
            payload_stream >> username >> password;

            AuthResult result = Authentication::login_user(username, password);
            std::cout << "[login] " << username << " " << password  << " " << auth_result_to_string(result) << std::endl;

            // Send response back to client
            Message response{};
            if (result == AuthResult::Success) {
                response.msg_type = LOGIN;
                response.payload_size = snprintf(response.payload, MAX_PAYLOAD_SIZE, "%s", username.c_str());
            } else {    
                response.msg_type = RESPONSE;
                response.payload_size = snprintf(response.payload, MAX_PAYLOAD_SIZE, "%s", auth_result_to_string(result).c_str());
            }
            send_message(client_socket, response);
            
            break;
        }

        case LOGOUT: {
            // Extract username from payload
            std::string username(msg.payload, msg.payload_size);
            Authentication::logout_user(username);
            std::cout << "[LOGOUT] " << username << " " << auth_result_to_string(AuthResult::Success) << std::endl;
            break;
        }

        case CHAT: {
            // Relay mode: Find recipient socket and forward the message
            pthread_mutex_lock(&clients_mutex);
            auto it = clients.find(msg.to_id);
            if (it != clients.end() && it->second.online) {
                send_message(it->second.socket_fd, msg);
            } else {
                // Recipient not online - optionally send error back
                std::cerr << "Recipient not online.\n";
            }
            pthread_mutex_unlock(&clients_mutex);
            break;
        }

        case REQUEST_PEER: {
            // Client wants peer info to establish a direct connection
            std::string peer_ip;
            int peer_port;
            Message resp;
            memset(&resp, 0, sizeof(resp));
            resp.msg_type = PEER_INFO;
            resp.from_id = 0; // Server
            resp.to_id = msg.from_id; // Send back to the requester

            if (get_client_info(msg.to_id, peer_ip, peer_port)) {
                snprintf(resp.payload, MAX_PAYLOAD_SIZE, "%s:%d", peer_ip.c_str(), peer_port);
                resp.payload_size = strlen(resp.payload);
            } else {
                snprintf(resp.payload, MAX_PAYLOAD_SIZE, "NOT_FOUND");
                resp.payload_size = strlen(resp.payload);
            }
            send_message(client_socket, resp);
            break;
        }

        case DIRECT_MSG: {
            // Direct message handling if routed through the server
            // Typically, direct mode will bypass the server and use P2P.
            std::cerr << "Received DIRECT_MSG: " << msg.payload << std::endl;
            break;
        }

        default:
            std::cerr << "Unknown message type: " << msg.msg_type << std::endl;
    }
}

bool get_client_info(int client_id, std::string& ip, int& port) {
    pthread_mutex_lock(&clients_mutex);
    auto it = clients.find(client_id);
    if (it != clients.end() && it->second.online) {
        ip = it->second.ip;
        port = it->second.listen_port;
        pthread_mutex_unlock(&clients_mutex);
        return true;
    }
    pthread_mutex_unlock(&clients_mutex);
    return false;
}

void send_message(int socket_fd, const Message& msg) {
    // For simplicity, do a blocking write
    if (write(socket_fd, &msg, sizeof(msg)) < 0) {
        perror("send_message failed");
    }
}
