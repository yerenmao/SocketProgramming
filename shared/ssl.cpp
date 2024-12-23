#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

// 初始化 OpenSSL
void init_openssl()
{
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// 清理 OpenSSL (可視版本情況而定)
void cleanup_openssl()
{
    EVP_cleanup();
}

// 建立並設定 SSL_CTX (server 端)
SSL_CTX* create_server_context(const char* certFile, const char* keyFile)
{
    // 使用 TLS_server_method (OpenSSL 1.1+)
    const SSL_METHOD* method = TLS_server_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }

    // 載入伺服器憑證
    if (SSL_CTX_use_certificate_file(ctx, certFile, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }
    // 載入伺服器私鑰
    if (SSL_CTX_use_PrivateKey_file(ctx, keyFile, SSL_FILETYPE_PEM) <= 0) {
        ERR_print_errors_fp(stderr);
        SSL_CTX_free(ctx);
        return nullptr;
    }

    // 若需要雙向驗證，可在此設定 CA、SSL_CTX_set_verify() 等
    // 不過此範例先不做 client 憑證驗證

    return ctx;
}

// 建立並設定 SSL_CTX (Client 端)
SSL_CTX* create_client_context(const char* caFile)
{
    const SSL_METHOD* method = TLS_client_method();
    SSL_CTX* ctx = SSL_CTX_new(method);
    if (!ctx) {
        ERR_print_errors_fp(stderr);
        return nullptr;
    }

    // 若要驗證 server 憑證，載入 CA
    if (caFile) {
        if (!SSL_CTX_load_verify_locations(ctx, caFile, nullptr)) {
            ERR_print_errors_fp(stderr);
            SSL_CTX_free(ctx);
            return nullptr;
        }
        // 啟用對 Server 憑證的驗證
        SSL_CTX_set_verify(ctx, SSL_VERIFY_PEER, nullptr);
        SSL_CTX_set_verify_depth(ctx, 4);
    }

    return ctx;
}
