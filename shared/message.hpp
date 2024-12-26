#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>
#include <map>
#include <arpa/inet.h> // sockaddr_in
#include <pthread.h>

// 定義 Message 的 payload 上限
#define MAX_PAYLOAD_SIZE 1024

// 定義 
#define MAX_USERNAME_SIZE 128
#define MAX_PASSWORD_SIZE 128

// Message types
enum MsgType {
    RESPONSE = 0,

    JOIN = 1,         // Client joining the server
    CHAT = 2,         // Relay chat message
    REQUEST_PEER = 3, // Request peer info for direct connection
    PEER_INFO = 4,    // Response containing peer info
    DIRECT_MSG = 5,   // Direct message type (if relayed via server for fallback)

    DIRECT_SEND_FILE = 6,
    RELAY_SEND_FILE = 7,
    
    TRANSFER_FILE_CONTENT = 8,

    REGISTER = 10,
    LOGIN = 11,
    LOGOUT = 12,

    DIRECT_STREAMING = 16,
    RELAY_STREAMING = 17,
    // Add more types: FILE_INIT, FILE_CHUNK, VIDEO_FRAME, etc.
};

struct Message {
    int msg_type;
    int from_id;       // Sender ID
    int to_id;         // Receiver ID
    int payload_size;
    char payload[MAX_PAYLOAD_SIZE];

    std::string from_username;
};

#endif
