/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

struct PositionBasedCommonSimulatorParameters
{
    PositionBasedCommonSimulatorParameters();

    // The number of iterations in a step
    size_t NumMechanicalDynamicsIterations;
    static size_t constexpr MinNumMechanicalDynamicsIterations = 1;
    static size_t constexpr MaxNumMechanicalDynamicsIterations = 100;

    // The fraction of a spring's over-length that it gets reduced to in a simulation step
    float SpringReductionFraction;
    static float constexpr MinSpringReductionFraction = 0.0f;
    static float constexpr MaxSpringReductionFraction = 1.0f;

    // Damping coefficient
    float SpringDampingCoefficient;
    static float constexpr MinSpringDampingCoefficient = 0.0f;
    static float constexpr MaxSpringDampingCoefficient = 2.0f;

    // Global velocity damping; lowers velocity uniformly, damping oscillations.
    float GlobalDamping;
    static float constexpr MinGlobalDamping = 0.0f;
    static float constexpr MaxGlobalDamping = 1.0f;
};