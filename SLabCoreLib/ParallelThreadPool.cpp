/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ParallelThreadPool.h"

#include "FloatingPoint.h"
#include "Log.h"

#include <algorithm>

ParallelThreadPool::ParallelThreadPool(size_t parallelism)
    : mLock()
    , mThreads()
    , mTasks(parallelism - 1, nullptr)
    , mNewTasksAvailableSignal()
    , mTasksCompletedSignal()
    , mEndOfTaskLoopSignal()
    , mTasksToComplete(0)
    , mIsStop(false)
{
    assert(parallelism > 0);

    // Start N-1 threads (main thread is one of them)
    for (size_t t = 0; t < parallelism - 1; ++t)
    {
        mThreads.emplace_back(&ParallelThreadPool::ThreadLoop, this, t);
    }
}

ParallelThreadPool::~ParallelThreadPool()
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

void ParallelThreadPool::Run(std::vector<Task> const & tasks)
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
    // TODO: need assurance that all threads are waiting!
    // TODO: might have them wait after decreasing semaphore, before restarting wait
    {
        std::unique_lock lock{ mLock };

        mTasksToComplete = mThreads.size();        
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

    // TODOTEST
    mEndOfTaskLoopSignal.notify_all();
}

void ParallelThreadPool::ThreadLoop(size_t t)
{
    //
    // Initialize thread
    //

    EnableFloatingPointFlushToZero();

    //
    // Run thread loop until thread pool is destroyed
    //

    while (true)
    {
        // Wait for tasks (or stop)
        {
            std::unique_lock lock{ mLock };

            // Wait for signal
            mNewTasksAvailableSignal.wait(
                lock,
                [this]
                {
                    return mIsStop || mTasksToComplete > 0;
                });

            if (mIsStop)
            {
                // We're done!
                break;
            }
        }

        // Run our task
        Task const * task = mTasks[t];
        if (task)
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
        {
            std::unique_lock lock{ mLock };
            
            assert(mTasksToComplete > 0);

            --mTasksToComplete;

            if (mTasksToComplete == 0)
            {
                mTasksCompletedSignal.notify_all();
            }

            // TODOTEST
            mEndOfTaskLoopSignal.wait(lock);
        }
    }
}
