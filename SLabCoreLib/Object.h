/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-05-16
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "ObjectSimulatorSpecificStructure.h"
#include "Points.h"
#include "Springs.h"

class Object
{
public:

    Object(
        Points && points,
        Springs && springs,
        ObjectSimulatorSpecificStructure && simulatorSpecificStructure)
        : mPoints(std::move(points))
        , mSprings(std::move(springs))
        , mSimulatorSpecificStructure(std::move(simulatorSpecificStructure))
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

    ObjectSimulatorSpecificStructure const & GetSimulatorSpecificStructure() const
    {
        return mSimulatorSpecificStructure;
    }

private:

    Points mPoints;
    Springs mSprings;
    ObjectSimulatorSpecificStructure mSimulatorSpecificStructure;
};
