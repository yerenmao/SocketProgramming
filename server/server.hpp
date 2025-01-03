#ifndef SERVER_HPP
#define SERVER_HPP

#include "threadpool.hpp"
#include "client_handler.hpp"
#include "../shared/ssl.hpp"
#include <string>

class Server {
public:
    Server(int port, int max_clients, int worker_count);
    ~Server();

    void start();

private:
    int server_fd;
    int port;
    int max_clients;

    ThreadPool thread_pool;

    SSL_CTX* ctx;

    void accept_clients();
};

#endif // SERVER_HPP
