#pragma once
#include "lock_guard.hpp"
#include <optional>
#include <pthread.h>

class Data {
    enum class State { EMPTY, FULL, END } state = State::EMPTY;
    int value;
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t empty_cond = PTHREAD_COND_INITIALIZER;
    pthread_cond_t full_cond = PTHREAD_COND_INITIALIZER;

public:
    void set(int new_value) {
        LockGuard lock(mutex);
        while (state == State::FULL) {
            pthread_cond_wait(&empty_cond, &mutex);
        }
        value = new_value;
        state = State::FULL;
        pthread_cond_signal(&full_cond);
    }

    std::optional<int> get() {
        int result;
        {
            LockGuard lock(mutex);
            while (state == State::EMPTY) {
                pthread_cond_wait(&full_cond, &mutex);
            }
            if (state == State::END) {
                return {};
            }
            result = value;
            state = State::EMPTY;
            pthread_cond_signal(&empty_cond);
        }
        return result;
    }

    void end() {
        LockGuard lock(mutex);
        while (state == State::FULL) {
            pthread_cond_wait(&empty_cond, &mutex);
        }
        state = State::END;
        pthread_cond_broadcast(&full_cond);
    }

    bool ended() {
        LockGuard lock(mutex);
        return state == State::END;
    }
};