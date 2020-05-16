/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
#include "BufferAllocator.h"
#include "Colors.h"
#include "ElementContainer.h"
#include "FixedSizeVector.h"
#include "Points.h"

#include <cassert>

class Springs : public ElementContainer
{
public:

    /*
     * The endpoints of a spring.
     */
    struct Endpoints
    {
        ElementIndex PointAIndex;
        ElementIndex PointBIndex;

        Endpoints(
            ElementIndex pointAIndex,
            ElementIndex pointBIndex)
            : PointAIndex(pointAIndex)
            , PointBIndex(pointBIndex)
        {}
    };

public:

    Springs(ElementCount elementCount)
        : ElementContainer(elementCount)
        //////////////////////////////////
        // Buffers
        //////////////////////////////////
        // Structure
        , mEndpointsBuffer(mBufferElementCount, mElementCount, Endpoints(NoneElementIndex, NoneElementIndex))
        // Physics
        , mMaterialStiffnessBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mRestLengthBuffer(mBufferElementCount, mElementCount, 1.0f)
        // Render
        , mRenderColorBuffer(mBufferElementCount, mElementCount, rgbaColor::zero())
        , mFactoryRenderColorBuffer(mBufferElementCount, mElementCount, rgbaColor::zero())
        , mRenderNormThicknessBuffer(mBufferElementCount, mElementCount, 0.0f)
        //////////////////////////////////
        // Misc
        //////////////////////////////////
        , mFloatBufferAllocator(mBufferElementCount)
    {
    }

    Springs(Springs && other) = default;

    void Add(
        ElementIndex pointAIndex,
        ElementIndex pointBIndex,
        Points const & points);

public:

    //
    // Structure
    //

    ElementIndex GetEndpointAIndex(ElementIndex springElementIndex) const noexcept
    {
        return mEndpointsBuffer[springElementIndex].PointAIndex;
    }

    ElementIndex GetEndpointBIndex(ElementIndex springElementIndex) const noexcept
    {
        return mEndpointsBuffer[springElementIndex].PointBIndex;
    }

    ElementIndex GetOtherEndpointIndex(
        ElementIndex springElementIndex,
        ElementIndex pointElementIndex) const
    {
        if (pointElementIndex == mEndpointsBuffer[springElementIndex].PointAIndex)
            return mEndpointsBuffer[springElementIndex].PointBIndex;
        else
        {
            assert(pointElementIndex == mEndpointsBuffer[springElementIndex].PointBIndex);
            return mEndpointsBuffer[springElementIndex].PointAIndex;
        }
    }

    Endpoints const * restrict GetEndpointsBuffer() const noexcept
    {
        return mEndpointsBuffer.data();
    }

    vec2f const & GetEndpointAPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return points.GetPosition(mEndpointsBuffer[springElementIndex].PointAIndex);
    }

    vec2f const & GetEndpointBPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return points.GetPosition(mEndpointsBuffer[springElementIndex].PointBIndex);
    }

    vec2f GetMidpointPosition(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return (GetEndpointAPosition(springElementIndex, points)
            + GetEndpointBPosition(springElementIndex, points)) / 2.0f;
    }

    //
    // Physics
    //

    float GetMaterialStiffness(ElementIndex springElementIndex) const
    {
        return mMaterialStiffnessBuffer[springElementIndex];
    }

    float GetLength(
        ElementIndex springElementIndex,
        Points const & points) const
    {
        return
            (points.GetPosition(GetEndpointAIndex(springElementIndex)) - points.GetPosition(GetEndpointBIndex(springElementIndex)))
            .length();
    }

    float GetRestLength(ElementIndex springElementIndex) const noexcept
    {
        return mRestLengthBuffer[springElementIndex];
    }

    float const * restrict GetRestLengthBuffer() const noexcept
    {
        return mRestLengthBuffer.data();
    }

    //
    // Temporary buffer
    //

    std::shared_ptr<Buffer<float>> AllocateWorkBufferFloat()
    {
        return mFloatBufferAllocator.Allocate();
    }

private:

    //////////////////////////////////////////////////////////
    // Buffers
    //////////////////////////////////////////////////////////

    //
    // Structure
    //

    Buffer<Endpoints> mEndpointsBuffer;

    //
    // Physical
    //

    Buffer<float> mMaterialStiffnessBuffer;
    Buffer<float> mRestLengthBuffer;

    //
    // Render
    //

    Buffer<rgbaColor> mRenderColorBuffer;
    Buffer<rgbaColor> mFactoryRenderColorBuffer;
    Buffer<float> mRenderNormThicknessBuffer;

    //////////////////////////////////////////////////////////
    // Misc
    //////////////////////////////////////////////////////////

    // Allocators for work buffers
    BufferAllocator<float> mFloatBufferAllocator;
};
