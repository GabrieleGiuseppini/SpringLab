/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ThreadManager.h"

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

/*
 * This class implements a thread pool that executes batches of tasks.
 *
 */
class ThreadPool
{
public:

    using Task = std::function<void()>;

public:

    explicit ThreadPool(
        size_t parallelism/*,
        ThreadManager & threadManager*/);

    ~ThreadPool();

    size_t GetParallelism() const
    {
        return mThreads.size() + 1;
    }

    /*
     * The first task is guaranteed to run on the main thread.
     */
    void Run(std::vector<Task> const & tasks);

private:

    void ThreadLoop(
        size_t t/*,
        ThreadManager & threadManager*/);

private:

    // Our thread lock
    std::mutex mLock;

    // Our threads
    std::vector<std::thread> mThreads;

    // The tasks currently awaiting to be picked up by each thread;
    // set by main thread, cleared by each thread
    std::vector<Task const *> mTasks;

    // The condition variable to wake up threads when new tasks are ready
    std::condition_variable mNewTasksAvailableSignal;

    // The number of tasks awaiting for completion; 
    // set by main thread, decreased by each thread, awaited by main thread
    size_t mTasksToComplete;

    // The condition variable to wake up the main thread when all tasks are completed
    std::condition_variable mTasksCompletedSignal;

    // Set to true when have to stop
    bool mIsStop;
};
