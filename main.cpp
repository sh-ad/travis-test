#include <sstream>
#include <iostream>
#include <atomic>
#include <pthread.h>
#include <vector>
#include <csignal>
#include <thread>
#include <random>

#define NOERROR 0
#define OVERFLOW 1

int N;
int MAX_SLEEP_TIME;

pthread_mutex_t mutex;
std::vector<pthread_t> consumers_threads{};
pthread_cond_t condition_new_element;
pthread_cond_t condition_processing;
pthread_t producer_thread;
pthread_t interruptor_thread;


thread_local std::atomic<int> last_error_code(NOERROR);
std::atomic<bool> values_ending{false};
std::atomic<bool> come_new_element{false};


struct consumers_data {
    int *num;
    int error = 0;
    int sum = 0;
};


int get_random_value(int m) {
    if (m == 0) {
        return 0;
    }
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(0, 1000);
    return dist(gen) % m;
}

bool check_int_overflow(int a, int b) {
    if (a > 0 || b > 0) {
        return a + b <= std::min(a, b);
    } else {
        return a + b >= std::min(a, b);
    }
}

int get_last_error() {
    return last_error_code;
}

void set_last_error(int code) {
    last_error_code = code;
}


void signalHandler(int) {
    values_ending = true;
    pthread_mutex_lock(&mutex);
    pthread_cancel(producer_thread);
    pthread_cond_signal(&condition_processing);
    pthread_cond_broadcast(&condition_new_element);
    pthread_mutex_unlock(&mutex);
}


void *producer_routine(void *num) {
    int number;
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, nullptr);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    while (std::cin >> number && !values_ending) {
        pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
        pthread_mutex_lock(&mutex);
        *static_cast<int *> (num) = number;
        come_new_element = true;
        pthread_cond_signal(&condition_new_element);
        while (come_new_element && !values_ending) {
            pthread_cond_wait(&condition_processing, &mutex);
        }
        pthread_mutex_unlock(&mutex);
        pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, nullptr);
    }

    pthread_mutex_lock(&mutex);
    values_ending = true;
    pthread_cond_broadcast(&condition_new_element);
    pthread_mutex_unlock(&mutex);
    return nullptr;
}


void *consumer_routine(void *arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);
    auto *consumer_args = static_cast<consumers_data *> (arg);
    int *num = consumer_args->num;

    int sum = 0;
    while (true) {
        pthread_mutex_lock(&mutex);
        while (!come_new_element && !values_ending) {
            pthread_cond_wait(&condition_new_element, &mutex);
        }

        if (values_ending) {
            pthread_mutex_unlock(&mutex);
            break;
        }

        bool check_overflow = check_int_overflow(sum, *num);
        if (check_overflow) {
            set_last_error(OVERFLOW);
        } else {
            sum += *num;
            come_new_element = false;
            pthread_cond_signal(&condition_processing);
        }

        pthread_mutex_unlock(&mutex);
        if (check_overflow) break;

        std::this_thread::sleep_for(std::chrono::milliseconds(get_random_value(MAX_SLEEP_TIME)));
    }
    consumer_args->error = get_last_error();
    consumer_args->sum = sum;
    return nullptr;
}

void *consumer_interruptor_routine(void *) {
    while (!values_ending) {
        pthread_cancel(consumers_threads[get_random_value(N)]);
    }
    return nullptr;
}

int run_threads() {
    signal(SIGINT, signalHandler);

    pthread_mutex_init(&mutex, nullptr);
    consumers_threads.reserve(N);
    pthread_cond_init(&condition_new_element, nullptr);
    pthread_cond_init(&condition_processing, nullptr);
    int value{};
    std::vector<consumers_data> consumers_threads_args(N, {&value});
    for (int i = 0; i < N; ++i) {
        pthread_create(&consumers_threads[i], nullptr, consumer_routine, &consumers_threads_args[i]);
    }

    pthread_create(&producer_thread, nullptr, producer_routine, &value);
    pthread_create(&interruptor_thread, nullptr, consumer_interruptor_routine, nullptr);

    pthread_join(interruptor_thread, nullptr);
    pthread_join(producer_thread, nullptr);

    int sum = 0;
    for (int i = 0; i < N; ++i) {
        auto &consumer_thread_args = consumers_threads_args[i];
        pthread_join(consumers_threads[i], nullptr);
        sum += consumer_thread_args.sum;
        if (consumer_thread_args.error == OVERFLOW) {
            return 1;
        }
    }

    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&condition_new_element);
    pthread_cond_destroy(&condition_processing);

    std::cout << sum << std::endl;
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 3)
        return 1;
    std::stringstream ss;
    ss << argv[1] << ' ' << argv[2];
    ss >> N >> MAX_SLEEP_TIME;
    return run_threads();
}