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
    : mPointMassBuffer(object.GetPoints().GetBufferElementCount(), 0, 0.0f)
    , mPointExternalForceBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    , mPointPositionPredictionBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
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
    IntegrateInitialDynamics(object, simulationParameters);
    ProjectConstraints(object, simulationParameters);
    FinalizeDynamics(object, simulationParameters);
}

///////////////////////////////////////////////////////////////////////////////////////////

void PositionBasedBasicSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    // TODOHERE

    float const dt = simulationParameters.Common.SimulationTimeStepDuration;

    float const dtSquared = dt * dt;

    //
    // Initialize point buffers
    //

    Points const & points = object.GetPoints();

    for (auto pointIndex : points)
    {
        mPointMassBuffer[pointIndex] = points.GetMass(pointIndex) * simulationParameters.Common.MassAdjustment;

        mPointExternalForceBuffer[pointIndex] =
            simulationParameters.Common.AssignedGravity * mPointMassBuffer[pointIndex]
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

void PositionBasedBasicSimulator::IntegrateInitialDynamics(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.PositionBasedCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f const * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();
    float const * restrict const pointMassBuffer = mPointMassBuffer.data();
    float const * restrict const pointFrozenCoefficientBuffer = object.GetPoints().GetFrozenCoefficientBuffer();
    vec2f const * restrict const pointExternalForceBuffer = mPointExternalForceBuffer.data();
    vec2f * restrict const pointPositionPredictionBuffer = mPointPositionPredictionBuffer.data();

    for (ElementIndex p : object.GetPoints())
    {
        pointVelocityBuffer[p] += pointExternalForceBuffer[p] * dt / pointMassBuffer[p] * pointFrozenCoefficientBuffer[p];
        pointPositionPredictionBuffer[p] = pointPositionBuffer[p] + pointVelocityBuffer[p] * dt;
    }

    // TODO: velocity damping
}

void PositionBasedBasicSimulator::ProjectConstraints(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    // TODOHERE
    (void)object;
    (void)simulationParameters;
}

void PositionBasedBasicSimulator::FinalizeDynamics(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.PositionBasedCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f const * restrict const pointPositionPredictionBuffer = mPointPositionPredictionBuffer.data();

    for (ElementIndex p : object.GetPoints())
    {
        pointVelocityBuffer[p] = (pointPositionPredictionBuffer[p] - pointPositionBuffer[p]) / dt;
        pointPositionBuffer[p] = pointPositionPredictionBuffer[p];
    }
}