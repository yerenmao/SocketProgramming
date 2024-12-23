#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <pthread.h>
#include "../shared/message.hpp"
#include "../shared/ssl.hpp"

class Client {
public:
    Client(const std::string& server_ip, int server_port, int my_listen_port);
    ~Client();

    bool connect_to_server();
    bool start_threads();
    void command_loop();
    void cleanup();
    bool is_running() const { return running; }
    int get_server_fd() const { return server_fd; }
    SSL* get_server_ssl() const { return server_ssl; }
    SSL_CTX* get_server_ctx() const {return server_ctx; }
    int get_direct_listen_fd() const { return direct_listen_fd; }

private:
    int server_fd;
    int direct_listen_fd;
    std::string server_ip;
    int server_port;
    int my_listen_port;
    SSL_CTX* client_ctx;
    SSL_CTX* server_ctx;
    SSL* server_ssl;

    pthread_t server_listener_thread;
    pthread_t direct_listener_thread;

    static bool running;
};

#endif // CLIENT_HPP
