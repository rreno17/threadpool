#include <condition_variable>
#include <functional>
#include <iostream>
#include <utility>
#include <vector>
#include <thread>
#include <memory>
#include <deque>
#include <mutex>

#include "ThreadPool.h"


namespace ThreadPool {

std::mutex
Dispatch::mut;

std::condition_variable
Dispatch::condvar;


Worker::Worker(Dispatch& d) : dispatch(d)
{
}

Worker::~Worker()
{
}


void
Worker::operator() ()
{
    std::function<void()> job;
    while(true) {
        { // acquire lock
            std::unique_lock<std::mutex> lock(Dispatch::mut);
            /*
             * Put a worker to sleep. If the Dispatch destructor
             * is called or if the job queue is nonempty,
             * the condvar will wake up the sleeping thread to either
             * be cleaned up or execute a job.
             */
            Dispatch::condvar.wait(lock,
                [this](){
                        return(dispatch.poolState == STOPPED
                               || dispatch.jobQueue.size());
                                   });

            if(dispatch.poolState == STOPPED) return;

            job = std::move(dispatch.jobQueue.front());
            dispatch.jobQueue.pop_front();
        }// release lock

        // Let Dispatch know we got a job.
        Dispatch::condvar.notify_all();

        try {
            job();
        }
        catch (const std::exception& e) {
            std::cerr << "Caught exception: "
                      << e.what() << " in thread: "
                      << std::this_thread::get_id()
                      << std::endl;
        }
    }
}


Dispatch::Dispatch()
         : m_pool(),
           m_numThreads(std::thread::hardware_concurrency()),
           jobQueue(),
           poolState(RUNNING)
{
    m_pool.reserve(m_numThreads);

    for (size_t i = 0; i < m_numThreads; i++) {
        m_pool.emplace_back(std::thread(Worker(*this)));
    }
}


Dispatch::~Dispatch()
{
    std::unique_lock<std::mutex> lock(mut);

    /*
     * Wait for the job queue to empty. Workers will wake this thread
     * as they remove jobs from the queue
     */
    condvar.wait(lock,
                 [this](){
                    return jobQueue.empty();
    });

    poolState = STOPPED;
    lock.unlock();

    condvar.notify_all(); // wake all workers so they exit operator()

    for (size_t i = m_pool.size() - 1; m_pool.size(); i--) {
        m_pool[i].join();
        m_pool.pop_back();
    }
}


} // namespace ThreadPool

