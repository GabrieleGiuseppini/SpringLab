/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2022-06-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <cstdint>
#include <memory>

class ThreadPool;

class ThreadManager final
{
public:

    static size_t GetNumberOfProcessors();

    ThreadManager(
        bool doForceNoMultithreadedRendering,
        size_t maxInitialParallelism);

    bool GetIsRenderingMultithreaded() const
    {
        return mIsRenderingMultithreaded;
    }

    size_t GetSimulationParallelism() const;

    void SetSimulationParallelism(size_t parallelism);

    size_t GetMinSimulationParallelism() const
    {
        return 1;
    }

    size_t GetMaxSimulationParallelism() const
    {
        return mMaxSimulationParallelism;
    }

    ThreadPool & GetSimulationThreadPool();

    void InitializeThisThread();

private:

    bool mIsRenderingMultithreaded; // Calculated via init args and hardware concurrency; never changes
    size_t mMaxSimulationParallelism; // Calculated via init args and hardware concurrency; never changes

    std::unique_ptr<ThreadPool> mSimulationThreadPool;
};

#include "ThreadPool.h"