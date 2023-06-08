/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FSBySpringStructuralIntrinsicsSimulator.h"

#include "Buffer.h"
#include "Simulator/Common/ISimulator.h"
#include "Vectors.h"

#include <memory>
#include <string>
#include <vector>

/*
 * Simulator implementing the same spring relaxation algorithm
 * as in the "By Spring" - "Structural Intrinsics" simulator, 
 * but with no intrinsics (for wildcard architecture), with 
 * multiple threads, *and* with vectorized integration.
 */

class FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator : public FSBySpringStructuralIntrinsicsSimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "FS 15 - By Spring - Structural PseudoInstrinsics - MT - Vectorized";
    }

    using layout_optimizer = FSBySpringStructuralIntrinsicsLayoutOptimizer;

public:

    FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator(
        Object const & object,
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager);

private:

    virtual void CreateState(
        Object const & object,
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager) override;

    void ApplySpringsForces(
        Object const & object,
        ThreadManager & threadManager) override;

    void ApplySpringsForcesPseudoVectorized(
        Object const & object,
        vec2f * restrict pointSpringForceBuffer,
        ElementIndex startSpringIndex,
        ElementCount endSpringIndex);  // Excluded

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

    std::vector<typename ThreadPool::Task> mSpringRelaxationTasks;

    std::vector<Buffer<vec2f>> mPointSpringForceBuffers;
    std::vector<float * restrict> mPointSpringForceBuffersVectorized;
};
