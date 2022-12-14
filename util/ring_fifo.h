#pragma once

#include <mutex>

#include "semaphore.h"

template <typename T>
class RingFIFO {
public:
    void Put(T t);
    bool PutNoWait(T t);

    T Get();
    bool GetNoWait(T& t);

    RingFIFO() = delete;
    RingFIFO(int size);
    ~RingFIFO();

private:
    T* buffer_;
    int size_;
    int head_;
    int tail_;

    std::mutex mux_;
    std::condition_variable cv_full_;
    std::condition_variable cv_empty_;
};
