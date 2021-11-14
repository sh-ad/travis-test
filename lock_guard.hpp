#pragma once
#include <pthread.h>

class LockGuard {
    pthread_mutex_t& mutex;

public:
    LockGuard(pthread_mutex_t& mutex) : mutex(mutex) {
        pthread_mutex_lock(&mutex);
    }

    ~LockGuard() noexcept {
        pthread_mutex_unlock(&mutex);
    }
};