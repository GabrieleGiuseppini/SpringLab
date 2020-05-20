/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "TaskThread.h"

#include "FloatingPoint.h"
#include "Log.h"

TaskThread::TaskThread()
    : mIsStop(false)
{
    // Start thread
    mThread = std::thread(&TaskThread::ThreadLoop, this);
}

TaskThread::~TaskThread()
{
    // Notify stop
    {
        std::unique_lock const lock{ mThreadLock };

        mIsStop = true;
        mThreadSignal.notify_one();
    }

    LogMessage("TaskThread::~TaskThread(): signaled stop; waiting for thread now...");

    // Wait for thread
    mThread.join();

    LogMessage("TaskThread::~TaskThread(): ...thread stopped.");
}

void TaskThread::ThreadLoop()
{
    //
    // Initialize floating point handling
    //

    // Avoid denormal numbers for very small quantities
    EnableFloatingPointFlushToZero();

#ifdef FLOATING_POINT_CHECKS
    EnableFloatingPointExceptions();
#endif

    //
    // Run loop
    //

    while (true)
    {
        //
        // Extract task
        //

        QueuedTask taskToRun;

        {
            std::unique_lock<std::mutex> lock(mThreadLock);

            // Wait for signal
            mThreadSignal.wait(
                lock,
                [this]
                {
                    return mIsStop || !mTaskQueue.empty();
                });

            if (mIsStop)
            {
                // We're done!
                break;
            }
            else
            {
                //
                // Deque task
                //

                assert(!mTaskQueue.empty());

                taskToRun = std::move(mTaskQueue.front());
                mTaskQueue.pop_front();
            }
        }

        //
        // Run task
        //

        assert(!!taskToRun.task);
        taskToRun.task();

        //
        // Signal task completion
        //

        {
            std::unique_lock const lock{ mThreadLock };

            *(taskToRun.isTaskCompleted) = true;
            mThreadSignal.notify_one();
        }
    }

    LogMessage("TaskThread::ThreadLoop(): exiting");
}