/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringStructuralIntrinsicsMTSimulator.h"

#include "Log.h"

#include <cassert>

FSBySpringStructuralIntrinsicsMTSimulator::FSBySpringStructuralIntrinsicsMTSimulator(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & threadManager)
    : FSBySpringStructuralIntrinsicsSimulator(
        object,
        simulationParameters,
        threadManager)
{
    // CreateState() on base has been called; our turn now
    InitializeThreadingState(object, threadManager);
}

void FSBySpringStructuralIntrinsicsMTSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & threadManager)
{
    FSBySpringStructuralIntrinsicsSimulator::CreateState(object, simulationParameters, threadManager);

    InitializeThreadingState(object, threadManager);
}

void FSBySpringStructuralIntrinsicsMTSimulator::InitializeThreadingState(
    Object const & object,
    ThreadManager const & threadManager)
{
    // Clear threading state
    mSpringRelaxationTasks.clear();
    mAdditionalPointSpringForceBuffers.clear();

    // Number of 4-spring blocks per thread, assuming we use maximum threads
    ElementCount const numberOfSprings = static_cast<ElementCount>(object.GetSprings().GetElementCount());
    ElementCount const numberOfFourSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(threadManager.GetMaxSimulationParallelism()) * 4);

    size_t parallelism;
    if (numberOfFourSpringsPerThread > 0)
    {
        parallelism = threadManager.GetMaxSimulationParallelism();

        ElementIndex springStart = 0;
        for (size_t t = 0; t < parallelism; ++t)
        {
            ElementIndex const springEnd = (t < parallelism - 1)
                ? springStart + numberOfFourSpringsPerThread * 4
                : numberOfSprings;

            if (t == 0)
            {
                // Use the "official" buffer for the first thread

                mSpringRelaxationTasks.emplace_back(
                    [this, &object, &buffer = mPointSpringForceBuffer, springStart, springEnd]()
                    {
                        FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForces(
                            object,
                            buffer.data(),
                            springStart,
                            springEnd);
                    });
            }
            else
            {
                // Create helper buffer for this thread
                mAdditionalPointSpringForceBuffers.emplace_back(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero());

                mSpringRelaxationTasks.emplace_back(
                    [this, &object, bufIndex = mAdditionalPointSpringForceBuffers.size() - 1, springStart, springEnd]()
                    {
                        mAdditionalPointSpringForceBuffers[bufIndex].fill(vec2f::zero());

                FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForces(
                    object,
                    mAdditionalPointSpringForceBuffers[bufIndex].data(),
                    springStart,
                    springEnd);
                    });
            }

            springStart = springEnd;
        }
    }
    else
    {
        // Not enough, use just one thread
        parallelism = 1;

        vec2f * restrict pointSpringForceBuffer = mPointSpringForceBuffer.data();

        mSpringRelaxationTasks.emplace_back(
            [this, &object, pointSpringForceBuffer, springStart = 0, springEnd = numberOfSprings]()
            {
                FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForces(
                    object,
                    pointSpringForceBuffer,
                    springStart,
                    springEnd);
            });
    }

    LogMessage("FSBySpringStructuralIntrinsicsMTSimulator: numSprings=", object.GetSprings().GetElementCount(), " springPerfectSquareCount=", mSpringPerfectSquareCount,
        " numberOfFourSpringsPerThread=", numberOfFourSpringsPerThread, " numThreads=", parallelism);
}

void FSBySpringStructuralIntrinsicsMTSimulator::ApplySpringsForces(
    Object const & object,
    ThreadManager & threadManager)
{
    //
    // Run algo
    //

    threadManager.GetSimulationThreadPool().Run(mSpringRelaxationTasks);

    //
    // Add additional spring forces to main spring force buffer
    //

#if !FS_IS_ARCHITECTURE_X86_32() && !FS_IS_ARCHITECTURE_X86_64()
#error Unsupported Architecture
#endif    
    static_assert(vectorization_float_count<int> >= 4);

    vec2f * const restrict pointSpringForceBuffer = mPointSpringForceBuffer.data();
    ElementCount const pointCount = object.GetPoints().GetElementCount();
    assert(pointCount % vectorization_float_count<ElementCount> == 0);
    for (ElementIndex p = 0; p < pointCount; p += 4)
    {
        __m128 pointForce1 = _mm_load_ps(reinterpret_cast<float const * restrict>(pointSpringForceBuffer + p));
        __m128 pointForce2 = _mm_load_ps(reinterpret_cast<float const * restrict>(pointSpringForceBuffer + p + 2));
        for (size_t a = 0; a < mAdditionalPointSpringForceBuffers.size(); ++a)
        {
            __m128 const addlForce1 = _mm_load_ps(reinterpret_cast<float const * restrict>(&(mAdditionalPointSpringForceBuffers[a][p])));
            __m128 const addlForce2 = _mm_load_ps(reinterpret_cast<float const * restrict>(&(mAdditionalPointSpringForceBuffers[a][p + 2])));
            pointForce1 = _mm_add_ps(pointForce1, addlForce1);
            pointForce2 = _mm_add_ps(pointForce2, addlForce2);
        }

        _mm_store_ps(reinterpret_cast<float * restrict>(pointSpringForceBuffer + p), pointForce1);
        _mm_store_ps(reinterpret_cast<float * restrict>(pointSpringForceBuffer + p + 2), pointForce2);
    }
}
