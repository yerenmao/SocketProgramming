#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>
#include <map>
#include <arpa/inet.h> // sockaddr_in
#include <pthread.h>

// Maximum payload size for messages
#define MAX_PAYLOAD_SIZE 1024

// Message types
enum MsgType {
    JOIN = 1,         // Client joining the server
    CHAT = 2,         // Relay chat message
    REQUEST_PEER = 3, // Request peer info for direct connection
    PEER_INFO = 4,    // Response containing peer info
    DIRECT_MSG = 5    // Direct message type (if relayed via server for fallback)
    // Add more types: FILE_INIT, FILE_CHUNK, VIDEO_FRAME, etc.
};

struct Message {
    int msg_type;
    int from_id;       // Sender ID
    int to_id;         // Receiver ID
    int payload_size;
    char payload[MAX_PAYLOAD_SIZE];
};

#endif
