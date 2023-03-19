/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringIntrinsicsLayoutOptimizationSimulator.h"

ILayoutOptimizer::LayoutRemap FSBySpringIntrinsicsLayoutOptimizer::Remap(
    std::vector<ObjectBuildPoint> const & points,
    std::vector<ObjectBuildSpring> const & springs) const
{
    // TODOHERE
    return LayoutRemap(
        IdempotentLayoutOptimizer::MakePointRemap(points),
        IdempotentLayoutOptimizer::MakeSpringRemap(springs));
}