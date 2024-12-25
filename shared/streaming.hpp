#ifndef STREAMING_HPP
#define STREAMING_HPP

#include <openssl/ssl.h>
#include <vector>
#include <string>
#include "streaming_queue.hpp" // Include the StreamingQueue definition

// Function declarations for streaming
void send_frame(SSL* ssl, const std::vector<char>& frame);
void stream_video(SSL* ssl, const std::string& video_path);
std::vector<char> receive_frame(SSL* ssl);

// Display frames from the streaming queue
void display(StreamingQueue& queue, bool& running);
// Enqueue frames from SSL connection into the streaming queue
void enqueue_frame(StreamingQueue& queue, SSL* ssl);

#endif // STREAMING_HPP
