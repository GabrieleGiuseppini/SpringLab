/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringIntrinsicsLayoutOptimizationSimulator.h"

#include "CacheModel.h"
#include "Log.h"

ILayoutOptimizer::LayoutRemap FSBySpringIntrinsicsLayoutOptimizer::Remap(
    std::vector<ObjectBuildPoint> const & points,
    std::vector<ObjectBuildSpring> const & springs) const
{
    auto idempotentPointRemap = IdempotentLayoutOptimizer::MakePointRemap(points);
    auto idempotentSpringRemap = IdempotentLayoutOptimizer::MakeSpringRemap(springs);

    //
    // Calculate initial ACMR
    // 

    
    float const initialAcmr = CalculateACMR(points, springs, idempotentPointRemap, idempotentSpringRemap);

    LogMessage("FSBySpringIntrinsicsLayoutOptimizer: initial ACMR = ", initialAcmr);

    // TODOHERE

    return LayoutRemap(
        std::move(idempotentPointRemap),
        std::move(idempotentSpringRemap));
}

float FSBySpringIntrinsicsLayoutOptimizer::CalculateACMR(
    std::vector<ObjectBuildPoint> const & /*points*/,
    std::vector<ObjectBuildSpring> const & springs,
    std::vector<ElementIndex> const & pointRemap,
    std::vector<ElementIndex> const & springRemap) const
{
    //
    // Use the same access pattern as our algorithm: for each spring, access the two endpoints
    //

    // Point cache: encompasses all point-based buffers
    CacheModel<ElementIndex, 16> pointCache;

    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    for (ElementIndex oldS : springRemap)
    {
        if (pointCache.Visit(pointRemap[springs[oldS].PointAIndex]))
            ++cacheHits;
        else
            ++cacheMisses;

        if (pointCache.Visit(pointRemap[springs[oldS].PointBIndex]))
            ++cacheHits;
        else
            ++cacheMisses;
    }

    return static_cast<float>(cacheMisses) / static_cast<float>(cacheHits + cacheMisses);
}
