#ifndef THREAD_HANDLERS_HPP
#define THREAD_HANDLERS_HPP

#include <pthread.h>

int create_listening_socket(int port);
void* server_listener_thread_func(void* arg);
void* direct_listener_thread_func(void* arg);

#endif // THREAD_HANDLERS_HPP