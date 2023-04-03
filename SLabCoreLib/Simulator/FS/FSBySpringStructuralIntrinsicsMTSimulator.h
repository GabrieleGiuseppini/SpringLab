/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FSBySpringStructuralIntrinsicsSimulator.h"

#include "Buffer.h"
#include "Simulator/Common/ISimulator.h"
#include "TaskThreadPool.h"
#include "Vectors.h"

#include <memory>
#include <string>
#include <vector>

/*
 * Simulator implementing the same spring relaxation algorithm
 * as in the "By Spring" - "Structural Intrinsics" simulator, 
 * but with multiple threads.
 */

class FSBySpringStructuralIntrinsicsMTSimulator : public FSBySpringStructuralIntrinsicsSimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "FS 13 - By Spring - Structural Instrinsics - MT";
    }

    using layout_optimizer = FSBySpringStructuralIntrinsicsLayoutOptimizer;

public:

    FSBySpringStructuralIntrinsicsMTSimulator(
        Object const & object,
        SimulationParameters const & simulationParameters);

private:

    virtual void CreateState(
        Object const & object,
        SimulationParameters const & simulationParameters) override;

    void ApplySpringsForces(
        Object const & object) override;

private:

    std::unique_ptr<TaskThreadPool> mThreadPool;
    std::vector<TaskThreadPool::Task> mSpringRelaxationTasks;

    std::vector<Buffer<vec2f>> mAdditionalPointSpringForceBuffers; // One less the number of threads
};
