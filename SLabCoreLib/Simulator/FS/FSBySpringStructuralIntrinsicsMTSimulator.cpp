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
    SimulationParameters const & simulationParameters)
    : FSBySpringStructuralIntrinsicsSimulator(
        object,
        simulationParameters)
{
    // CreateState() from base has been called; our turn now
    CreateState(object, simulationParameters);
}

void FSBySpringStructuralIntrinsicsMTSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    FSBySpringStructuralIntrinsicsSimulator::CreateState(object, simulationParameters);

    // Clear threading state
    mThreadPool.reset();
    mSpringRelaxationTasks.clear();
    mAdditionalPointSpringForceBuffers.clear();

    // Number of 4-spring blocks per thread
    assert(simulationParameters.Common.NumberOfThreads > 0);
    ElementCount const numberOfSprings = static_cast<ElementCount>(object.GetSprings().GetElementCount());
    ElementCount const numberOfFourSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(simulationParameters.Common.NumberOfThreads) * 4);

    size_t numThreads;
    if (numberOfFourSpringsPerThread > 0)
    {
        numThreads = simulationParameters.Common.NumberOfThreads;

        ElementIndex springStart = 0;
        for (size_t t = 0; t < numThreads; ++t)
        {
            vec2f * restrict pointSpringForceBuffer;
            if (t == 0)
            {
                // Use the "official" buffer for the first thread
                pointSpringForceBuffer = mPointSpringForceBuffer.data();
            }
            else
            {
                // Create helper buffer for this thread
                mAdditionalPointSpringForceBuffers.emplace_back(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero());
                pointSpringForceBuffer = mAdditionalPointSpringForceBuffers.back().data();
            }

            ElementIndex springEnd = (t < numThreads - 1)
                ? springStart + numberOfFourSpringsPerThread * 4
                : numberOfSprings;

            mSpringRelaxationTasks.emplace_back(
                [this, &object, pointSpringForceBuffer, springStart, springEnd]()
                {
                    FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForces(
                        object,
                        pointSpringForceBuffer,
                        springStart,
                        springEnd);
                });
            
            springStart = springEnd;
        }
    }
    else
    {
        // Not enough, use one thread
        numThreads = 1;

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
        " numberOfFourSpringsPerThread=", numberOfFourSpringsPerThread, " numThreads=", numThreads);

    mThreadPool = std::make_unique<TaskThreadPool>(numThreads);
}

void FSBySpringStructuralIntrinsicsMTSimulator::ApplySpringsForces(
    Object const & object)
{
    //
    // Run algo
    //

    mThreadPool->Run(mSpringRelaxationTasks);

    //
    // Add additional spring forces to main spring force buffer
    //

    vec2f * restrict pointSpringForceBuffer = mPointSpringForceBuffer.data();
    ElementCount const pointCount = object.GetPoints().GetElementCount();
    for (ElementIndex p = 0; p < pointCount; ++p)
    {
        vec2f springForce = pointSpringForceBuffer[p];
        for (size_t a = 0; a < mAdditionalPointSpringForceBuffers.size(); ++a)
        {
            springForce += mAdditionalPointSpringForceBuffers[a][p];
        }

        pointSpringForceBuffer[p] = springForce;
    }

    //
    // Clear additional spring force buffers
    //

    for (auto & buf : mAdditionalPointSpringForceBuffers)
    {
        buf.fill(vec2f::zero());
    }
}