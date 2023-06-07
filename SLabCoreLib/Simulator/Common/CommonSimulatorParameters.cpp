/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "CommonSimulatorParameters.h"

CommonSimulatorParameters::CommonSimulatorParameters()
    : SimulationTimeStepDuration(1.0f / 64.0f)
    , MassAdjustment(1.0f)
    , AssignedGravity(vec2f::zero())
    , GravityAdjustment(1.0f)
{
}