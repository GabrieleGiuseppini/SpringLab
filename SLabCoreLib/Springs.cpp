/***************************************************************************************
 * Original Author:      Gabriele Giuseppini
 * Created:              2020-05-16
 * Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "Springs.h"

void Springs::Add(
    ElementIndex pointAIndex,
    ElementIndex pointBIndex,
    Points const & points)
{
    ElementIndex const springIndex = static_cast<ElementIndex>(mEndpointsBuffer.GetCurrentPopulatedSize());

    mEndpointsBuffer.emplace_back(pointAIndex, pointBIndex);

    // Stiffness is average
    float const stiffness =
        (points.GetStructuralMaterial(pointAIndex).Stiffness + points.GetStructuralMaterial(pointBIndex).Stiffness)
        / 2.0f;
    mMaterialStiffnessBuffer.emplace_back(stiffness);

    mRestLengthBuffer.emplace_back((points.GetPosition(pointAIndex) - points.GetPosition(pointBIndex)).length());

    // Color is arbitrarily the color of the first endpoint
    mRenderColorBuffer.emplace_back(points.GetFactoryRenderColor(pointAIndex));
    mFactoryRenderColorBuffer.emplace_back(points.GetFactoryRenderColor(pointAIndex));
    mRenderNormThicknessBuffer.emplace_back(1.0f);
}