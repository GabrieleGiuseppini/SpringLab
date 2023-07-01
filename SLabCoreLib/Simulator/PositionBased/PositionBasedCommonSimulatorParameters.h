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
    size_t NumUpdateIterations;
    static size_t constexpr MinNumUpdateIterations = 1;
    static size_t constexpr MaxNumUpdateIterations = 100;

    // The number of solver iterations per iterations
    size_t NumSolverIterations;
    static size_t constexpr MinNumSolverIterations = 1;
    static size_t constexpr MaxNumSolverIterations = 100;

    // The springs' stiffness
    float SpringStiffness;
    static float constexpr MinSpringStiffness = 0.0f;
    static float constexpr MaxSpringStiffness = 1.0f;

    // Global velocity damping; lowers velocity uniformly, damping oscillations.
    float GlobalDamping;
    static float constexpr MinGlobalDamping = 0.0f;
    static float constexpr MaxGlobalDamping = 1.0f;
};