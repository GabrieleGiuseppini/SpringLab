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
    auto idempotentPointRemap = IndexRemap::MakeIdempotent(points.size());
    auto idempotentSpringRemap = IndexRemap::MakeIdempotent(springs.size());

    //
    // Calculate initial ACMR
    // 
    
    float const initialAcmr = CalculateACMR(points, springs, idempotentPointRemap, idempotentSpringRemap);
    LogMessage("FSBySpringIntrinsicsLayoutOptimizer: initial ACMR = ", initialAcmr);

    //
    // Optimize
    //

    // TODOTEST
    auto const optimalLayout = Optimize1(pointMatrix, points, springs);
    //auto const optimalLayout = Optimize1(pointMatrix, points, springs);

    // TODOTEST
    for (ElementIndex s  = 0; s < springs.size() && s < 120; ++s)
    {
        LogMessage(springs[optimalLayout.SpringRemap.NewToOld(s)].PointAIndex, " <-> ", springs[optimalLayout.SpringRemap.NewToOld(s)].PointBIndex);
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
    IndexRemap const & pointRemap,
    IndexRemap const & springRemap) const
{
    //
    // Use the same access pattern as our algorithm: for each spring, access the two endpoints
    //

    // Point cache: encompasses all point-based buffers
    MyCacheModel pointCache;

    size_t cacheHits = 0;
    size_t cacheMisses = 0;
    for (ElementIndex oldS : springRemap.GetOldIndices())
    {
        if (pointCache.Visit(pointRemap.OldToNew(springs[oldS].PointAIndex)))
            ++cacheHits;
        else
            ++cacheMisses;

        if (pointCache.Visit(pointRemap.OldToNew(springs[oldS].PointBIndex)))
            ++cacheHits;
        else
            ++cacheMisses;
    }

    return static_cast<float>(cacheMisses) / static_cast<float>(cacheHits + cacheMisses);
}

ILayoutOptimizer::LayoutRemap FSBySpringIntrinsicsLayoutOptimizer::Optimize1(
    ObjectBuildPointIndexMatrix const & /*pointMatrix*/,
    std::vector<ObjectBuildPoint> const & points,
    std::vector<ObjectBuildSpring> const & springs) const
{
    IndexRemap optimalPointRemap = IndexRemap::MakeIdempotent(points.size());
    IndexRemap optimalSpringRemap(springs.size());

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
        optimalSpringRemap.AddOld(*sIndex);

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
    IndexRemap optimalPointRemap(points.size());
    IndexRemap optimalSpringRemap(springs.size());

    std::vector<bool> remappedPointMask(points.size(), false);
    std::vector<bool> remappedSpringMask(springs.size(), false);

    // Build Point Pair -> Spring table
    PointPairToIndexMap pointPairToSpringMap;
    for (ElementIndex s = 0; s < springs.size(); ++s)
    {
        pointPairToSpringMap.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(springs[s].PointAIndex, springs[s].PointBIndex),
            std::forward_as_tuple(s));
    }

    //
    // Find all squares from left-bottom
    //

    for (int y = 0; y < pointMatrix.height; ++y)
    {
        for (int x = 0; x < pointMatrix.width; ++x)
        {
            // Check if a square
            if (pointMatrix[{x, y}] 
                && x < pointMatrix.width - 1 && pointMatrix[{x + 1, y}]
                && y < pointMatrix.height - 1 && pointMatrix[{x + 1, y + 1}]
                && pointMatrix[{x, y + 1}])
            {
                // Do this square's points

                ElementIndex const lb = *pointMatrix[{x, y}];
                if (!remappedPointMask[lb])
                {
                    optimalPointRemap.AddOld(lb);
                    remappedPointMask[lb] = true;
                }

                ElementIndex const rb = *pointMatrix[{x + 1, y}];
                if (!remappedPointMask[rb])
                {
                    optimalPointRemap.AddOld(rb);
                    remappedPointMask[rb] = true;
                }

                ElementIndex const rt = *pointMatrix[{x + 1, y + 1}];
                if (!remappedPointMask[rt])
                {
                    optimalPointRemap.AddOld(rt);
                    remappedPointMask[rt] = true;
                }

                ElementIndex const lt = *pointMatrix[{x, y + 1}];
                if (!remappedPointMask[lt])
                {
                    optimalPointRemap.AddOld(lt);
                    remappedPointMask[lt] = true;
                }

                // Do all springs across this square's points

                for (auto const & pair : { 
                    PointPair(lb, rb),  // _
                    PointPair(lb, rt),  // /
                    PointPair(lb, lt),  // |
                    PointPair(lt, rb),  // \ 
                    PointPair(rb, rt),  //  |
                    PointPair(lt, rt)   // -
                    })
                {
                    if (auto const springIt = pointPairToSpringMap.find(pair);
                        springIt != pointPairToSpringMap.cend() && !remappedSpringMask[springIt->second])
                    {
                        optimalSpringRemap.AddOld(springIt->second);
                        remappedSpringMask[springIt->second] = true;
                    }
                }
            }
        }
    }

    //
    // Map leftovers now
    //

    LogMessage("LayoutOptimizer: ", std::count(remappedPointMask.cbegin(), remappedPointMask.cend(), false), " leftover points, ",
        std::count(remappedSpringMask.cbegin(), remappedSpringMask.cend(), false), " leftover springs");

    for (ElementIndex p = 0; p < points.size(); ++p)
    {
        if (!remappedPointMask[p])
        {
            optimalPointRemap.AddOld(p);
        }
    }

    for (ElementIndex s = 0; s < springs.size(); ++s)
    {
        if (!remappedSpringMask[s])
        {
            optimalSpringRemap.AddOld(s);
        }
    }

    return LayoutRemap(
        std::move(optimalPointRemap),
        std::move(optimalSpringRemap));
}
