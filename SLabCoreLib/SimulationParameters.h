
/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Vectors.h"

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

    // Dynamics

    // Fraction of a spring displacement that is removed during a spring relaxation
    // iteration. The remaining spring displacement is (1.0 - this fraction).
    static float constexpr SpringReductionFraction = 0.4f;

    // The empirically-determined constant for the spring damping.
    // The simulation is quite sensitive to this value:
    // - 0.03 is almost fine (though bodies are sometimes soft)
    // - 0.8 makes everything explode
    static float constexpr SpringDampingCoefficient = 0.03f;

    //
    // Tunable parameters
    //

    float SpringStiffnessAdjustment;
    static float constexpr MinSpringStiffnessAdjustment = 0.001f;
    static float constexpr MaxSpringStiffnessAdjustment = 2.4f;

    float SpringDampingAdjustment;
    static float constexpr MinSpringDampingAdjustment = 0.001f;
    static float constexpr MaxSpringDampingAdjustment = 4.0f;

    //
    // Simulation constants
    //

    static size_t constexpr MaxSpringsPerPoint = 8;
};
