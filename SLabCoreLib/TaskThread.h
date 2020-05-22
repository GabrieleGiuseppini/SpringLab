/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-20
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

/*
 * A thread that runs tasks provided by the main thread. The "user" of this
 * class may simply queue-and-forget tasks, or queue-and-wait until those
 * tasks are completed.
 *
 * The implementation assumes that there is only one thread "using" this
 * class (the main thread), and that thread is responsible for the lifetime
 * of this class (cctor and dctor).
 *
 */
class TaskThread
{
public:

    using Task = std::function<void()>;

    // Note: instances of this class are owned by the main thread, which is
    // also responsible for invoking the destructor of TaskThread, hence if
    // we assume there won't be any Wait() calls after TaskThread has been destroyed,
    // then there's no need for instances of this class to outlive the TaskThread
    // instance that generated them.
    struct TaskCompletionIndicator
    {
    public:

        //
        // Invoked by main thread to wait until the task is completed.
        //
        // Throws an exception if the task threw an exception.
        //

        void Wait()
        {
            std::unique_lock<std::mutex> lock(mThreadLock);

            // Wait for task completion
            mThreadSignal.wait(
                lock,
                [this]
                {
                    return mIsTaskCompleted;
                });

            // Check if an exception was thrown
            if (!mExceptionMessage.empty())
            {
                throw std::runtime_error(mExceptionMessage);
            }
        }

    private:

        TaskCompletionIndicator(
            std::mutex & threadLock,
            std::condition_variable & threadSignal)
            : mIsTaskCompleted(false)
            , mExceptionMessage()
            , mThreadLock(threadLock)
            , mThreadSignal(threadSignal)
        {}

        void MarkCompleted()
        {
            mIsTaskCompleted = true;
        }

        void RegisterException(std::string const & exceptionMessage)
        {
            mExceptionMessage = exceptionMessage;
        }

    private:

        bool mIsTaskCompleted; // Flag set by task thread to signal completion of the task
        std::string mExceptionMessage; // Set by task thread upon exception

        std::mutex & mThreadLock;
        std::condition_variable & mThreadSignal;

        friend class TaskThread;
    };

public:

    TaskThread();

    ~TaskThread();

    TaskThread(TaskThread const & other) = delete;
    TaskThread(TaskThread && other) = delete;
    TaskThread & operator=(TaskThread const & other) = delete;
    TaskThread & operator=(TaskThread && other) = delete;

    //
    // Invoked on the main thread to queue a task that will run
    // on the task thread.
    //

    std::shared_ptr<TaskCompletionIndicator> QueueTask(Task && task)
    {
        std::unique_lock<std::mutex> lock(mThreadLock);

        std::shared_ptr<TaskCompletionIndicator> taskCompletionIndicator = std::shared_ptr<TaskCompletionIndicator>(
            new TaskCompletionIndicator(
                mThreadLock,
                mThreadSignal));

        mTaskQueue.emplace_back(
            std::move(task),
            taskCompletionIndicator);

        mThreadSignal.notify_one();

        return taskCompletionIndicator;
    }

    //
    // Invoked on the main thread to queue a task that will run
    // on the task thread.
    //

    void RunSynchronously(Task && task)
    {
        auto result = QueueTask(std::move(task));
        result->Wait();
    }

private:

    void ThreadLoop();

private:

    struct QueuedTask
    {
        Task TaskToRun;
        std::shared_ptr<TaskCompletionIndicator> CompletionIndicator;

        QueuedTask()
            : TaskToRun()
            , CompletionIndicator()
        {
        }

        QueuedTask(
            Task && taskToRun,
            std::shared_ptr<TaskCompletionIndicator> completionIndicator)
            : TaskToRun(std::move(taskToRun))
            , CompletionIndicator(std::move(completionIndicator))
        {}
    };

private:

    std::thread mThread;

    std::mutex mThreadLock;
    std::condition_variable mThreadSignal; // Just one, as each of {main thread, worker thread} can't be waiting and signaling at the same time

    std::deque<QueuedTask> mTaskQueue;

    bool mIsStop;
};