#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include "../shared/message.hpp"
#include "../shared/ssl.hpp"
#include "../shared/streaming.hpp"

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
    void successful_login(const std::string& username);
    StreamingQueue& get_streaming_queue();

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

    std::string username; // Store the logged-in user's username
    bool logged_in = false; // Track login state
    void login(const std::string& username, const std::string& password);
    void logout();
    void register_user(const std::string& username, const std::string& password);
    void chat(int to_id, const std::string& message);
    void request_peer();
    void direct_send(const std::string& peer_ip, int peer_port, const std::string& message);

    /* Transfer file feature */
    void direct_send_file(const std::string& peer_ip, int peer_port, const std::string& filename);
    void relay_send_file(int to_id, const std::string& filename);

    /* Streaming feature */
    StreamingQueue streaming_queue;
    void direct_streaming(const std::string& peer_ip, int peer_port, const std::string& filename);
    void relay_streaming(int to_id, const std::string& filename);
    void receive_streaming();
    void welcome_message(bool& first);

    SSL* ssl_connect(const std::string& ip, int port, int& fd);
    

    static bool running;
    bool first;
};

void send_file(SSL* ssl, const std::string& filename);
void recv_file(SSL* ssl);
void ssl_free(SSL* ssl, int fd);

#endif // CLIENT_HPP
