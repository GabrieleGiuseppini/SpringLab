/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "FSBySpringIntrinsicsSimulator.h"

#include "Simulator/Common/ISimulator.h"

#include "IndexRemap.h"
#include "ILayoutOptimizer.h"

#include <memory>
#include <string>
#include <utility>

/*
 * Simulator implementing the same spring relaxation algorithm
 * as in the "By Spring" - "With Intrinsics" simulator, but with layout optimization
 * to minimize cache misses
 */

class FSBySpringIntrinsicsLayoutOptimizer;

class FSBySpringIntrinsicsLayoutOptimizationSimulator : public FSBySpringIntrinsicsSimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "FS 11 - By Spring - Instrinsics - Layout Optimized";
    }

    using layout_optimizer = FSBySpringIntrinsicsLayoutOptimizer;

public:

    FSBySpringIntrinsicsLayoutOptimizationSimulator(
        Object const & object,
        SimulationParameters const & simulationParameters)
        : FSBySpringIntrinsicsSimulator(
            object,
            simulationParameters)
    {}
};

class FSBySpringIntrinsicsLayoutOptimizer : public ILayoutOptimizer
{
public:

    LayoutRemap Remap(
        ObjectBuildPointIndexMatrix const & pointMatrix,
        std::vector<ObjectBuildPoint> const & points,
        std::vector<ObjectBuildSpring> const & springs) const override;

private:

    float CalculateACMR(
        std::vector<ObjectBuildPoint> const & points,
        std::vector<ObjectBuildSpring> const & springs,
        IndexRemap const & pointRemap,
        IndexRemap const & springRemap) const;

    LayoutRemap Optimize1(
        ObjectBuildPointIndexMatrix const & pointMatrix,
        std::vector<ObjectBuildPoint> const & points,
        std::vector<ObjectBuildSpring> const & springs) const;

    LayoutRemap Optimize2(
        ObjectBuildPointIndexMatrix const & pointMatrix,
        std::vector<ObjectBuildPoint> const & points,
        std::vector<ObjectBuildSpring> const & springs) const;
};