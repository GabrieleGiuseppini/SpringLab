/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringStructuralIntrinsicsMTSimulator.h"

#include "Log.h"

void FSBySpringStructuralIntrinsicsMTSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    FSBySpringStructuralIntrinsicsSimulator::OnStateChanged(object, simulationParameters);

    // TODOHERE
}

void FSBySpringStructuralIntrinsicsMTSimulator::ApplySpringsForces(
    Object const & object)
{
    // TODOHERE
    FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForces(
        object,
        mPointSpringForceBuffer.data(),
        0,
        object.GetSprings().GetElementCount());
}