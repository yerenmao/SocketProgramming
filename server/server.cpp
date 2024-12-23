#include "server.hpp"

#include <iostream>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdexcept>

/* 開 socket，bind，listen */
Server::Server(int port, int max_clients, int worker_count)
    : port(port), max_clients(max_clients), thread_pool(worker_count) {}

Server::~Server() {
    close(server_fd);
}

void Server::create_socket() {
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        throw std::runtime_error("Failed to create socket");
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        throw std::runtime_error("Bind failed");
    }

    if (listen(server_fd, max_clients) < 0) {
        close(server_fd);
        throw std::runtime_error("Listen failed");
    }
}

void Server::start() {
    thread_pool.start();
    std::cout << "Server listening on port " << port << std::endl;
    accept_clients();
    thread_pool.stop();
}

void Server::accept_clients() {
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_socket < 0) {
            perror("accept");
            continue;
        }

        // 在 accept 時利用 add_task 傳入 lambda function
        thread_pool.add_task([client_socket]() {
            handle_client(client_socket);
        });
    }
}