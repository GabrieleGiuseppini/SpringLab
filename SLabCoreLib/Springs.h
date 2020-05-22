/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Buffer.h"
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
        , mRenderColorBuffer(mBufferElementCount, mElementCount, vec4f::zero())
        , mFactoryRenderColorBuffer(mBufferElementCount, mElementCount, vec4f::zero())
        , mRenderNormThicknessBuffer(mBufferElementCount, mElementCount, 0.0f)
        , mRenderHighlightBuffer(mBufferElementCount, mElementCount, 0.0f)
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
    // Render
    //

    vec4f const & GetRenderColor(ElementIndex springElementIndex) const
    {
        return mRenderColorBuffer[springElementIndex];
    }

    float GetRenderNormThickness(ElementIndex springElementIndex) const
    {
        return mRenderNormThicknessBuffer[springElementIndex];
    }

    float GetRenderHighlight(ElementIndex springElementIndex) const
    {
        return mRenderHighlightBuffer[springElementIndex];
    }

    void SetRenderHighlight(
        ElementIndex springElementIndex,
        float hightlight)
    {
        mRenderHighlightBuffer[springElementIndex] = hightlight;
    }

    float const * GetRenderHighlightBuffer() const
    {
        return mRenderHighlightBuffer.data();
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

    Buffer<vec4f> mRenderColorBuffer;
    Buffer<vec4f> mFactoryRenderColorBuffer;
    Buffer<float> mRenderNormThicknessBuffer;
    Buffer<float> mRenderHighlightBuffer;
};
