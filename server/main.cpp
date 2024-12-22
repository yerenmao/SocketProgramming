#include <iostream>
#include <cstdlib>
#include "server.hpp"

int main(int argc, char* argv[]) {
    /* 讀 terminal input */
    if (argc < 2 || argc > 4) {
        std::cerr << "Usage: " << argv[0] << " <server_port> [<max_clients> <worker_count>]\n";
        return 1;
    }
    int server_port = std::atoi(argv[1]);                    // 要開在哪個 port
    int max_clients = (argc > 2) ? std::atoi(argv[2]) : 10;  // 控制 listen 時最多可以有幾個 pending connection，預設為 10
    int worker_count = (argc > 3) ? std::atoi(argv[3]) : 10; // 控制 worker thread 的數量，預設為 10

    try {
        Server server(server_port, max_clients, worker_count);
        server.start();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
