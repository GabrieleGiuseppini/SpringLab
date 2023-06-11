/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageSize.h"
#include "ILayoutOptimizer.h"
#include "Object.h"
#include "ObjectBuilderTypes.h"
#include "ObjectDefinition.h"
#include "ObjectSimulatorSpecificStructure.h"
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
        StructuralMaterialDatabase const & structuralMaterialDatabase,
        ILayoutOptimizer const & layoutOptimizer);

    static Object MakeSynthetic(
        size_t numSprings,
        StructuralMaterialDatabase const & structuralMaterialDatabase,
        ILayoutOptimizer const & layoutOptimizer);

private:

    static Object InternalCreate(
        RgbImageData && structuralLayerImage,
        StructuralMaterialDatabase const & structuralMaterialDatabase,
        ILayoutOptimizer const & layoutOptimizer);

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

    static std::tuple<std::vector<ObjectBuildPoint>, std::vector<ObjectBuildSpring>, ObjectSimulatorSpecificStructure> Remap(
        ObjectBuildPointIndexMatrix const & pointIndexMatrix,
        std::vector<ObjectBuildPoint> const & pointInfos,
        std::vector<ObjectBuildSpring> const & springInfos,
        ILayoutOptimizer const & layoutOptimizer);
};
