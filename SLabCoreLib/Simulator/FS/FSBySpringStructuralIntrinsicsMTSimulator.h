/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FSBySpringStructuralIntrinsicsSimulator.h"

#include "Simulator/Common/ISimulator.h"

#include <memory>
#include <string>
#include <utility>

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
        SimulationParameters const & simulationParameters)
        : FSBySpringStructuralIntrinsicsSimulator(
            object,
            simulationParameters)
    {}
};
