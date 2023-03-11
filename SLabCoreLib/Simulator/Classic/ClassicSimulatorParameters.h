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
    float SpringStiffnessCoefficient;
    static float constexpr MinSpringStiffnessCoefficient = 0.0f;
    static float constexpr MaxSpringStiffnessCoefficient = 500000.0f;

    // Pure and simple damping coefficient.
    float SpringDampingCoefficient;
    static float constexpr MinSpringDampingCoefficient = 0.0f;
    static float constexpr MaxSpringDampingCoefficient = 10000.0f;

    // Global velocity damping; lowers velocity uniformly, damping oscillations.
    float GlobalDamping;
    static float constexpr MinGlobalDamping = 0.0f;
    static float constexpr MaxGlobalDamping = 1.0f;
};