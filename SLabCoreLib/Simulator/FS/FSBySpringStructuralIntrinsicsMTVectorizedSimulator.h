/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-15
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
 * but with multiple threads *and* with vectorized integration.
 */

class FSBySpringStructuralIntrinsicsMTVectorizedSimulator : public FSBySpringStructuralIntrinsicsSimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "FS 14 - By Spring - Structural Instrinsics - MT - Vectorized";
    }

    using layout_optimizer = FSBySpringStructuralIntrinsicsLayoutOptimizer;

public:

    FSBySpringStructuralIntrinsicsMTVectorizedSimulator(
        Object const & object,
        SimulationParameters const & simulationParameters);

private:

    virtual void CreateState(
        Object const & object,
        SimulationParameters const & simulationParameters) override;

    void ApplySpringsForces(
        Object const & object) override;

    void IntegrateAndResetSpringForces(
        Object & object,
        SimulationParameters const & simulationParameters) override;

    void IntegrateAndResetSpringForces_1(
        Object & object,
        SimulationParameters const & simulationParameters);

    void IntegrateAndResetSpringForces_2(
        Object & object,
        SimulationParameters const & simulationParameters);

    void IntegrateAndResetSpringForces_4(
        Object & object,
        SimulationParameters const & simulationParameters);

    void IntegrateAndResetSpringForces_N(
        Object & object,
        SimulationParameters const & simulationParameters);

private:

    std::unique_ptr<TaskThreadPool> mThreadPool;
    std::vector<typename TaskThreadPool::Task> mSpringRelaxationTasks;

    std::vector<Buffer<vec2f>> mPointSpringForceBuffers;
};
