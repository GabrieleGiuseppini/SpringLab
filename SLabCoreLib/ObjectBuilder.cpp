/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ObjectBuilder.h"

#include "Log.h"

#include <cassert>
#include <utility>


rgbColor constexpr EmptyMaterialColorKey = rgbColor(255, 255, 255);

//////////////////////////////////////////////////////////////////////////////

Object ObjectBuilder::Create(
    ObjectDefinition && objectDefinition,
    StructuralMaterialDatabase const & structuralMaterialDatabase,
    ILayoutOptimizer const & layoutOptimizer)
{
    return InternalCreate(
        std::move(objectDefinition.StructuralLayerImage),
        structuralMaterialDatabase,
        layoutOptimizer);
}

Object ObjectBuilder::MakeSynthetic(
    size_t numSprings,
    StructuralMaterialDatabase const & structuralMaterialDatabase,
    ILayoutOptimizer const & layoutOptimizer)
{
    // Number of springs on bottom side
    //
    // Given a square with side sideSprings:
    //  numSprings = sideSprings * (6 + (sideSprings - 1) * 4)
    size_t const sideSprings = static_cast<size_t>(std::ceil((std::sqrt(1.0f + 4.0f * numSprings) - 1) / 4.0f));

    // Allocate image
    size_t const sidePixels = sideSprings + 1;
    std::unique_ptr<rgbColor[]> pixels = std::unique_ptr<rgbColor[]>(new rgbColor[sidePixels * sidePixels]);

    // Start "empty"
    std::fill(
        pixels.get(),
        pixels.get() + sidePixels * sidePixels,
        EmptyMaterialColorKey);

    size_t actualNumSprings = 0;

    //
    // 1. Bottom stripe
    //

    rgbColor constexpr MaterialColorKey = rgbColor(0x80, 0x80, 0x90); // Iron Grey

    int x = 0;
    int y = 0;
    pixels[0 + 0] = MaterialColorKey;

    for (x = 1; x < sidePixels && actualNumSprings < numSprings; ++x)
    {
        pixels[x + 0] = MaterialColorKey;
        ++actualNumSprings;
    }

    //
    // 2. Filling
    //

    x = 0;
    y = 1;
    while (actualNumSprings < numSprings)
    {
        assert(y < sidePixels); // Guaranteed by side calculation

        pixels[x + y * sidePixels] = MaterialColorKey;

        if (x == 0)
        {
            actualNumSprings += 2;
        }
        else if (x < sidePixels - 1)
        {
            actualNumSprings += 4;
        }
        else
        {
            assert(x == sidePixels - 1);
            actualNumSprings += 3;
        }

        ++x;
        if (x == sidePixels)
        {
            x = 0;
            ++y;
        }
    }
    
    //
    // Finalize
    //

    return InternalCreate(
        RgbImageData(
            static_cast<int>(sidePixels),
            static_cast<int>(sidePixels),
            std::move(pixels)),
        structuralMaterialDatabase,
        layoutOptimizer);
}

