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

    virtual ~ISimulator() = default;

    /*
     * Invoked when either a parameter changes, or when an attribute of the
     * object changes - excluding position and velocity changes.
     * No structural changes.
     */
    virtual void OnStateChanged(
        Object const & object,
        SimulationParameters const & simulationParameters) = 0;

    /*
     * Performs a single update step of the simulation.
     * The outcome is a new set of positions and velocities of the particles.
     */
    virtual void Update(
        Object & object,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) = 0;
};