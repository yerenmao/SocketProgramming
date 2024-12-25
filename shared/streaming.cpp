#include "streaming.hpp"
#include <openssl/ssl.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <unistd.h> 

void send_frame(SSL* ssl, const std::vector<char>& frame) {
    uint32_t frame_size = htonl(static_cast<uint32_t>(frame.size())); // Convert to network byte order
    size_t total_written = 0;

    // Send frame size
    while (total_written < sizeof(frame_size)) {
        int bytes_written = SSL_write(ssl, reinterpret_cast<const char*>(&frame_size) + total_written, sizeof(frame_size) - total_written);
        if (bytes_written <= 0) {
            std::cerr << "Error: Failed to send frame size. SSL_write returned " << bytes_written << "\n";
            return;
        }
        total_written += bytes_written;
    }

    // Send frame data
    total_written = 0;
    while (total_written < frame.size()) {
        int bytes_written = SSL_write(ssl, frame.data() + total_written, frame.size() - total_written);
        if (bytes_written <= 0) {
            std::cerr << "Error: Failed to send frame data. SSL_write returned " << bytes_written << "\n";
            return;
        }
        total_written += bytes_written;
    }
}


std::vector<char> receive_frame(SSL* ssl) {
    uint32_t frame_size_network = 0; // Buffer to store network byte order frame size
    size_t total_read = 0;

    // Read frame size
    while (total_read < sizeof(frame_size_network)) {
        int bytes_read = SSL_read(ssl, reinterpret_cast<char*>(&frame_size_network) + total_read, sizeof(frame_size_network) - total_read);
        if (bytes_read <= 0) {
            std::cerr << "Error: Failed to read frame size. SSL_read returned " << bytes_read << "\n";
            return {}; // Return an empty vector on failure
        }
        total_read += bytes_read;
    }

    // Convert frame size to host byte order
    uint32_t frame_size = ntohl(frame_size_network);

    // Prepare buffer for frame data
    std::vector<char> frame(frame_size);
    total_read = 0;

    // Read frame data
    while (total_read < frame_size) {
        int bytes_read = SSL_read(ssl, frame.data() + total_read, frame_size - total_read);
        if (bytes_read <= 0) {
            std::cerr << "Error: Failed to read frame data. SSL_read returned " << bytes_read << "\n";
            return {}; // Return an empty vector on failure
        }
        total_read += bytes_read;
    }

    return frame;
}

void stream_video(SSL* ssl, const std::string& video_path) {
    cv::VideoCapture cap(video_path);
    if (!cap.isOpened()) {
        std::cerr << "Error: Unable to open video file.\n";
        return;
    }

    cv::Mat frame;
    while (cap.read(frame)) {
        std::vector<uchar> buffer;
        cv::imencode(".jpg", frame, buffer); // Compress frame to JPEG
        send_frame(ssl, std::vector<char>(buffer.begin(), buffer.end()));
    }

    // Send an empty frame as EOF
    send_frame(ssl, std::vector<char>());

    // Ensure all data is sent
    int flush_status = SSL_write(ssl, nullptr, 0);
    if (flush_status <= 0) {
        std::cerr << "Error: Failed to flush SSL write buffer.\n";
    }

    std::cout << "Video streaming finished. Initiating SSL shutdown...\n";
}

void display(StreamingQueue& queue, bool& running) {
    bool streaming_complete = false;

    while (running) {
        if (!queue.empty()) {
            auto frame_data = queue.pop();

            // Check for EOF signal
            if (frame_data.empty()) {
                std::cout << "Received EOF. Stopping video playback.\n";
                streaming_complete = true;
                break; // Exit the loop
            }

            cv::Mat frame = cv::imdecode(cv::Mat(frame_data), cv::IMREAD_COLOR);
            if (frame.empty()) {
                std::cerr << "Error: Failed to decode frame.\n";
                continue;
            }

            cv::imshow("Video Stream", frame);
            if (cv::waitKey(30) >= 0) {
                std::cout << "User interrupted streaming. Exiting...\n";
                break;
            }
        } else if (streaming_complete) {
            // Exit if streaming is marked as complete and the queue is empty
            break;
        } else {
            usleep(10000); // Add a slight delay to reduce CPU usage
        }
    }

    cv::destroyAllWindows();
    for (int i = 0; i < 5; i++) {
        cv::waitKey(1);
    }
    std::cout << "Streaming window closed.\n";
}


void enqueue_frame(StreamingQueue& queue, SSL* ssl) {
    while (true) {
        auto frame_data = receive_frame(ssl);

        // Push EOF signal (empty frame) to the queue
        if (frame_data.empty()) {
            std::cout << "Received EOF. Exiting frame receiving loop.\n";
            queue.push(frame_data); // Push EOF to signal the end of the stream
            break;
        }

        queue.push(frame_data);
    }
}