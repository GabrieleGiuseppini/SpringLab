/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2023-03-19
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "IndexRemap.h"
#include "ObjectBuilderTypes.h"
#include "SLabTypes.h"

#include <vector>

class ILayoutOptimizer
{
public:

    virtual ~ILayoutOptimizer() = default;

    struct LayoutRemap
    {
        IndexRemap PointRemap;
        IndexRemap SpringRemap;
        std::vector<bool> SpringEndpointFlipMask; // Indexed by OLD

        LayoutRemap(
            IndexRemap && pointRemap,
            IndexRemap && springRemap)
            : PointRemap(std::move(pointRemap))
            , SpringRemap(std::move(springRemap))
            , SpringEndpointFlipMask(std::vector<bool>(SpringRemap.GetOldIndices().size(), false))
        {}

        LayoutRemap(
            IndexRemap && pointRemap,
            IndexRemap && springRemap,
            std::vector<bool> && springEndpointFlipMask)
            : PointRemap(std::move(pointRemap))
            , SpringRemap(std::move(springRemap))
            , SpringEndpointFlipMask(std::move(springEndpointFlipMask))
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
            IndexRemap::MakeIdempotent(points.size()),
            IndexRemap::MakeIdempotent(springs.size()));
    }
};
