#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

class ThreadPool {
    std::vector<std::thread> thread_pool;
    std::queue<std::function<void()>> job_queue;
    std::mutex queue_mtx;
    std::condition_variable condition;
    std::atomic<bool> pool_terminated = false;

    void setupThreadPool(unsigned int thread_count);
    void workerLoop();

public:
    explicit ThreadPool(unsigned int thread_count = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<typename F>
    void addJob(F job) {
        if (pool_terminated) return;
        {
            std::unique_lock lock(queue_mtx);
            job_queue.push(std::function<void()>(job));
        }
        condition.notify_one();
    }

    template<typename F, typename... Arg>
    void addJob(const F& job, const Arg&... args) {
        addJob([job, args...] { job(args...); });
    }

    void join();
    unsigned int getThreadCount() const;
    void dropUnstartedJobs();
    void stop();
    void start(unsigned int thread_count = std::thread::hardware_concurrency());
};