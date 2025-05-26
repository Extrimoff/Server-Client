#include "ThreadPool.hpp"

void ThreadPool::setupThreadPool(unsigned int thread_count) {
    thread_pool.clear();
    for (unsigned int i = 0; i < thread_count; ++i)
        thread_pool.emplace_back(&ThreadPool::workerLoop, this);
}

void ThreadPool::workerLoop() {
    std::function<void()> job;
    while (!pool_terminated) {
        {
            std::unique_lock lock(queue_mtx);
            condition.wait(lock, [this]() { return !job_queue.empty() || pool_terminated; });
            if (pool_terminated) return;
            job = job_queue.front();
            job_queue.pop();
        }
        job();
    }
}

ThreadPool::ThreadPool(unsigned int thread_count) {
    setupThreadPool(thread_count);
}

ThreadPool::~ThreadPool() {
    pool_terminated = true;
    join();
}

void ThreadPool::join() {
    for (auto& thread : thread_pool) thread.join();
}

unsigned int ThreadPool::getThreadCount() const {
    return thread_pool.size();
}

void ThreadPool::dropUnstartedJobs() {
    pool_terminated = true;
    join();
    pool_terminated = false;
    std::queue<std::function<void()>> empty;
    std::swap(job_queue, empty);
    setupThreadPool(thread_pool.size());
}

void ThreadPool::stop() {
    pool_terminated = true;
    join();
}

void ThreadPool::start(unsigned int thread_count) {
    if (!pool_terminated) return;
    pool_terminated = false;
    setupThreadPool(thread_count);
}