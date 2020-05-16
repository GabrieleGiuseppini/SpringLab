/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageSize.h"
#include "Object.h"
#include "ObjectDefinition.h"
#include "Points.h"
#include "SLabTypes.h"
#include "Springs.h"
#include "StructuralMaterialDatabase.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

/*
 * This class contains all the logic for building an object out of an ObjectDefinition.
 */
class ObjectBuilder
{
public:

    static Object Create(
        ObjectDefinition && objectDefinition,
        StructuralMaterialDatabase const & structuralMaterialDatabase);

private:

    using ObjectBuildPointIndexMatrix = std::unique_ptr<std::unique_ptr<std::optional<ElementIndex>[]>[]>;

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


private:

    /////////////////////////////////////////////////////////////////
    // Building helpers
    /////////////////////////////////////////////////////////////////

    static void DetectSprings(
        ObjectBuildPointIndexMatrix const & pointIndexMatrix,
        ImageSize const & structureImageSize,
        std::vector<ObjectBuildPoint> & pointInfos,
        std::vector<ObjectBuildSpring> & springInfos);

    static Points CreatePoints(std::vector<ObjectBuildPoint> const & pointInfos);

    static Springs CreateSprings(
        std::vector<ObjectBuildSpring> const & springInfos,
        Points & points);
};
