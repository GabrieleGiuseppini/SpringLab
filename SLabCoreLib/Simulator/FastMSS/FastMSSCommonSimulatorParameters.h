/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

struct FastMSSCommonSimulatorParameters
{
    FastMSSCommonSimulatorParameters();

    // Pure and simple stiffness coefficient
    float SpringStiffnessCoefficient;
    static float constexpr MinSpringStiffnessCoefficient = 0.0f;
    static float constexpr MaxSpringStiffnessCoefficient = 4000000.0f;

    // Global damping
    float GlobalDamping;
    static float constexpr MinGlobalDamping = 0.0f;
    static float constexpr MaxGlobalDamping = 1.0f;

    // The number of local-global iterations in a step
    size_t NumLocalGlobalStepIterations;
    static size_t constexpr MinNumLocalGlobalStepIterations = 1;
    static size_t constexpr MaxNumLocalGlobalStepIterations = 100;
};