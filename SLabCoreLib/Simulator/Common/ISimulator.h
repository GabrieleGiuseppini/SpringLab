/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Object.h"
#include "SimulationParameters.h"

class ISimulator
{
public:

    virtual ~ISimulator()
    {}

    virtual void OnSimulationParametersChanged(SimulationParameters const & simulationParameters) = 0;

    virtual void Update(
        Object & object,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) = 0;
};