#include <iostream>
#include <string>
#include <chrono>
#include <thread>
#include <memory>
#include <mutex>

#include "ThreadPool.h"

using namespace std::chrono_literals;
using namespace ThreadPool;

void job(const std::string& greeting, const std::string& msg)
{
    std::cout << "This is not thread safe." << std::endl;
    std::this_thread::sleep_for(3s);
    {
        std::lock_guard<std::mutex> lock(Dispatch::mut);
        std::cout << "This is thread safe.\n";
        std::cout << "The message is: \n"
                  << greeting << " " << msg << std::endl;
    }
}

int main(int argc, char** argv)
{
    try {
        auto dispatch = std::make_unique<Dispatch>();

        auto greeting = "Hello!";
        auto message = "I have a message for you from the people of Earth.";

        for (int i = 0; i < 10; i++) {
            dispatch->addJob(job, greeting, message);
        } 

        return 0;
    } catch (PoolException &p) {
        return -1;
    }
}
