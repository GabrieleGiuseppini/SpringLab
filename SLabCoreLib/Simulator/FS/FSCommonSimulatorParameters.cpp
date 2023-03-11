/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-11
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSCommonSimulatorParameters.h"

FSCommonSimulatorParameters::FSCommonSimulatorParameters()
    : NumMechanicalDynamicsIterations(30)
    , SpringReductionFraction(0.5f)
    , SpringDampingCoefficient(0.03f)
    , GlobalDamping(0.00010749653315f)
{
}