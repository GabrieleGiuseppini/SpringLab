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

        void Wait()
        {
            std::unique_lock<std::mutex> lock(mThreadLock);

            // Wait for task completion
            mThreadSignal.wait(
                lock,
                [this]
                {
                    return *mIsTaskCompleted;
                });
        }

    private:

        TaskCompletionIndicator(
            std::shared_ptr<bool> isTaskCompleted,
            std::mutex & threadLock,
            std::condition_variable & threadSignal)
            : mIsTaskCompleted(std::move(isTaskCompleted))
            , mThreadLock(threadLock)
            , mThreadSignal(threadSignal)
        {}

    private:

        std::shared_ptr<bool> mIsTaskCompleted;

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

    TaskCompletionIndicator QueueTask(Task && task)
    {
        std::unique_lock<std::mutex> lock(mThreadLock);

        auto & queuedTask = mTaskQueue.emplace_back(std::move(task));

        TaskCompletionIndicator taskCompletionIndicator(
            queuedTask.isTaskCompleted,
            mThreadLock,
            mThreadSignal);

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
        result.Wait();
    }

private:

    void ThreadLoop();

private:

    struct QueuedTask
    {
        Task task;
        std::shared_ptr<bool> isTaskCompleted;

        QueuedTask()
            : task()
            , isTaskCompleted()
        {
        }

        QueuedTask(Task && _task)
            : task(std::move(_task))
            , isTaskCompleted(std::make_shared<bool>(false))
        {}
    };

private:

    std::thread mThread;

    std::mutex mThreadLock;
    std::condition_variable mThreadSignal; // Just one, as each of {main thread, worker thread} can't be waiting and signaling at the same time

    std::deque<QueuedTask> mTaskQueue;

    bool mIsStop;
};