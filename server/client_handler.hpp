#ifndef CLIENT_HANDLER_HPP
#define CLIENT_HANDLER_HPP

#include "server.hpp"
#include <string>
#include "threadpool.hpp"
#include "../shared/ssl.hpp"

struct ClientInfo {
    int client_id;
    int socket_fd;
    std::string ip;
    int listen_port;
    bool online;
    SSL *ssl;
};

void handle_client(SSL *client_ssl, int client_socket);

#endif // CLIENT_HANDLER_HPP
