#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP

#include "server.hpp"
#include <string>
#include "threadpool.hpp"

struct ClientInfo {
    int client_id;
    int socket_fd;
    std::string ip;
    int listen_port;
    bool online;
};

void handle_client(int client_socket);

#endif // CLIENT_HANDLER_HPP
