#pragma once
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <functional>

class ThreadPool {
    std::vector<std::thread>            m_thread_pool;
    std::queue<std::function<void()>>   m_job_queue;
    std::mutex                          m_queue_mtx;
    std::condition_variable             m_condition;
    std::atomic<bool>                   m_pool_terminated;

    void setupThreadPool(unsigned int thread_count);
    void workerLoop();

public:
    explicit ThreadPool(unsigned int thread_count = std::thread::hardware_concurrency());
    ~ThreadPool();

    template<typename F>
    void addJob(F job) {
        if (m_pool_terminated) return;
        {
            std::unique_lock lock(m_queue_mtx);
            m_job_queue.push(std::function<void()>(job));
        }
        m_condition.notify_one();
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