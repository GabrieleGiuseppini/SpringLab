/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PositionBasedCommonSimulatorParameters.h"

PositionBasedCommonSimulatorParameters::PositionBasedCommonSimulatorParameters()
    : NumUpdateIterations(1)
    , NumSolverIterations(1)
    , SpringStiffness(1.0f)
    , GlobalDamping(0.99983998f)
{
}