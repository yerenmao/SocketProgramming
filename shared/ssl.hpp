#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

// 初始化 OpenSSL
void init_openssl();

// 清理 OpenSSL (可視版本情況而定)
void cleanup_openssl();

// 建立並設定 SSL_CTX (server 端)
SSL_CTX* create_server_context(const char* certFile, const char* keyFile);

// 建立並設定 SSL_CTX (Client 端)
SSL_CTX* create_client_context(const char* caFile);

