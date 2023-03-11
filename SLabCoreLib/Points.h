/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "AABB.h"
#include "Buffer.h"
#include "ElementContainer.h"
#include "ElementIndexRangeIterator.h"
#include "FixedSizeVector.h"
#include "SimulationParameters.h"
#include "SLabTypes.h"
#include "StructuralMaterial.h"
#include "Vectors.h"

#include <algorithm>
#include <cassert>
#include <optional>

class Points : public ElementContainer
{
public:

    /*
     * The metadata of a single spring connected to a point.
     */
    struct ConnectedSpring
    {
        ElementIndex SpringIndex;
        ElementIndex OtherEndpointIndex;

        ConnectedSpring()
            : SpringIndex(NoneElementIndex)
            , OtherEndpointIndex(NoneElementIndex)
        {}

        ConnectedSpring(
            ElementIndex springIndex,
            ElementIndex otherEndpointIndex)
            : SpringIndex(springIndex)
            , OtherEndpointIndex(otherEndpointIndex)
        {}
    };

    using ConnectedSpringsVector = FixedSizeVector<ConnectedSpring, SimulationParameters::MaxSpringsPerPoint>;

    /*
     * The metadata about bending probes. 
     */
    struct BendingProbe
    {
        ElementIndex PointIndex;
        vec2f OriginalWorldCoordinates;

        BendingProbe(
            ElementIndex pointIndex,
            vec2f const & originalWorldCoordinates)
            : PointIndex(pointIndex)
            , OriginalWorldCoordinates(originalWorldCoordinates)
        {}
    };

public:

