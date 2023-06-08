/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2022-06-19
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "ThreadManager.h"

#include "FloatingPoint.h"
#include "Log.h"
#include "SysSpecifics.h"

#include <cassert>

size_t ThreadManager::GetNumberOfProcessors()
{
    return std::max(
        size_t(1),
        static_cast<size_t>(std::thread::hardware_concurrency()));
}

ThreadManager::ThreadManager(
    bool doForceNoMultithreadedRendering,
    size_t maxInitialParallelism)
{
    auto const numberOfProcessors = GetNumberOfProcessors();

    // Calculate whether we'll have multi-threaded rendering
    mIsRenderingMultithreaded = (numberOfProcessors > 1) && (!doForceNoMultithreadedRendering);

    // Calculate max parallelism

    int availableThreads = static_cast<int>(numberOfProcessors);    
    if (mIsRenderingMultithreaded)
        --availableThreads;

    mMaxSimulationParallelism = std::max(availableThreads, 1);

    // Setup current parallelism
    SetSimulationParallelism(std::min(mMaxSimulationParallelism, maxInitialParallelism));

    LogMessage("ThreadManager: isRenderingMultithreaded=", (GetIsRenderingMultithreaded() ? "YES" : "NO"),
        " maxSimulationParallelism=", GetMaxSimulationParallelism(),
        " simulationParallism=", GetSimulationParallelism());
}

bool ThreadManager::GetSimulationParallelism() const
{
    // TODOTEST
    LogMessage("TODOTEST1: ThreadManager::GetSimulationParallelism: ", mSimulationThreadPool->GetParallelism());
    return mSimulationThreadPool->GetParallelism();
}

void ThreadManager::SetSimulationParallelism(size_t parallelism)
{
    assert(parallelism >= 1 && parallelism <= GetMaxSimulationParallelism());

    //
    // (Re-)create thread pool
    //

    mSimulationThreadPool.reset();
    mAllocatedProcessors.clear();

    LogMessage("ThreadManager: creating simulation thread pool with parallelism=", parallelism);
    mSimulationThreadPool = std::make_unique<ThreadPool>(parallelism, *this);
}

ThreadPool & ThreadManager::GetSimulationThreadPool()
{
    return *mSimulationThreadPool;
}

void ThreadManager::InitializeThisThread()
{
    //
    // Affinitize thread
    //

    // TODOTEST
    //AffinitizeThisThread();

    //
    // Initialize floating point handling
    //

    // Avoid denormal numbers for very small quantities
    EnableFloatingPointFlushToZero();

#ifdef FLOATING_POINT_CHECKS
    EnableFloatingPointExceptions();
#endif
}

void ThreadManager::AffinitizeThisThread()
{
    if (GetNumberOfProcessors() > 1)
    {
#if FS_IS_OS_WINDOWS()

        // Pick a processor that we haven't already assigned, among those 
        // allowed by GetProcessAffinityMask()

        HANDLE const hThisProcess = GetCurrentProcess();
        assert(hThisProcess != NULL);

        DWORD_PTR dwProcessAffinityMask = 0;
        DWORD_PTR dwSystemAffinityMask = 0;
        BOOL res = ::GetProcessAffinityMask(hThisProcess, &dwProcessAffinityMask, &dwSystemAffinityMask);
        if (!res)
        {
            DWORD const dwLastError = ::GetLastError();
            LogMessage("Error invoking GetProcessAffinityMask: ", dwLastError);
            return;
        }

        LogMessage("GetProcessAffinityMask: proc=", dwProcessAffinityMask, " system=", dwSystemAffinityMask);

        // Visit all CPU/bits until one is found
        {
            std::lock_guard<std::mutex> lock(mAllocatedProcessorsMutex);

            DWORD dwCpuId = 0;
            DWORD dwCpuMask = 1;
            for (; dwCpuId < sizeof(dwProcessAffinityMask) * 8 && dwCpuId < std::numeric_limits<CpuIdType>::max(); ++dwCpuId, dwCpuMask = (dwCpuMask << 1))
            {
                CpuIdType const cpuId = static_cast<CpuIdType>(dwCpuId);
                if (dwProcessAffinityMask & dwCpuMask // Allowed by process affinity mask
                    && mAllocatedProcessors.count(cpuId) == 0) // Not used already
                {
                    //
                    // Found!
                    //

                    // Set thread affinity mask
                    DWORD_PTR const dwNewThreadAffinityMask = dwCpuMask;
                    DWORD_PTR dwOldThreadAffinityMask = ::SetThreadAffinityMask(GetCurrentThread(), dwNewThreadAffinityMask);

                    LogMessage("SetThreadAffinityMask(", dwNewThreadAffinityMask, " for CPU ", size_t(cpuId), ") returned ", dwOldThreadAffinityMask);

                    if (dwOldThreadAffinityMask != 0)
                    {
                        // We're done

                        // Allocate this CPU
                        mAllocatedProcessors.insert(cpuId);

                        return;
                    }
                }
            }
        }

        // If we're here, no luck
        LogMessage("WARNING: couldn't find a CPU to affinitize this thread on");
#endif
    }
}