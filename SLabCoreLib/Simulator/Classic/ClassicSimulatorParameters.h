/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

struct ClassicSimulatorParameters
{
    ClassicSimulatorParameters();

    // Fraction of a spring displacement that is removed during a spring relaxation
    // iteration. The remaining spring displacement is (1.0 - this fraction).
    float SpringReductionFraction;
    static float constexpr MinSpringReductionFraction = 0.0001f;
    static float constexpr MaxSpringReductionFraction = 1.0f;

    // The empirically-determined constant for the spring damping.
    // The simulation is quite sensitive to this value:
    // - 0.03 is almost fine (though bodies are sometimes soft)
    // - 0.8 makes everything explode
    float SpringDampingCoefficient;
    static float constexpr MinSpringDampingCoefficient = 0.0f;
    static float constexpr MaxSpringDampingCoefficient = 1.0f;

    // How much spring forces contribute to the intertia of
    // a particle
    float SpringForceInertia;
    static float constexpr MinSpringForceInertia = 0.0f;
    static float constexpr MaxSpringForceInertia = 1.0f;
};