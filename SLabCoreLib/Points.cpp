/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Points.h"

#include "Log.h"

void Points::Add(
    vec2f const & position,
    vec3f const & color,
    StructuralMaterial const & structuralMaterial)
{
    ElementIndex const pointIndex = static_cast<ElementIndex>(mStructuralMaterialBuffer.GetCurrentPopulatedSize());

    mStructuralMaterialBuffer.emplace_back(&structuralMaterial);

    mPositionBuffer.emplace_back(position);
    mVelocityBuffer.emplace_back(vec2f::zero());
    mMassBuffer.emplace_back(structuralMaterial.GetMass());

    mConnectedSpringsBuffer.emplace_back();

    mRenderColorBuffer.emplace_back(vec4f(color, 1.0f));
    mFactoryRenderColorBuffer.emplace_back(vec4f(color, 1.0f));
    mRenderNormRadiusBuffer.emplace_back(1.0f);
}

void Points::Query(ElementIndex pointElementIndex) const
{
    LogMessage("PointIndex: ", pointElementIndex, " (", mStructuralMaterialBuffer[pointElementIndex]->Name, ")");
    LogMessage("P=", mPositionBuffer[pointElementIndex].toString(), " V=", mVelocityBuffer[pointElementIndex].toString());
    LogMessage("Springs: ", mConnectedSpringsBuffer[pointElementIndex].size());
}