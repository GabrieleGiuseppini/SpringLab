/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PositionBasedBasicSimulator.h"

#include <cassert>

PositionBasedBasicSimulator::PositionBasedBasicSimulator(
    Object const & object,
    SimulationParameters const & simulationParameters)
    // Point buffers
    : mPointExternalForceBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    // Spring buffers
    , mSpringStiffnessCoefficientBuffer(object.GetSprings().GetBufferElementCount(), 0, 0.0f)
    , mSpringDampingCoefficientBuffer(object.GetSprings().GetBufferElementCount(), 0, 0.0f)
{
    CreateState(object, simulationParameters);
}

void PositionBasedBasicSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    CreateState(object, simulationParameters);
}

void PositionBasedBasicSimulator::Update(
    Object & object,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters)
{
    // TODOHERE
    (void)object;
    (void)simulationParameters;
}

///////////////////////////////////////////////////////////////////////////////////////////

void PositionBasedBasicSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    // TODO
    float const dt = simulationParameters.Common.SimulationTimeStepDuration;

    float const dtSquared = dt * dt;

    //
    // Initialize point buffers
    //

    Points const & points = object.GetPoints();

    for (auto pointIndex : points)
    {
        mPointExternalForceBuffer[pointIndex] =
            simulationParameters.Common.AssignedGravity * points.GetMass(pointIndex) * simulationParameters.Common.MassAdjustment
            + points.GetAssignedForce(pointIndex);
    }


    //
    // Initialize spring buffers
    //

    Springs const & springs = object.GetSprings();

    for (auto springIndex : springs)
    {
        auto const endpointAIndex = springs.GetEndpointAIndex(springIndex);
        auto const endpointBIndex = springs.GetEndpointBIndex(springIndex);

        float const endpointAMass = points.GetMass(endpointAIndex) * simulationParameters.Common.MassAdjustment;
        float const endpointBMass = points.GetMass(endpointBIndex) * simulationParameters.Common.MassAdjustment;

        float const massFactor =
            (endpointAMass * endpointBMass)
            / (endpointAMass + endpointBMass);

        // The "stiffness coefficient" is the factor which, once multiplied with the spring displacement,
        // yields the spring force, according to Hooke's law.
        mSpringStiffnessCoefficientBuffer[springIndex] =
            simulationParameters.FSCommonSimulator.SpringReductionFraction
            * springs.GetMaterialStiffness(springIndex)
            * massFactor
            / dtSquared;

        // Damping coefficient
        // Magnitude of the drag force on the relative velocity component along the spring.
        mSpringDampingCoefficientBuffer[springIndex] =
            simulationParameters.FSCommonSimulator.SpringDampingCoefficient
            * massFactor
            / dt;
    }
}
