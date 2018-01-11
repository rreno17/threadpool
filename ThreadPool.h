#pragma once

#include <condition_variable>
#include <functional>
#include <utility>
#include <vector>
#include <thread>
#include <deque>
#include <mutex>


namespace ThreadPool {

#define RUNNING     1
#define STOPPED     0

using ThreadPool = std::vector<std::thread>;
using JobQueue = std::deque<std::function<void()>>;

class Dispatch;

class Worker {
    /*
     *
     * Worker
     *
     * Takes a refrence to the spawning Dispatch for access to the
     * JobQueue. We're passing a refrence here so the worker can be
     * sure it's getting a valid object to work for. We also need to
     * be able to manipulate the JobQueue so a const& won't do either
     *
     */
public:
    Worker(Dispatch& dispatch);
    ~Worker();

    void        operator()();

private:
    Dispatch&   dispatch;   // omitting m_ prefix to avoid visual clutter

};


class Dispatch {
    /*
     *
     * Dispatch
     *
     * Manages thread pool and job queue. Provides the public
     * interface for assigning jobs to workers.
     *
     */
public:
    Dispatch();
    ~Dispatch();

    template<typename T, typename... A>
    void        addJob(T fn, A&&... args);

    static std::mutex               mut;
    static std::condition_variable  condvar;

protected:
    friend class Worker;

    JobQueue            jobQueue;
    int                 poolState;

private:
    ThreadPool          m_pool;
    size_t              m_numThreads;
};

/*
 *
 * Implementation of addJob required here so all compilation units
 * get the template parameters they need.
 *
 */

template<typename T, typename... A>
void Dispatch::addJob(T fn, A&&... args)
{
    /*
     * addJob takes a function and a list of arguments. It binds and
     * forwards them to a job object which is then placed in the
     * queue.
     *
     * example usage: dispatch->addJob(myFunc, "arg1", "arg2");
     * Since we aren't concerned at the moment with a return value,
     * we will not use futures.
     *
     */

    auto job = std::bind(fn, std::forward<A>(args)...);
    {
        std::lock_guard<std::mutex> lock(mut);
        jobQueue.push_back(job);
    }
    condvar.notify_one();
}

} // namespace ThreadPool
