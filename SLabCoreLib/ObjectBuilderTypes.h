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
#include <unordered_map>
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

// Utilities for navigating object's structure

struct PointPair
{
    ElementIndex Endpoint1Index;
    ElementIndex Endpoint2Index;

    PointPair()
        : Endpoint1Index(NoneElementIndex)
        , Endpoint2Index(NoneElementIndex)
    {}

    PointPair(
        ElementIndex endpoint1Index,
        ElementIndex endpoint2Index)
        : Endpoint1Index(std::min(endpoint1Index, endpoint2Index))
        , Endpoint2Index(std::max(endpoint1Index, endpoint2Index))
    {}

    bool operator==(PointPair const & other) const
    {
        return this->Endpoint1Index == other.Endpoint1Index
            && this->Endpoint2Index == other.Endpoint2Index;
    }

    struct Hasher
    {
        size_t operator()(PointPair const & p) const
        {
            return p.Endpoint1Index * 23
                + p.Endpoint2Index;
        }
    };
};

using PointPairToIndexMap = std::unordered_map<PointPair, ElementIndex, PointPair::Hasher>;
