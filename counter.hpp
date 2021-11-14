#pragma once
#include "lock_guard.hpp"
#include <pthread.h>

class Counter {
    int count = 0;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

public:
    void inc() {
        LockGuard lock(mutex);
        ++count;
        pthread_cond_broadcast(&cond);
    }

    void wait(int n) {
        LockGuard lock(mutex);
        while (count < n) {
            pthread_cond_wait(&cond, &mutex);
        }
    }
};