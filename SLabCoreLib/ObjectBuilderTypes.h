/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Colors.h"
#include "Matrix.h"
#include "SLabTypes.h"
#include "StructuralMaterialDatabase.h"
#include "Vectors.h"

#include <memory>
#include <vector>

/*
 * Types describing the intermediate object structure.
 */

using ObjectBuildPointIndexMatrix = Matrix2<std::optional<ElementIndex>>;

struct ObjectBuildPoint
{
    vec2f Position;
    rgbColor RenderColor;
    StructuralMaterial const & Material;

    std::vector<ElementIndex> ConnectedSprings;

    ObjectBuildPoint(
        vec2f position,
        rgbColor renderColor,
        StructuralMaterial const & material)
        : Position(position)
        , RenderColor(renderColor)
        , Material(material)
        , ConnectedSprings()
    {
    }

    void AddConnectedSpring(ElementIndex springIndex)
    {
        assert(!ContainsConnectedSpring(springIndex));
        ConnectedSprings.push_back(springIndex);
    }

private:

    inline bool ContainsConnectedSpring(ElementIndex springIndex1) const
    {
        return std::find(
            ConnectedSprings.cbegin(),
            ConnectedSprings.cend(),
            springIndex1)
            != ConnectedSprings.cend();
    }
};

struct ObjectBuildSpring
{
    ElementIndex PointAIndex;
    ElementIndex PointBIndex;

    ObjectBuildSpring(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex)
        : PointAIndex(pointAIndex)
        , PointBIndex(pointBIndex)
    {
    }
};
