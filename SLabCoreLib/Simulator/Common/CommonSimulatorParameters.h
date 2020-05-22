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

    // Gravity as currently assigned to the simulation.
    // It is directly the gravity vector, rather than a boolean, for convenience.
    vec2f AssignedGravity;

    // Global velocity damping; lowers velocity uniformly, damping oscillations.
    float GlobalDamping;
};