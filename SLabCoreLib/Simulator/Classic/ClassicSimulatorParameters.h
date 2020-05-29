/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

struct ClassicSimulatorParameters
{
    ClassicSimulatorParameters();

    // Pure and simple stiffness coefficient for Hooke's law.
    float SpringStiffness;
    static float constexpr MinSpringStiffness = 0.0f;
    static float constexpr MaxSpringStiffness = 500000.0f;

    // Pure and simple damping coefficient.
    float SpringDamping;
    static float constexpr MinSpringDamping = 0.0f;
    static float constexpr MaxSpringDamping = 10000.0f;

    // How much spring forces contribute to the intertia of
    // a particle
    float SpringForceInertia;
    static float constexpr MinSpringForceInertia = 0.0f;
    static float constexpr MaxSpringForceInertia = 1.0f;
};