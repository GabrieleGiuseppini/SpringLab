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
    mPositionBuffer.emplace_back(position);
    mVelocityBuffer.emplace_back(vec2f::zero());

    mAssignedForceBuffer.emplace_back(vec2f::zero());
    mStructuralMaterialBuffer.emplace_back(&structuralMaterial);
    mMassBuffer.emplace_back(structuralMaterial.GetMass());
    mFrozenCoefficientBuffer.emplace_back(structuralMaterial.IsFixed ? 0.0f : 1.0f);
    mConnectedSpringsBuffer.emplace_back();

    mRenderColorBuffer.emplace_back(vec4f(color, 1.0f));
    mFactoryRenderColorBuffer.emplace_back(vec4f(color, 1.0f));
    mRenderNormRadiusBuffer.emplace_back(1.0f);
    mRenderHighlightBuffer.emplace_back(0.0f);
}

void Points::Finalize()
{
    //
    // Bending probe
    //

    for (ElementIndex p : *this)
    {
        if (mStructuralMaterialBuffer[p]->IsBendingProbe)
        {
            if (mBendingProbe)
            {
                throw SLabException("There is more than one bending probe in the object");
            }

            mBendingProbe.emplace(
                p,
                mPositionBuffer[p]);
        }
    }
}

void Points::Query(ElementIndex pointElementIndex) const
{
    LogMessage("PointIndex: ", pointElementIndex,
        " (", mStructuralMaterialBuffer[pointElementIndex]->Name,
        ") M=", mMassBuffer[pointElementIndex]);
    LogMessage("P=", mPositionBuffer[pointElementIndex].toString(), " V=", mVelocityBuffer[pointElementIndex].toString());
    LogMessage("Springs: ", mConnectedSpringsBuffer[pointElementIndex].size());
}