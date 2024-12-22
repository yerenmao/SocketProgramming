#include "threadpool.hpp"
#include <iostream>
#include <stdexcept>

ThreadPool::ThreadPool(int worker_count) : stop_flag(false) {
    pthread_mutex_init(&queue_mutex, nullptr);
    pthread_cond_init(&condition, nullptr);
    workers.resize(worker_count);
}

ThreadPool::~ThreadPool() {
    stop();
    pthread_mutex_destroy(&queue_mutex);
    pthread_cond_destroy(&condition);
}

void ThreadPool::start() {
    for (auto& worker : workers) {
        pthread_create(&worker, nullptr, worker_thread, this);
    }
}

void ThreadPool::stop() {
    {
        pthread_mutex_lock(&queue_mutex);
        stop_flag = true;
        pthread_cond_broadcast(&condition);
        pthread_mutex_unlock(&queue_mutex);
    }

    for (auto& worker : workers) {
        pthread_join(worker, nullptr);
    }
}

void ThreadPool::add_task(const Task& task) {
    pthread_mutex_lock(&queue_mutex);
    task_queue.push(task);
    pthread_cond_signal(&condition);        // 通知說有新的 task 進來了！
    pthread_mutex_unlock(&queue_mutex);
}

/* 因為 pthread_create 要求 thread function 為：

    void* (*start_routine)(void*);

因此利用傳入 this，就可以呼叫到 process_tasks 這個 function。
*/
void* ThreadPool::worker_thread(void* arg) {
    ThreadPool* pool = static_cast<ThreadPool*>(arg);
    pool->process_tasks();
    return nullptr;
}

/* 此 function 的目的為：
1. 在正確的時機利用 condition variable 離開 wait
2. 利用 mutex lock 正確的從 queue 中把 task pop 出來
3. 執行 task 的內容
*/
void ThreadPool::process_tasks() {
    while (true) {
        Task task;

        /* Condition Variables 的標準寫法*/
        pthread_mutex_lock(&queue_mutex);
        while (task_queue.empty() && !stop_flag) {
            pthread_cond_wait(&condition, &queue_mutex);    // 等新的 task
        }
        if (stop_flag) {
            pthread_mutex_unlock(&queue_mutex);
            break;
        }
        task = task_queue.front();              // 把 queue 的一個 task pop 出來
        task_queue.pop();
        pthread_mutex_unlock(&queue_mutex);     // 解除 queue 的 mutex lock

        /* 執行 task 的內容 */
        try {
            task();
        } catch (const std::exception& e) {
            std::cerr << "Task execution error: " << e.what() << std::endl;
        }
    }
}
