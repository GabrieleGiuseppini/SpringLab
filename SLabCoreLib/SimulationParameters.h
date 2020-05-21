/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Vectors.h"

#include "Simulator/Common/CommonSimulatorParameters.h"
#include "Simulator/Classic/ClassicSimulatorParameters.h"

/*
 * Parameters that affect the simulation.
 */
struct SimulationParameters
{
    SimulationParameters();

    //
    // The dt of each step
    //

    template <typename T>
    static constexpr T SimulationStepTimeDuration = 0.02f;


    //
    // Physical Constants
    //

    // Gravity
    static constexpr vec2f Gravity = vec2f(0.0f, -9.80f);
    static constexpr vec2f GravityNormalized = vec2f(0.0f, -1.0f);
    static float constexpr GravityMagnitude = 9.80f; // m/s


    //
    // Simulator parameters
    //

    CommonSimulatorParameters Common;
    ClassicSimulatorParameters ClassicSimulator;


    //
    // Structural constants
    //

    static size_t constexpr MaxSpringsPerPoint = 8;
};
