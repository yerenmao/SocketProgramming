#ifndef STREAMING_QUEUE_HPP
#define STREAMING_QUEUE_HPP

#include <queue>
#include <vector>
#include <pthread.h>

class StreamingQueue {
private:
    std::queue<std::vector<char>> queue;
    pthread_mutex_t mutex;
    pthread_cond_t cond;

public:
    StreamingQueue() {
        pthread_mutex_init(&mutex, nullptr);
        pthread_cond_init(&cond, nullptr);
    }

    ~StreamingQueue() {
        pthread_mutex_destroy(&mutex);
        pthread_cond_destroy(&cond);
    }

    void push(const std::vector<char>& frame) {
        pthread_mutex_lock(&mutex);
        queue.push(frame);
        pthread_cond_signal(&cond); // Notify waiting threads
        pthread_mutex_unlock(&mutex);
    }

    std::vector<char> pop() {
        pthread_mutex_lock(&mutex);

        // Wait until the queue is not empty
        while (queue.empty()) {
            pthread_cond_wait(&cond, &mutex);
        }

        std::vector<char> frame = queue.front();
        queue.pop();

        pthread_mutex_unlock(&mutex);
        return frame;
    }

    bool empty() {
        pthread_mutex_lock(&mutex);
        bool is_empty = queue.empty();
        pthread_mutex_unlock(&mutex);
        return is_empty;
    }
};

#endif // STREAMING_QUEUE_HPP
