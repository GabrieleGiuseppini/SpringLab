/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2022-06-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <cstdint>
#include <memory>
#include <mutex>
#include <set>

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

    bool GetSimulationParallelism() const;

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

    void AffinitizeThisThread();

private:

    bool mIsRenderingMultithreaded; // Calculated via init args and hardware concurrency; never changes
    size_t mMaxSimulationParallelism; // Calculated via init args and hardware concurrency; never changes

    std::unique_ptr<ThreadPool> mSimulationThreadPool;

    // Affinitization
    using CpuIdType = std::uint8_t;    
    std::set<CpuIdType> mAllocatedProcessors;
    std::mutex mAllocatedProcessorsMutex;
};

#include "ThreadPool.h"