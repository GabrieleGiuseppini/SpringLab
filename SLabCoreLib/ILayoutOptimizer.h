/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2023-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IndexRemapper.h"
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
        IndexRemapper PointRemap;
        IndexRemapper SpringRemap;

        LayoutRemap(
            IndexRemapper && pointRemap,
            IndexRemapper && springRemap)
            : PointRemap(std::move(pointRemap))
            , SpringRemap(std::move(springRemap))
        {}
    };

    virtual LayoutRemap Remap(
        ObjectBuildPointIndexMatrix const & pointMatrix,
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
        ObjectBuildPointIndexMatrix const & /*pointMatrix*/,
        std::vector<ObjectBuildPoint> const & points,
        std::vector<ObjectBuildSpring> const & springs) const override
    {
        return LayoutRemap(
            IndexRemapper::MakeIdempotent(points.size()),
            IndexRemapper::MakeIdempotent(springs.size()));
    }
};
