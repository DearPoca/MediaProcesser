#include "semaphore.h"

Semaphore::Semaphore(int count) : count_(count) {}

void Semaphore::Signal() {
    std::unique_lock<std::mutex> lock(mutex_);
    ++count_;
    cv_.notify_one();
}

void Semaphore::Wait() {
    std::unique_lock<std::mutex> lock(mutex_);
    cv_.wait(lock, [=] { return count_ > 0; });
    --count_;
}