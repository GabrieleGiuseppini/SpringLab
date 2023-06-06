/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ThreadPool.h"

#include "ThreadManager.h"

#include <algorithm>

ThreadPool::ThreadPool(size_t parallelism)
    : mLock()
    , mThreads()
    , mTasks(parallelism - 1, nullptr)
    , mNewTasksAvailableSignal()
    , mTasksToComplete(0)
    , mTasksCompletedSignal()
    , mIsStop(false)
{
    assert(parallelism > 0);

    // Start N-1 threads (main thread is one of them)
    for (size_t t = 0; t < parallelism - 1; ++t)
    {
        mThreads.emplace_back(&ThreadPool::ThreadLoop, this, t);
    }
}

ThreadPool::~ThreadPool()
{
    // Tell all threads to stop
    {
        std::unique_lock const lock{ mLock };

        mIsStop = true;
    }

    // Signal threads
    mNewTasksAvailableSignal.notify_all();

    // Wait for all threads to exit
    for (auto & t : mThreads)
    {
        t.join();
    }
}

void ThreadPool::Run(std::vector<Task> const & tasks)
{   
    assert(tasks.size() > 0);
    assert(mTasksToComplete == 0);
    assert(!mIsStop);

    // Queue all tasks that may run on our threads - padding with null's

    size_t const queuedTasks = std::min(tasks.size() - 1, mThreads.size());
    for (size_t t = 0; t < mThreads.size(); ++t)
    {
        if (t < queuedTasks)
        {
            mTasks[t] = &(tasks[tasks.size() - queuedTasks + t]);
        }
        else
        {
            mTasks[t] = nullptr;
        }
    }

    // Signal that there are tasks available

    {
        std::unique_lock lock{ mLock };

        mTasksToComplete = queuedTasks;
    }

    mNewTasksAvailableSignal.notify_all();

    // Run all tasks that have to run on main thread

    for (size_t t = 0; t < tasks.size() - queuedTasks; ++t)
    {
        tasks[t]();
    }

    // Wait until all tasks are completed

    {
        std::unique_lock lock{ mLock };

        // Wait for signal
        mTasksCompletedSignal.wait(
            lock,
            [this]
            {
                return 0 == mTasksToComplete;
            });

        assert(0 == mTasksToComplete);
    }
}

void ThreadPool::ThreadLoop(size_t t)
{
    //
    // Initialize thread
    //

    ThreadManager::GetInstance().InitializeThisThread();

    //
    // Run thread loop until thread pool is destroyed
    //

    while (true)
    {
        // Wait for our task (or stop)
        Task const * task;
        {
            std::unique_lock lock{ mLock };

            // Wait for signal
            mNewTasksAvailableSignal.wait(
                lock,
                [this, t]
                {
                    return mIsStop || mTasks[t] != nullptr;
                });

            if (mIsStop)
            {
                // We're done!
                break;
            }

            task = mTasks[t];
            mTasks[t] = nullptr; // Consume task
        }

        // Run our task
        assert(task != nullptr);
        {
            // TODOTEST: verify how expensive to try/catch
            ////try
            ////{
            ////    task();
            ////}
            ////catch (std::exception const & e)
            ////{
            ////    assert(false); // Catch it in debug mode

            ////    LogMessage("Error running task: " + std::string(e.what()));

            ////    // Keep going...
            ////}

            (*task)();
        }

        // Signal we're done

        size_t remainingTasksToComplete;
        {
            std::unique_lock lock{ mLock };
            
            assert(mTasksToComplete > 0);

            remainingTasksToComplete = --mTasksToComplete;
        }

        if (remainingTasksToComplete == 0)
        {
            mTasksCompletedSignal.notify_all();
        }
    }
}
