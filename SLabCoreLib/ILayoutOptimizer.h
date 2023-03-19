/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2023-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ObjectBuilderTypes.h"
#include "SLabTypes.h"

#include <numeric>
#include <vector>

class ILayoutOptimizer
{
public:

    virtual ~ILayoutOptimizer() = default;

    struct LayoutRemap
    {
        std::vector<ElementIndex> PointRemap;
        std::vector<ElementIndex> SpringRemap;

        LayoutRemap(
            std::vector<ElementIndex> && pointRemap,
            std::vector<ElementIndex> && springRemap)
            : PointRemap(std::move(pointRemap))
            , SpringRemap(std::move(springRemap))
        {}
    };

    virtual LayoutRemap Remap(
        std::vector<ObjectBuildPoint> const & points,
        std::vector<ObjectBuildSpring> const & springs) const = 0;
};

/*
 * A layout optimizer that does not changes the layout.
 * Used as defaul lyout optimizer.
 */
class IdempotentLayoutOptimizer final : public ILayoutOptimizer
{
public:

    LayoutRemap Remap(
        std::vector<ObjectBuildPoint> const & points,
        std::vector<ObjectBuildSpring> const & springs) const override
    {
        return LayoutRemap(
            MakePointRemap(points),
            MakeSpringRemap(springs));
    }

    static std::vector<ElementIndex> MakePointRemap(std::vector<ObjectBuildPoint> const & points)
    {
        std::vector<ElementIndex> remap(points.size());
        std::iota(remap.begin(), remap.end(), 0);
        return remap;
    }

    static std::vector<ElementIndex> MakeSpringRemap(std::vector<ObjectBuildSpring> const & springs)
    {
        std::vector<ElementIndex> remap(springs.size());
        std::iota(remap.begin(), remap.end(), 0);
        return remap;
    }
};
