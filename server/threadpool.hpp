#ifndef THREADPOOL_HPP
#define THREADPOOL_HPP

#include <pthread.h>
#include <queue>
#include <functional>
#include <vector>

// Represents a task to be handled by the thread pool
using Task = std::function<void()>;

// Thread pool class
class ThreadPool {
public:
    ThreadPool(int worker_count);
    ~ThreadPool();

    void start();
    void stop();
    void add_task(const Task& task);

private:
    std::vector<pthread_t> workers;
    std::queue<Task> task_queue;
    pthread_mutex_t queue_mutex;
    pthread_cond_t condition;
    bool stop_flag;

    static void* worker_thread(void* arg);
    void process_tasks();
};

#endif // THREADPOOL_HPP
