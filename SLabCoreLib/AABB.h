/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-05-16
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Vectors.h"

#include <limits>

// Axis-Aligned Bounding Box
class AABB
{
public:
    vec2f TopRight;
    vec2f BottomLeft;

    AABB()
        : TopRight(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest())
        , BottomLeft(std::numeric_limits<float>::max(), std::numeric_limits<float>::max())
    {}

    AABB(
        float left,
        float right,
        float top,
        float bottom)
        : TopRight(right, top)
        , BottomLeft(left, bottom)
    {}

    AABB(
        vec2f const topRight,
        vec2f const bottomLeft)
        : TopRight(topRight)
        , BottomLeft(bottomLeft)
    {}

    inline float GetWidth() const
    {
        return TopRight.x - BottomLeft.x;
    }

    inline float GetHeight() const
    {
        return TopRight.y - BottomLeft.y;
    }

    inline vec2f GetSize() const
    {
        return vec2f(GetWidth(), GetHeight());
    }

    inline void ExtendTo(vec2f const & point)
    {
        if (point.x > TopRight.x)
            TopRight.x = point.x;
        if (point.y > TopRight.y)
            TopRight.y = point.y;
        if (point.x < BottomLeft.x)
            BottomLeft.x = point.x;
        if (point.y < BottomLeft.y)
            BottomLeft.y = point.y;
    }

    inline void ExtendTo(AABB const & other)
    {
        if (other.TopRight.x > TopRight.x)
            TopRight.x = other.TopRight.x;
        if (other.TopRight.y > TopRight.y)
            TopRight.y = other.TopRight.y;
        if (other.BottomLeft.x < BottomLeft.x)
            BottomLeft.x = other.BottomLeft.x;
        if (other.BottomLeft.y < BottomLeft.y)
            BottomLeft.y = other.BottomLeft.y;
    }

    inline bool Contains(vec2f const & point) const
    {
        return point.x >= BottomLeft.x
            && point.x <= TopRight.x
            && point.y >= BottomLeft.y
            && point.y <= TopRight.y;
    }
};
