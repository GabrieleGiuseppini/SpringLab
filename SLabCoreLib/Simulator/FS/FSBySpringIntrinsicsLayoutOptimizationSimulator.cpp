/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-19
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringIntrinsicsLayoutOptimizationSimulator.h"

#include "CacheModel.h"
#include "Log.h"

#include <limits>
#include <optional>

using MyCacheModel = CacheModel<1, 64, vec2f>;
size_t constexpr Lookead = 0;

int ProbeCacheMisses(
    ObjectBuildSpring const & spring,
    MyCacheModel const & pointCache)
{
    //
    // Use the same access pattern as our algorithm: for each spring, access the two endpoints
    //

    int cacheMisses = 0;

    if (!pointCache.IsCached(spring.PointAIndex))
        ++cacheMisses;

    if (!pointCache.IsCached(spring.PointBIndex))
        ++cacheMisses;

    return cacheMisses;
}

// Returns spring index, # cache misses in the path long Lookahead starting at the spring index
template<size_t RemaniningLookahead>
std::tuple<std::optional<ElementIndex>, int> FindNextBestSpring(
    std::vector<ObjectBuildSpring> const & springs,
    MyCacheModel const & currentPointCache,
    std::vector<bool> & visitedSprings)
{
    //
    // Find first non-visited spring with minimal cache misses in the next looakead steps
    //

    std::optional<ElementIndex> bestSpring;
    int lowestCacheMissCount = std::numeric_limits<int>::max();

    for (ElementIndex sIndex = 0; sIndex < springs.size(); ++sIndex)
    {
        if (!visitedSprings[sIndex])
        {
            // Calculate score from this spring

            int cacheMissCount = ProbeCacheMisses(springs[sIndex], currentPointCache);

            if constexpr (RemaniningLookahead > 0)
            {
                // Go down recursively

                // Show cache as it would be after thing visit
                MyCacheModel localPointCache = currentPointCache;
                localPointCache.Visit(springs[sIndex].PointAIndex);
                localPointCache.Visit(springs[sIndex].PointBIndex);
                
                visitedSprings[sIndex] = true;

                auto const [nextSpringIndex, postfixCacheMissCount] = FindNextBestSpring<RemaniningLookahead - 1>(springs, localPointCache, visitedSprings);

                if (nextSpringIndex)
                {
                    cacheMissCount += postfixCacheMissCount;
                }

                visitedSprings[sIndex] = false;
            }

            if (cacheMissCount < lowestCacheMissCount)
            {
                // This is a winner
                lowestCacheMissCount = cacheMissCount;
                bestSpring = sIndex;

                // Shorcut
                if (lowestCacheMissCount == 0)
                {
                    // Can't get better than this
                    break;
                }
            }
        }
    }

    return { bestSpring, lowestCacheMissCount };
}

ILayoutOptimizer::LayoutRemap FSBySpringIntrinsicsLayoutOptimizer::Remap(
    ObjectBuildPointIndexMatrix const & pointMatrix,
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

    //
    // Optimize
    //

    auto const optimalLayout = Optimize1(pointMatrix, points, springs);

    // TODOTEST
    for (size_t s  = 0; s < optimalLayout.SpringRemap.size() && s < 120; ++s)
    {
        LogMessage(springs[s].PointAIndex, " <-> ", springs[s].PointBIndex);
    }

    //
    // Recalculate ACMR
    //

    float const finalAcmr = CalculateACMR(points, springs, optimalLayout.PointRemap, optimalLayout.SpringRemap);
    LogMessage("FSBySpringIntrinsicsLayoutOptimizer: final ACMR = ", finalAcmr);

    return optimalLayout;
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
    MyCacheModel pointCache;

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

ILayoutOptimizer::LayoutRemap FSBySpringIntrinsicsLayoutOptimizer::Optimize1(
    ObjectBuildPointIndexMatrix const & pointMatrix,
    std::vector<ObjectBuildPoint> const & points,
    std::vector<ObjectBuildSpring> const & springs) const
{
    std::vector<ElementIndex> optimalPointRemap = IdempotentLayoutOptimizer::MakePointRemap(points);
    std::vector<ElementIndex> optimalSpringRemap;

    MyCacheModel pointCache;

    std::vector<bool> visitedSprings(springs.size(), false);

    for (ElementCount nIteration = 0; nIteration < springs.size(); ++nIteration)
    {
        // There's still a non-visited spring
        assert(std::find(visitedSprings.cbegin(), visitedSprings.cend(), false) != visitedSprings.cend());

        // Find next best spring
        auto const [sIndex, _] = FindNextBestSpring<Lookead>(springs, pointCache, visitedSprings);

        assert(sIndex);

        // Store remap
        optimalSpringRemap.emplace_back(*sIndex);

        // Visit spring
        pointCache.Visit(springs[*sIndex].PointAIndex);
        pointCache.Visit(springs[*sIndex].PointBIndex);
        assert(visitedSprings[*sIndex] == false);
        visitedSprings[*sIndex] = true;
    }

    return LayoutRemap(
        std::move(optimalPointRemap),
        std::move(optimalSpringRemap));
}

ILayoutOptimizer::LayoutRemap FSBySpringIntrinsicsLayoutOptimizer::Optimize2(
    ObjectBuildPointIndexMatrix const & pointMatrix,
    std::vector<ObjectBuildPoint> const & points,
    std::vector<ObjectBuildSpring> const & springs) const
{
    std::vector<ElementIndex> optimalPointRemap = IdempotentLayoutOptimizer::MakePointRemap(points);
    std::vector<ElementIndex> optimalSpringRemap;

    // TODOHERE

    return LayoutRemap(
        std::move(optimalPointRemap),
        std::move(optimalSpringRemap));
}
