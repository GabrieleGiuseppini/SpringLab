/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-05-16
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "Points.h"
#include "Springs.h"

class Object
{
public:

    Object(
        Points && points,
        Springs && springs)
        : mPoints(std::move(points))
        , mSprings(std::move(springs))
    {}


    Points const & GetPoints() const
    {
        return mPoints;
    }

    Points & GetPoints()
    {
        return mPoints;
    }

    Springs const & GetSprings() const
    {
        return mSprings;
    }

    Springs & GetSprings()
    {
        return mSprings;
    }

private:

    Points mPoints;
    Springs mSprings;
};
