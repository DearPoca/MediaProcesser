#pragma once

#include <atomic>
#include <condition_variable>
#include <mutex>

class Semaphore {
public:
    explicit Semaphore(int count = 0);

    void Signal();

    void Wait();

private:
    std::mutex mutex_;
    std::condition_variable cv_;
    int count_;
};
