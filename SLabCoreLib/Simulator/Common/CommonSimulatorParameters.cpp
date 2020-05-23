/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "CommonSimulatorParameters.h"

CommonSimulatorParameters::CommonSimulatorParameters()
    : SimulationTimeStepDuration(0.02f)
    , MassAdjustment(1.0f)
    , GravityAdjustment(1.0f)
    , AssignedGravity(vec2f::zero())
    , GlobalDamping(0.99983998f)
{
}