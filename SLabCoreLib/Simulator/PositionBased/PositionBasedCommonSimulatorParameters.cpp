/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PositionBasedCommonSimulatorParameters.h"

PositionBasedCommonSimulatorParameters::PositionBasedCommonSimulatorParameters()
    : NumMechanicalDynamicsIterations(1)
    , SpringReductionFraction(0.5f)
    , SpringDampingCoefficient(0.03f)
    , GlobalDamping(0.99983998f)
{
}