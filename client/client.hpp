#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include <pthread.h>

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
    int get_direct_listen_fd() const { return direct_listen_fd; }
    void successful_login(const std::string& username);

private:
    int server_fd;
    int direct_listen_fd;
    std::string server_ip;
    int server_port;
    int my_listen_port;

    pthread_t server_listener_thread;
    pthread_t direct_listener_thread;

    std::string username; // Store the logged-in user's username
    bool logged_in = false; // Track login state
    void login(const std::string& username, const std::string& password);
    void logout();
    void register_user(const std::string& username, const std::string& password);
    void chat(int to_id, const std::string& message);
    void request_peer(int to_id);
    void direct_send(const std::string& peer_ip, int peer_port, const std::string& message);

    static bool running;
};

#endif // CLIENT_HPP
