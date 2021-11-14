#include <pthread.h>
#include <signal.h>
#include <unistd.h>

#include <csignal>
#include <cstdlib>
#include <ctime>

#include <iostream>
#include <string>
#include <vector>

#include "counter.hpp"
#include "data.hpp"

sigset_t sigterm_sigset() {
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    return sigset;
}

namespace {
volatile std::sig_atomic_t terminated = false;
}

void sigterm_handler(int) {
    terminated = true;
}

struct producer_args {
    Data& data;
    Counter& consumer_counter;
};

void* producer_routine(void* arg) {
    producer_args& args = *(producer_args*)arg;

    // Unblock SIGTERM and set its handler
    sigset_t sigterm = sigterm_sigset();
    pthread_sigmask(SIG_UNBLOCK, &sigterm, nullptr);
    std::signal(SIGTERM, sigterm_handler);

    // Wait for consumer to start
    args.consumer_counter.wait(1);

    // Read data, loop through each value and update the value, notify consumer, wait for consumer to process
    std::FILE* file = std::fopen("in.txt", "r");
    char symbol;
    do {
        int value;
        if (std::fscanf(file, "%d%c", &value, &symbol) < 2) {
            break;
        }
        args.data.set(value);
    } while (symbol != '\n' && !terminated);
    std::fclose(file);
    args.data.end();

    return nullptr;
}

void random_sleep(useconds_t sleep_max) {
    if (sleep_max) {
        useconds_t sleep_time = std::rand() % sleep_max;
        usleep(sleep_time);
    }
}

struct consumer_args {
    Data& data;
    Counter& consumer_counter;
    useconds_t sleep_max;
    bool debug;
};

void* consumer_routine(void* arg) {
    consumer_args& args = *(consumer_args*)arg;
    pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, nullptr);

    // notify about start
    args.consumer_counter.inc();

    // for every update issued by producer, read the value and add to sum
    int sum = 0;
    pthread_t thread_id = pthread_self();
    while (true) {
        std::optional<int> value = args.data.get();
        if (!value) {
            break;
        }
        sum += value.value();
        if (args.debug) {
            std::cout << thread_id << ' ' << sum << std::endl;
        }
        random_sleep(args.sleep_max);
    }

    // return pointer to result (for particular consumer)
    return new int{sum};
}

void interrupt_consumers(Data& data, std::vector<pthread_t> const& consumers) {
    while (!data.ended()) {
        pthread_t consumer = consumers[std::rand() % consumers.size()];
        pthread_cancel(consumer);
    }
}

struct interruptor_args {
    Data& data;
    Counter& consumer_counter;
    std::vector<pthread_t> const& consumers;
};

void* consumer_interruptor_routine(void* arg) {
    interruptor_args& args = *(interruptor_args*)arg;

    // wait for consumers to start
    args.consumer_counter.wait(args.consumers.size());

    // interrupt random consumer while producer is running
    while (!args.data.ended()) {
        int random_consumer = std::rand() % args.consumers.size();
        pthread_cancel(args.consumers[random_consumer]);
    }

    return nullptr;
}

int run_threads(int N, useconds_t sleep_max, bool debug) {
    pthread_t producer, interruptor;
    std::vector<pthread_t> consumers(N);
    Data data;
    Counter consumer_counter;

    std::srand(std::time(nullptr));

    // block SIGTERM in this thread and its children
    sigset_t sigterm = sigterm_sigset();
    pthread_sigmask(SIG_BLOCK, &sigterm, nullptr);

    // start 2+N threads and wait until they're done
    producer_args* p_args = new producer_args{data, consumer_counter};
    pthread_create(&producer, nullptr, producer_routine, p_args);
    interruptor_args* i_args = new interruptor_args{data, consumer_counter, consumers};
    pthread_create(&interruptor, nullptr, consumer_interruptor_routine, i_args);
    consumer_args* c_args = new consumer_args{data, consumer_counter, sleep_max, debug};
    for (int i = 0; i < N; ++i) {
        pthread_create(&consumers[i], nullptr, consumer_routine, c_args);
    }

    pthread_join(producer, nullptr);
    pthread_join(interruptor, nullptr);
    int sum = 0;
    for (int i = 0; i < N; ++i) {
        void* partial_sum;
        pthread_join(consumers[i], &partial_sum);
        sum += *(int*)partial_sum;
        delete (int*)partial_sum;
    }

    delete p_args;
    delete i_args;
    delete c_args;

    // return aggregated sum of values
    return sum;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Too few arguments" << std::endl;
        return 1;
    }
    int N = std::atoi(argv[1]);
    int sleep_max_ms = std::atoi(argv[2]);
    bool debug = argc > 3 && std::string(argv[3]) == "-debug";

    std::cout << run_threads(N, sleep_max_ms * 1000, debug) << std::endl;
    return 0;
}
