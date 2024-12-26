#include "streaming.hpp"
#include <openssl/ssl.h>
#include <opencv2/opencv.hpp>
#include <portaudio.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <unistd.h> 

#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

/* OpenCV for video streaming */

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

void stream_webcam(SSL* ssl) {
    cv::VideoCapture cap(0); // Open the default webcam (device index 0)
    if (!cap.isOpened()) {
        std::cerr << "Error: Unable to access the webcam.\n";
        return;
    }

    cv::Mat frame;
    while (true) {
        cap >> frame; // Capture a frame
        if (frame.empty()) {
            std::cerr << "Error: Failed to capture frame from webcam.\n";
            break;
        }

        // crop the center 640x480 region of a 1280x720 frame
        int crop_x = (frame.cols - 640) / 2;
        int crop_y = (frame.rows - 480) / 2;
        int crop_width = 640;
        int crop_height = 480;

        // Ensure the cropping rectangle is within the frame bounds
        crop_width = std::min(crop_width, frame.cols - crop_x);
        crop_height = std::min(crop_height, frame.rows - crop_y);

        if (crop_width > 0 && crop_height > 0) {
            cv::Rect crop_rect(crop_x, crop_y, crop_width, crop_height);
            cv::Mat cropped_frame = frame(crop_rect); // Crop the frame

            // Compress the cropped frame to JPEG
            std::vector<uchar> buffer;
            if (!cv::imencode(".jpg", cropped_frame, buffer)) {
                std::cerr << "Error: Failed to encode frame.\n";
                continue;
            }

            send_frame(ssl, std::vector<char>(buffer.begin(), buffer.end()));

            // Display the cropped frame
            cv::imshow("Webcam Streaming", cropped_frame);
        } else {
            std::cerr << "Error: Invalid cropping dimensions.\n";
        }

        // Add delay to control the frame rate (e.g., 30 FPS)
        if (cv::waitKey(30) >= 0) { // Exit when any key is pressed
            std::cout << "User interrupted webcam streaming. Exiting...\n";
            break;
        }
    }

    // Send an empty frame as EOF
    send_frame(ssl, std::vector<char>());

    // Ensure all data is sent
    int flush_status = SSL_write(ssl, nullptr, 0);
    if (flush_status <= 0) {
        std::cerr << "Error: Failed to flush SSL write buffer.\n";
    }

    std::cout << "Webcam streaming finished. Initiating SSL shutdown...\n";
    cap.release(); // Release the webcam

    // Explicitly destroy the window
    cv::destroyWindow("Webcam Streaming");
    for (int i = 0; i < 5; i++) {
        cv::waitKey(1);
    }
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

    // Flush remaining frames in the queue
    std::cout << "Flushing remaining frames...\n";
    while (!queue.empty()) {
        auto frame_data = queue.pop();
        if (!frame_data.empty()) {
            cv::Mat frame = cv::imdecode(cv::Mat(frame_data), cv::IMREAD_COLOR);
            if (!frame.empty()) {
                cv::imshow("Video Stream", frame);
                cv::waitKey(1); // Show frame briefly
            }
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

/* miniaudio for audio streaming */

struct AudioMetadata {
    uint32_t sampleRate;
    uint32_t channels;
    uint32_t format; // Use constants like `ma_format_f32` to indicate format
};

void stream_audio(SSL* ssl, const std::string& audio_path) {
    ma_decoder decoder;
    ma_result result = ma_decoder_init_file(audio_path.c_str(), nullptr, &decoder);
    if (result != MA_SUCCESS) {
        std::cerr << "Error: Failed to initialize decoder for audio file." << std::endl;
        return;
    }

    // Send metadata
    AudioMetadata metadata = { decoder.outputSampleRate, decoder.outputChannels, ma_format_unknown };
    if (SSL_write(ssl, &metadata, sizeof(metadata)) <= 0) {
        std::cerr << "Error: Failed to send audio metadata." << std::endl;
        ma_decoder_uninit(&decoder);
        return;
    }

    const size_t frame_chunk_size = 1024 * decoder.outputChannels; // Number of samples per chunk
    std::vector<char> frame_buffer(frame_chunk_size * sizeof(float)); // Buffer for decoded frames
    const double chunk_duration = static_cast<double>(frame_chunk_size) / 
                                  (decoder.outputSampleRate * decoder.outputChannels);

    while (true) {
        ma_uint64 frames_read = 0;
        ma_result read_result = ma_decoder_read_pcm_frames(&decoder, frame_buffer.data(), frame_chunk_size, &frames_read);

        if (read_result != MA_SUCCESS || frames_read == 0) {
            break; // End of audio file or error
        }

        // Send the frame
        send_frame(ssl, frame_buffer);

        // Introduce delay to match chunk duration
        usleep(static_cast<useconds_t>(chunk_duration * 1e6)); // Convert seconds to microseconds
    }

    send_frame(ssl, {}); // Send an empty frame as EOF
    ma_decoder_uninit(&decoder);

    int flush_status = SSL_write(ssl, nullptr, 0);
    if (flush_status <= 0) {
        std::cerr << "Error: Failed to flush SSL write buffer.\n";
    }
    std::cout << "Audio streaming finished." << std::endl;
}


void data_callback(ma_device* pDevice, void* pOutput, const void* pInput, ma_uint32 frameCount) {
    SSL* ssl = static_cast<SSL*>(pDevice->pUserData);
    std::vector<char> audio_data = receive_frame(ssl);

    if (audio_data.empty()) {
        // EOF received
        std::cout << "Received EOF in audio stream. Stopping playback." << std::endl;
        stop_audio(); // Signal to stop playback
        std::memset(pOutput, 0, frameCount * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
        return;
    }

    // Determine how many frames we can actually copy
    size_t frames_to_copy = std::min(static_cast<size_t>(frameCount), audio_data.size() / ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
    std::memcpy(pOutput, audio_data.data(), frames_to_copy * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));

    // Fill remaining buffer space with silence if necessary
    if (frames_to_copy < frameCount) {
        size_t offset = frames_to_copy * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels);
        std::memset(static_cast<uint8_t*>(pOutput) + offset, 0, (frameCount - frames_to_copy) * ma_get_bytes_per_frame(pDevice->playback.format, pDevice->playback.channels));
    }

    (void)pInput; // Unused
}

// Shared resources for playback control
std::condition_variable cond_var;
std::mutex cond_var_mutex;
std::atomic<bool> stop_flag(false);

void play_audio(SSL* ssl) {
    // Receive metadata
    AudioMetadata metadata;
    if (SSL_read(ssl, &metadata, sizeof(metadata)) <= 0) {
        std::cerr << "Error: Failed to receive audio metadata." << std::endl;
        return;
    }

    ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
    deviceConfig.playback.format = static_cast<ma_format>(metadata.format); // Use format from metadata
    deviceConfig.playback.channels = metadata.channels;                     // Use channels from metadata
    deviceConfig.sampleRate = metadata.sampleRate;                          // Use sample rate from metadata
    deviceConfig.dataCallback = data_callback;                              // Set the callback function
    deviceConfig.pUserData = ssl;                                           // Pass the SSL pointer to the callback

    ma_device device;
    if (ma_device_init(nullptr, &deviceConfig, &device) != MA_SUCCESS) {
        std::cerr << "Error: Failed to initialize playback device." << std::endl;
        return;
    }

    if (ma_device_start(&device) != MA_SUCCESS) {
        std::cerr << "Error: Failed to start playback device." << std::endl;
        ma_device_uninit(&device);
        return;
    }

    // Wait for the stop condition
    std::cout << "Audio is playing." << std::endl;
    std::unique_lock<std::mutex> lock(cond_var_mutex);
    cond_var.wait(lock, [] { return stop_flag.load(); });

    ma_device_stop(&device);
    ma_device_uninit(&device);
    std::cout << "Audio playback finished." << std::endl;
}

void stop_audio() {
    {
        std::lock_guard<std::mutex> lock(cond_var_mutex);
        stop_flag.store(true);
    }
    cond_var.notify_one();
}