/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ObjectBuilder.h"

#include "Log.h"

#include <cassert>
#include <utility>

//////////////////////////////////////////////////////////////////////////////

Object ObjectBuilder::Create(
    ObjectDefinition && objectDefinition,
    StructuralMaterialDatabase const & structuralMaterialDatabase,
    ILayoutOptimizer const & layoutOptimizer)
{
    int const structureWidth = objectDefinition.StructuralLayerImage.Size.Width;
    float const halfWidth = static_cast<float>(structureWidth) / 2.0f;

    int const structureHeight = objectDefinition.StructuralLayerImage.Size.Height;
    float const halfHeight = static_cast<float>(structureHeight) / 2.0f;

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
    ObjectBuildPointIndexMatrix pointIndexMatrix(new std::unique_ptr<std::optional<ElementIndex>[]>[structureWidth + 2]);
    for (int c = 0; c < structureWidth + 2; ++c)
    {
        pointIndexMatrix[c] = std::unique_ptr<std::optional<ElementIndex>[]>(new std::optional<ElementIndex>[structureHeight + 2]);
    }

    // Visit all columns
    for (int x = 0; x < structureWidth; ++x)
    {
        // From bottom to top
        for (int y = 0; y < structureHeight; ++y)
        {
            StructuralMaterialDatabase::ColorKey const colorKey = objectDefinition.StructuralLayerImage.Data[x + y * structureWidth];
            StructuralMaterial const * structuralMaterial = structuralMaterialDatabase.FindStructuralMaterial(colorKey);
            if (nullptr != structuralMaterial)
            {
                //
                // Make a point
                //

                ElementIndex const pointIndex = static_cast<ElementIndex>(pointInfos.size());

                pointIndexMatrix[x + 1][y + 1] = static_cast<ElementIndex>(pointIndex);

                pointInfos.emplace_back(
                    vec2f(
                        static_cast<float>(x) - halfWidth,
                        static_cast<float>(y) - halfHeight),
                    colorKey,
                    *structuralMaterial);
            }
            else if (colorKey != rgbColor(255, 255, 255))
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
        objectDefinition.StructuralLayerImage.Size,
        pointInfos,
        springInfos);

    //
    // Remap
    //

    auto const [pointInfos2, springInfos2] = Remap(
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

    LogMessage("Created object: W=", objectDefinition.StructuralLayerImage.Size.Width, ", H=", objectDefinition.StructuralLayerImage.Size.Height, ", ",
        points.GetElementCount(), "/", points.GetBufferElementCount(), "buf points, ",
        springs.GetElementCount(), " springs.");

    return Object(
        std::move(points),
        std::move(springs));
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
            if (!!pointIndexMatrix[x][y])
            {
                //
                // A point exists at these coordinates
                //

                ElementIndex pointIndex = *pointIndexMatrix[x][y];

                //
                // Check if a spring exists
                //

                // First four directions out of 8: from 0 deg (+x) through to 225 deg (-x -y),
                // i.e. E, SE, S, SW - this covers each pair of points in each direction
                for (int i = 0; i < 4; ++i)
                {
                    int adjx1 = x + Directions[i][0];
                    int adjy1 = y + Directions[i][1];

                    if (!!pointIndexMatrix[adjx1][adjy1])
                    {
                        // This point is adjacent to the first point at one of E, SE, S, SW

                        //
                        // Create BuildSpring
                        //

                        ElementIndex const otherEndpointIndex = *pointIndexMatrix[adjx1][adjy1];

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

std::tuple<std::vector<ObjectBuildPoint>, std::vector<ObjectBuildSpring>> ObjectBuilder::Remap(
    ObjectBuildPointIndexMatrix const & pointIndexMatrix,
    std::vector<ObjectBuildPoint> const & pointInfos,
    std::vector<ObjectBuildSpring> const & springInfos,
    ILayoutOptimizer const & layoutOptimizer)
{
    auto const layoutRemap = layoutOptimizer.Remap(pointIndexMatrix, pointInfos, springInfos);

    // Remap point info's

    std::vector<ObjectBuildPoint> pointInfos2;
    pointInfos2.reserve(pointInfos.size());
    for (ElementIndex oldP : layoutRemap.PointRemap)
    {
        pointInfos2.emplace_back(pointInfos[oldP]);

        for (size_t is = 0; is < pointInfos2.back().ConnectedSprings.size(); ++is)
        {
            pointInfos2.back().ConnectedSprings[is] = layoutRemap.SpringRemap[pointInfos2.back().ConnectedSprings[is]];
        }
    }

    // Remap spring info's

    std::vector<ObjectBuildSpring> springInfos2;
    springInfos2.reserve(springInfos.size());
    for (ElementIndex oldS : layoutRemap.SpringRemap)
    {
        springInfos2.emplace_back(springInfos[oldS]);

        springInfos2.back().PointAIndex = layoutRemap.PointRemap[springInfos2.back().PointAIndex];
        springInfos2.back().PointBIndex = layoutRemap.PointRemap[springInfos2.back().PointBIndex];
    }

    return { pointInfos2, springInfos2 };
}