Object ObjectBuilder::InternalCreate(
    RgbImageData && structuralLayerImage,
    StructuralMaterialDatabase const & structuralMaterialDatabase,
    ILayoutOptimizer const & layoutOptimizer)
{
    int const structureWidth = structuralLayerImage.Size.Width;
    float const halfWidth = static_cast<float>(structureWidth / 2); // We want to align on integral world coords

    int const structureHeight = structuralLayerImage.Size.Height;
    float const halfHeight = static_cast<float>(structureHeight / 2); // We want to align on integral world coords

    // Build Point's
    std::vector<ObjectBuildPoint> pointInfos;

    // Build Spring's
    std::vector<ObjectBuildSpring> springInfos;

    //
    // Process structural layer points and:
    // - Identify all points, and create BuildPoint's for them
    // - Build a 2D matrix containing indices to the points above
    //

    // Matrix of points - we allocate 2 extra dummy rows and cols - around - to avoid checking for boundaries
    ObjectBuildPointIndexMatrix pointIndexMatrix(structureWidth + 2, structureHeight + 2);

    // Visit all columns
    for (int x = 0; x < structureWidth; ++x)
    {
        // From bottom to top
        for (int y = 0; y < structureHeight; ++y)
        {
            StructuralMaterialDatabase::ColorKey const colorKey = structuralLayerImage.Data[x + y * structureWidth];
            StructuralMaterial const * structuralMaterial = structuralMaterialDatabase.FindStructuralMaterial(colorKey);
            if (nullptr != structuralMaterial)
            {
                //
                // Make a point
                //

                ElementIndex const pointIndex = static_cast<ElementIndex>(pointInfos.size());

                pointIndexMatrix[{x + 1, y + 1}] = static_cast<ElementIndex>(pointIndex);

                pointInfos.emplace_back(
                    vec2f(
                        static_cast<float>(x) - halfWidth,
                        static_cast<float>(y) - halfHeight),
                    colorKey,
                    *structuralMaterial);
            }
            else if (colorKey != EmptyMaterialColorKey)
            {
                throw SLabException("Pixel at coordinate (" + std::to_string(x) + ", " + std::to_string(y) + ") is not a recognized material");
            }
        }
    }

    //
    // Visit point matrix and detect all springs, connecting points and springs together
    //

    DetectSprings(
        pointIndexMatrix,
        structuralLayerImage.Size,
        pointInfos,
        springInfos);

    //
    // Remap
    //

    auto [pointInfos2, springInfos2, simulatorSpecificStructure] = Remap(
        pointIndexMatrix,
        pointInfos,
        springInfos,
        layoutOptimizer);


    //
    // Visit all Build Point's and create Points, i.e. the entire set of points
    //

    Points points = CreatePoints(pointInfos2);


    //
    // Visit all Build Springs's and create Springs, i.e. the entire set of springs
    //

    Springs springs = CreateSprings(
        springInfos2,
        points);

    //
    // We're done!
    //

    LogMessage("Created object: W=", structuralLayerImage.Size.Width, ", H=", structuralLayerImage.Size.Height, ", ",
        points.GetElementCount(), "/", points.GetBufferElementCount(), "buf points, ",
        springs.GetElementCount(), " springs.");

    return Object(
        std::move(points),
        std::move(springs),
        std::move(simulatorSpecificStructure));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Building helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

void ObjectBuilder::DetectSprings(
    ObjectBuildPointIndexMatrix const & pointIndexMatrix,
    ImageSize const & structureImageSize,
    std::vector<ObjectBuildPoint> & pointInfos,
    std::vector<ObjectBuildSpring> & springInfos)
{
    //
    // Visit point matrix and:
    //  - Detect springs and create Build Spring's for them
    //

    // This is our local circular order
    static const int Directions[8][2] = {
        {  1,  0 },  // 0: E
        {  1, -1 },  // 1: SE
        {  0, -1 },  // 2: S
        { -1, -1 },  // 3: SW
        { -1,  0 },  // 4: W
        { -1,  1 },  // 5: NW
        {  0,  1 },  // 6: N
        {  1,  1 }   // 7: NE
    };

    // From bottom to top - excluding extras at boundaries
    for (int y = 1; y <= structureImageSize.Height; ++y)
    {
        // From left to right - excluding extras at boundaries
        for (int x = 1; x <= structureImageSize.Width; ++x)
        {
            if (!!pointIndexMatrix[{x, y}])
            {
                //
                // A point exists at these coordinates
                //

                ElementIndex pointIndex = *pointIndexMatrix[{x, y}];

                //
                // Check if a spring exists
                //

                // First four directions out of 8: from 0 deg (+x) through to 225 deg (-x -y),
                // i.e. E, SE, S, SW - this covers each pair of points in each direction
                for (int i = 0; i < 4; ++i)
                {
                    int adjx1 = x + Directions[i][0];
                    int adjy1 = y + Directions[i][1];

                    if (!!pointIndexMatrix[{adjx1, adjy1}])
                    {
                        // This point is adjacent to the first point at one of E, SE, S, SW

                        //
                        // Create BuildSpring
                        //

                        ElementIndex const otherEndpointIndex = *pointIndexMatrix[{adjx1, adjy1}];

                        ElementIndex const springIndex = static_cast<ElementIndex>(springInfos.size());

                        springInfos.emplace_back(
                            pointIndex,
                            otherEndpointIndex);

                        // Add the spring to its endpoints
                        pointInfos[pointIndex].AddConnectedSpring(springIndex);
                        pointInfos[otherEndpointIndex].AddConnectedSpring(springIndex);
                    }
                }
            }
        }
    }
}

Points ObjectBuilder::CreatePoints(std::vector<ObjectBuildPoint> const & pointInfos)
{
    Points points(static_cast<ElementIndex>(pointInfos.size()));

    for (size_t p = 0; p < pointInfos.size(); ++p)
    {
        ObjectBuildPoint const & pointInfo = pointInfos[p];

        //
        // Create point
        //

        points.Add(
            pointInfo.Position,
            pointInfo.RenderColor.toVec3f(),
            pointInfo.Material);
    }

    points.Finalize();

    return points;
}

Springs ObjectBuilder::CreateSprings(
    std::vector<ObjectBuildSpring> const & springInfos,
    Points & points)
{
    Springs springs(static_cast<ElementIndex>(springInfos.size()));

    for (ElementIndex s = 0; s < springInfos.size(); ++s)
    {
        // Create spring
        springs.Add(
            springInfos[s].PointAIndex,
            springInfos[s].PointBIndex,
            points);

        // Add spring to its endpoints
        points.AddConnectedSpring(
            springInfos[s].PointAIndex,
            s,
            springInfos[s].PointBIndex);
        points.AddConnectedSpring(
            springInfos[s].PointBIndex,
            s,
            springInfos[s].PointAIndex);
    }

    return springs;
}

std::tuple<std::vector<ObjectBuildPoint>, std::vector<ObjectBuildSpring>, ObjectSimulatorSpecificStructure> ObjectBuilder::Remap(
    ObjectBuildPointIndexMatrix const & pointIndexMatrix,
    std::vector<ObjectBuildPoint> const & pointInfos,
    std::vector<ObjectBuildSpring> const & springInfos,
    ILayoutOptimizer const & layoutOptimizer)
{
    auto layoutRemap = layoutOptimizer.Remap(pointIndexMatrix, pointInfos, springInfos);

    // Remap point info's

    std::vector<ObjectBuildPoint> pointInfos2;
    pointInfos2.reserve(pointInfos.size());
    for (ElementIndex oldP : layoutRemap.PointRemap.GetOldIndices())
    {
        pointInfos2.emplace_back(pointInfos[oldP]);

        for (size_t is = 0; is < pointInfos2.back().ConnectedSprings.size(); ++is)
        {
            pointInfos2.back().ConnectedSprings[is] = layoutRemap.SpringRemap.OldToNew(pointInfos2.back().ConnectedSprings[is]);
        }
    }

    // Remap spring info's

    std::vector<ObjectBuildSpring> springInfos2;
    springInfos2.reserve(springInfos.size());
    for (ElementIndex oldS : layoutRemap.SpringRemap.GetOldIndices())
    {
        springInfos2.emplace_back(springInfos[oldS]);

        springInfos2.back().PointAIndex = layoutRemap.PointRemap.OldToNew(springInfos2.back().PointAIndex);
        springInfos2.back().PointBIndex = layoutRemap.PointRemap.OldToNew(springInfos2.back().PointBIndex);

        if (layoutRemap.SpringEndpointFlipMask[oldS])
        {
            std::swap(springInfos2.back().PointAIndex, springInfos2.back().PointBIndex);
        }
    }

    return { pointInfos2, springInfos2, layoutRemap.SimulatorSpecificStructure };
}