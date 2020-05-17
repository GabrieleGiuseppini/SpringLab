/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "AABB.h"
#include "Buffer.h"
#include "BufferAllocator.h"
#include "ElementContainer.h"
#include "ElementIndexRangeIterator.h"
#include "FixedSizeVector.h"
#include "SimulationParameters.h"
#include "SLabTypes.h"
#include "StructuralMaterial.h"
#include "Vectors.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <vector>

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

public:

    Points(ElementCount pointCount)
        : ElementContainer(make_aligned_float_element_count(pointCount))
        , mRawPointCount(pointCount)
        , mAlignedPointCount(make_aligned_float_element_count(pointCount))
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        // Materials
        , mStructuralMaterialBuffer(mBufferElementCount, pointCount, nullptr)
        // Physics
        , mPositionBuffer(mBufferElementCount, pointCount, vec2f::zero())
        , mVelocityBuffer(mBufferElementCount, pointCount, vec2f::zero())
        , mMassBuffer(mBufferElementCount, pointCount, 0.0f)
        // Structure
        , mConnectedSpringsBuffer(mBufferElementCount, pointCount, ConnectedSpringsVector())
        // Render
        , mRenderColorBuffer(mBufferElementCount, pointCount, vec4f::zero())
        , mFactoryRenderColorBuffer(mBufferElementCount, pointCount, vec4f::zero())
        , mRenderNormRadiusBuffer(mBufferElementCount, pointCount, 0.0f)
        , mRenderHighlightBuffer(mBufferElementCount, pointCount, 0.0f)
        //////////////////////////////////
        // Misc
        //////////////////////////////////
        , mFloatBufferAllocator(mBufferElementCount)
        , mVec2fBufferAllocator(mBufferElementCount)
    {
    }

    Points(Points && other) = default;

    /*
     * Returns an iterator for the (unaligned) points.
     */
    inline auto RawPoints() const
    {
        return ElementIndexRangeIterable(0, mRawPointCount);
    }

    ElementCount GetRawPointCount() const
    {
        return mRawPointCount;
    }

    ElementCount GetAlignedPointCount() const
    {
        return mAlignedPointCount;
    }

    void Add(
        vec2f const & position,
        vec3f const & color,
        StructuralMaterial const & structuralMaterial);

    void Query(ElementIndex pointElementIndex) const;

    AABB GetAABB() const
    {
        AABB box;
        for (ElementIndex pointIndex : RawPoints())
        {
            box.ExtendTo(GetPosition(pointIndex));
        }

        return box;
    }

public:

    //
    // Materials
    //

    StructuralMaterial const & GetStructuralMaterial(ElementIndex pointElementIndex) const
    {
        assert(nullptr != mStructuralMaterialBuffer[pointElementIndex]);
        return *(mStructuralMaterialBuffer[pointElementIndex]);
    }

    //
    // Physics
    //

    vec2f const & GetPosition(ElementIndex pointElementIndex) const noexcept
    {
        return mPositionBuffer[pointElementIndex];
    }

    vec2f & GetPosition(ElementIndex pointElementIndex) noexcept
    {
        return mPositionBuffer[pointElementIndex];
    }

    vec2f * GetPositionBuffer()
    {
        return mPositionBuffer.data();
    }

    std::shared_ptr<Buffer<vec2f>> MakePositionBufferCopy()
    {
        auto positionBufferCopy = mVec2fBufferAllocator.Allocate();
        positionBufferCopy->copy_from(mPositionBuffer);

        return positionBufferCopy;
    }

    void SetPosition(
        ElementIndex pointElementIndex,
        vec2f const & position) noexcept
    {
        mPositionBuffer[pointElementIndex] = position;
    }

    vec2f const & GetVelocity(ElementIndex pointElementIndex) const noexcept
    {
        return mVelocityBuffer[pointElementIndex];
    }

    vec2f & GetVelocity(ElementIndex pointElementIndex) noexcept
    {
        return mVelocityBuffer[pointElementIndex];
    }

    vec2f * GetVelocityBuffer()
    {
        return mVelocityBuffer.data();
    }

    std::shared_ptr<Buffer<vec2f>> MakeVelocityBufferCopy()
    {
        auto velocityBufferCopy = mVec2fBufferAllocator.Allocate();
        velocityBufferCopy->copy_from(mVelocityBuffer);

        return velocityBufferCopy;
    }

    void SetVelocity(
        ElementIndex pointElementIndex,
        vec2f const & velocity) noexcept
    {
        mVelocityBuffer[pointElementIndex] = velocity;
    }

    float GetMass(ElementIndex pointElementIndex) noexcept
    {
        return mMassBuffer[pointElementIndex];
    }

    //
    // Structure
    //

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
    // Temporary buffer
    //

    std::shared_ptr<Buffer<float>> AllocateWorkBufferFloat()
    {
        return mFloatBufferAllocator.Allocate();
    }

    std::shared_ptr<Buffer<vec2f>> AllocateWorkBufferVec2f()
    {
        return mVec2fBufferAllocator.Allocate();
    }

private:

    ElementCount const mRawPointCount;
    ElementCount const mAlignedPointCount;

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    //
    // Materials
    //

    Buffer<StructuralMaterial const *> mStructuralMaterialBuffer;

    //
    // Physics
    //

    Buffer<vec2f> mPositionBuffer;
    Buffer<vec2f> mVelocityBuffer;
    Buffer<float> mMassBuffer; // Augmented + Water

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


    //////////////////////////////////////////////////////////
    // Misc
    //////////////////////////////////////////////////////////

    // Allocators for work buffers
    BufferAllocator<float> mFloatBufferAllocator;
    BufferAllocator<vec2f> mVec2fBufferAllocator;
};
