/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GaussSeidelCommonSimulatorParameters.h"

GaussSeidelCommonSimulatorParameters::GaussSeidelCommonSimulatorParameters()
    : NumMechanicalDynamicsIterations(30)
    , SpringReductionFraction(0.5f)
    , SpringDampingCoefficient(0.03f)
    , GlobalDamping(0.00010749653315f)
{
}