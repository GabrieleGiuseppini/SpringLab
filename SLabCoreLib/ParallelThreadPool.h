/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <cassert>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

/*
 * This class implements a thread pool that executes batches of tasks.
 *
 */
class ParallelThreadPool
{
public:

    using Task = std::function<void()>;

public:

    explicit ParallelThreadPool(size_t parallelism);

    ~ParallelThreadPool();

    size_t GetParallelism() const
    {
        return mThreads.size() + 1;
    }

    /*
     * The first task is guaranteed to run on the main thread.
     */
    void Run(std::vector<Task> const & tasks);

private:

    void ThreadLoop(size_t t);

private:

    // Our thread lock
    std::mutex mLock;

    // Our threads
    std::vector<std::thread> mThreads;

    // The tasks currently awaiting to be picked up by each thread
    std::vector<Task const *> mTasks;

    // The condition variable to wake up threads when new tasks are ready
    std::condition_variable mNewTasksAvailableSignal;

    // The condition variable to wake up the main thread when all tasks are completed
    std::condition_variable mTasksCompletedSignal;

    // TODOTEST: the condition variable to sync the end of a loop
    std::condition_variable mEndOfTaskLoopSignal;

    // The number of tasks awaiting for completion
    size_t mTasksToComplete;

    // Set to true when have to stop
    bool mIsStop;
};
