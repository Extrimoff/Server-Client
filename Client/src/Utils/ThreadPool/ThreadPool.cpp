#include "ThreadPool.hpp"

void ThreadPool::setupThreadPool(unsigned int thread_count) {
    m_thread_pool.clear();
    for (unsigned int i = 0; i < thread_count; ++i)
        m_thread_pool.emplace_back(&ThreadPool::workerLoop, this);
}

void ThreadPool::workerLoop() {
    std::function<void()> job;
    while (!m_pool_terminated) {
        {
            std::unique_lock lock(m_queue_mtx);
            m_condition.wait(lock, [this]() { return !m_job_queue.empty() || m_pool_terminated; });
            if (m_pool_terminated) return;
            job = m_job_queue.front();
            m_job_queue.pop();
        }
        job();
    }
}

ThreadPool::ThreadPool(unsigned int thread_count) : m_pool_terminated(false) {
    setupThreadPool(thread_count);
}

ThreadPool::~ThreadPool() {
    if(!m_pool_terminated)
        this->stop();
}

void ThreadPool::join() {
    for (auto& thread : m_thread_pool) thread.join();
}

unsigned int ThreadPool::getThreadCount() const {
    return static_cast<unsigned int>(m_thread_pool.size());
}

void ThreadPool::dropUnstartedJobs() {
    m_pool_terminated = true;
    join();
    m_pool_terminated = false;
    std::queue<std::function<void()>> empty;
    std::swap(m_job_queue, empty);
    setupThreadPool(static_cast<unsigned int>(m_thread_pool.size()));
}

void ThreadPool::stop() {
    m_pool_terminated = true;
    m_condition.notify_all();
    join();
}

void ThreadPool::start(unsigned int thread_count) {
    if (!m_pool_terminated) return;
    m_pool_terminated = false;
    setupThreadPool(thread_count);
}