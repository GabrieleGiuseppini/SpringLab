/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Vectors.h"

struct CommonSimulatorParameters
{
    CommonSimulatorParameters();

    // Time step
    float SimulationTimeStepDuration;

    // Mass
    float MassAdjustment;
    static float constexpr MinMassAdjustment = 0.0001f;
    static float constexpr MaxMassAdjustment = 1000.0f;

    // Gravity
    float GravityAdjustment;
    static float constexpr MinGravityAdjustment = 0.0f;
    static float constexpr MaxGravityAdjustment = 1000.0f;

    // Gravity as currently assigned to the simulation;
    // this is *excluding* the adjustment.
    // It is directly the gravity vector for convenience.
    vec2f AssignedGravity;

    // Global velocity damping; lowers velocity uniformly, damping oscillations.
    float GlobalDamping;
    static float constexpr MinGlobalDamping = 0.0f;
    static float constexpr MaxGlobalDamping = 1.0f;
};