    Points(ElementCount pointCount)
        : ElementContainer(pointCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        // Observable Physics
        , mPositionBuffer(mBufferElementCount, pointCount, vec2f::zero())
        , mVelocityBuffer(mBufferElementCount, pointCount, vec2f::zero())
        // System State
        , mAssignedForceBuffer(mBufferElementCount, pointCount, vec2f::zero())
        , mStructuralMaterialBuffer(mBufferElementCount, pointCount, nullptr)
        , mMassBuffer(mBufferElementCount, pointCount, 0.0f)
        , mFrozenCoefficientBuffer(mBufferElementCount, pointCount, 0.0f)
        , mConnectedSpringsBuffer(mBufferElementCount, pointCount, ConnectedSpringsVector())
        // Render
        , mRenderColorBuffer(mBufferElementCount, pointCount, vec4f::zero())
        , mFactoryRenderColorBuffer(mBufferElementCount, pointCount, vec4f::zero())
        , mRenderNormRadiusBuffer(mBufferElementCount, pointCount, 0.0f)
        , mRenderHighlightBuffer(mBufferElementCount, pointCount, 0.0f)
    {
    }

    Points(Points && other) = default;

    void Add(
        vec2f const & position,
        vec3f const & color,
        StructuralMaterial const & structuralMaterial);

    void Finalize();

    void Query(ElementIndex pointElementIndex) const;

    AABB GetAABB() const
    {
        AABB box;
        for (ElementIndex pointIndex : *this)
        {
            box.ExtendTo(GetPosition(pointIndex));
        }

        return box;
    }

public:

    //
    // Observable Physics
    //

    vec2f const & GetPosition(ElementIndex pointElementIndex) const noexcept
    {
        return mPositionBuffer[pointElementIndex];
    }

    vec2f const * GetPositionBuffer() const noexcept
    {
        return mPositionBuffer.data();
    }

    vec2f * GetPositionBuffer() noexcept
    {
        return mPositionBuffer.data();
    }

    void SetPosition(
        ElementIndex pointElementIndex,
        vec2f const & value) noexcept
    {
        mPositionBuffer[pointElementIndex] = value;
    }

    vec2f const & GetVelocity(ElementIndex pointElementIndex) const noexcept
    {
        return mVelocityBuffer[pointElementIndex];
    }

    vec2f const * GetVelocityBuffer() const noexcept
    {
        return mVelocityBuffer.data();
    }

    vec2f * GetVelocityBuffer() noexcept
    {
        return mVelocityBuffer.data();
    }

    void SetVelocity(
        ElementIndex pointElementIndex,
        vec2f const & value) noexcept
    {
        mVelocityBuffer[pointElementIndex] = value;
    }

    //
    // System State
    //

    vec2f const & GetAssignedForce(ElementIndex pointElementIndex) const noexcept
    {
        return mAssignedForceBuffer[pointElementIndex];
    }

    vec2f * GetAssignedForceBuffer() noexcept
    {
        return mAssignedForceBuffer.data();
    }

    void SetAssignedForce(
        ElementIndex pointElementIndex,
        vec2f const & value) noexcept
    {
        mAssignedForceBuffer[pointElementIndex] = value;
    }

    StructuralMaterial const & GetStructuralMaterial(ElementIndex pointElementIndex) const
    {
        assert(nullptr != mStructuralMaterialBuffer[pointElementIndex]);
        return *(mStructuralMaterialBuffer[pointElementIndex]);
    }

    float GetMass(ElementIndex pointElementIndex) const noexcept
    {
        return mMassBuffer[pointElementIndex];
    }

    bool GetFrozenCoefficient(ElementIndex pointElementIndex) const
    {
        return mFrozenCoefficientBuffer[pointElementIndex];
    }

    float const * GetFrozenCoefficientBuffer() const
    {
        return mFrozenCoefficientBuffer.data();
    }

    void SetFrozenCoefficient(
        ElementIndex pointElementIndex,
        float value)
    {
        mFrozenCoefficientBuffer[pointElementIndex] = value;
    }

    auto const & GetConnectedSprings(ElementIndex pointElementIndex) const
    {
        return mConnectedSpringsBuffer[pointElementIndex];
    }

    void AddConnectedSpring(
        ElementIndex pointElementIndex,
        ElementIndex springElementIndex,
        ElementIndex otherEndpointElementIndex)
    {
        assert(!mConnectedSpringsBuffer[pointElementIndex].contains(
            [springElementIndex](auto const & cs)
            {
                return cs.SpringIndex == springElementIndex;
            }));

        mConnectedSpringsBuffer[pointElementIndex].emplace_back(
            springElementIndex,
            otherEndpointElementIndex);
    }

    //
    // Render
    //

    vec4f const & GetRenderColor(ElementIndex pointElementIndex) const
    {
        return mRenderColorBuffer[pointElementIndex];
    }

    void SetRenderColor(
        ElementIndex pointElementIndex,
        vec4f const & color)
    {
        mRenderColorBuffer[pointElementIndex] = color;
    }

    vec4f const * GetRenderColorBuffer() const
    {
        return mRenderColorBuffer.data();
    }

    void ResetRenderColorsToFactoryRenderColors()
    {
        mRenderColorBuffer.copy_from(mFactoryRenderColorBuffer);
    }

    vec4f const & GetFactoryRenderColor(ElementIndex pointElementIndex) const
    {
        return mFactoryRenderColorBuffer[pointElementIndex];
    }

    float GetRenderNormRadius(ElementIndex pointElementIndex) const
    {
        return mRenderNormRadiusBuffer[pointElementIndex];
    }

    void SetRenderNormRadius(
        ElementIndex pointElementIndex,
        float normRadius)
    {
        mRenderNormRadiusBuffer[pointElementIndex] = normRadius;
    }

    float const * GetRenderNormRadiusBuffer() const
    {
        return mRenderNormRadiusBuffer.data();
    }

    void SetRenderHighlight(
        ElementIndex pointElementIndex,
        float hightlight)
    {
        mRenderHighlightBuffer[pointElementIndex] = hightlight;
    }

    float const * GetRenderHighlightBuffer() const
    {
        return mRenderHighlightBuffer.data();
    }

    //
    // Misc
    //

    auto const & GetBendingProbe() const
    {
        return mBendingProbe;
    }

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    //
    // Observable Physics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;

    //
    // System State
    //

    Buffer<vec2f> mAssignedForceBuffer;
    Buffer<StructuralMaterial const *> mStructuralMaterialBuffer;
    Buffer<float> mMassBuffer;
    Buffer<float> mFrozenCoefficientBuffer; // 1.0: not frozen; 0.0f: frozen

    //
    // Structure
    //

    Buffer<ConnectedSpringsVector> mConnectedSpringsBuffer;

    //
    // Render
    //

    Buffer<vec4f> mRenderColorBuffer;
    Buffer<vec4f> mFactoryRenderColorBuffer;
    Buffer<float> mRenderNormRadiusBuffer;
    Buffer<float> mRenderHighlightBuffer;

    //
    // Misc
    //

    std::optional<BendingProbe> mBendingProbe;
};
