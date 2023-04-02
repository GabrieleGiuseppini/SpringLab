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
    mSpringSpansPerThread.clear();

    // Number of 4-spring blocks per thread
    assert(simulationParameters.Common.NumberOfThreads > 0);
    ElementCount const numberOfSprings = static_cast<ElementCount>(object.GetSprings().GetElementCount());
    ElementCount const numberOfFourSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(simulationParameters.Common.NumberOfThreads) * 4);

    size_t numThreads;
    if (numberOfFourSpringsPerThread > 0)
    {
        numThreads = simulationParameters.Common.NumberOfThreads;

        ElementCount consumedSprings = 0;
        for (size_t t = 0; t < numThreads - 1; ++t)
        {
            mSpringSpansPerThread.emplace_back(consumedSprings, consumedSprings + numberOfFourSpringsPerThread * 4);
            consumedSprings += numberOfFourSpringsPerThread * 4;
        }

        assert(consumedSprings < numberOfSprings);
        mSpringSpansPerThread.emplace_back(consumedSprings, consumedSprings + (numberOfSprings - consumedSprings));
    }
    else
    {
        // Not enough, use one thread
        numThreads = 1;

        mSpringSpansPerThread.emplace_back(0, numberOfSprings);
    }

    LogMessage("FSBySpringStructuralIntrinsicsMTSimulator: numSprings=", object.GetSprings().GetElementCount(), " springPerfectSquareCount=", mSpringPerfectSquareCount,
        " numberOfFourSpringsPerThread=", numberOfFourSpringsPerThread, " numThreads=", numThreads);
    for (size_t t = 0; t < numThreads; ++t)
    {
        LogMessage("     Thread ", t, ": ", std::get<0>(mSpringSpansPerThread[t]), " -> ", std::get<1>(mSpringSpansPerThread[t]));
    }

    mThreadPool = std::make_unique<TaskThreadPool>(numThreads);
}

void FSBySpringStructuralIntrinsicsMTSimulator::ApplySpringsForces(
    Object const & object)
{
    // TODOHERE
    FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForces(
        object,
        mPointSpringForceBuffer.data(),
        0,
        object.GetSprings().GetElementCount());
}