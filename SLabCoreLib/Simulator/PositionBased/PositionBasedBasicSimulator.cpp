/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "PositionBasedBasicSimulator.h"

#include <cassert>

PositionBasedBasicSimulator::PositionBasedBasicSimulator(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & /*threadManager*/)
    // Point buffers
    : mPointMassBuffer(object.GetPoints().GetBufferElementCount(), 0, 0.0f)
    , mPointExternalForceBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    , mPointPositionPredictionBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    // Spring buffers
    , mSpringScalingFactorsBuffer(object.GetSprings().GetBufferElementCount(), 0, SpringScalingFactors())
{
    CreateState(object, simulationParameters);
}

void PositionBasedBasicSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & /*threadManager*/)
{
    CreateState(object, simulationParameters);
}

void PositionBasedBasicSimulator::Update(
    Object & object,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters,
    ThreadManager & /*threadManager*/)
{
    for (size_t i = 0; i < simulationParameters.PositionBasedCommonSimulator.NumUpdateIterations; ++i)
    {
        IntegrateInitialDynamics(object, simulationParameters);

        for (size_t j = 0; j < simulationParameters.PositionBasedCommonSimulator.NumSolverIterations; ++j)
        {
            ProjectConstraints(object, simulationParameters);
        }

        FinalizeDynamics(object, simulationParameters);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

void PositionBasedBasicSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
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

        float const endpointAMass = mPointMassBuffer[endpointAIndex];
        float const endpointAMassInv = (1.0f / mPointMassBuffer[endpointAIndex]) * points.GetFrozenCoefficient(endpointAIndex);
        float const endpointBMass = mPointMassBuffer[endpointBIndex];
        float const endpointBMassInv = (1.0f / mPointMassBuffer[endpointBIndex]) * points.GetFrozenCoefficient(endpointBIndex);
        
        float const den = (endpointAMassInv + endpointBMassInv);
        mSpringScalingFactorsBuffer[springIndex].EndpointA = endpointAMassInv / (den == 0.0f ? 1.0f : den) * simulationParameters.PositionBasedCommonSimulator.SpringStiffness;
        mSpringScalingFactorsBuffer[springIndex].EndpointB = endpointBMassInv / (den == 0.0f ? 1.0f : den) * simulationParameters.PositionBasedCommonSimulator.SpringStiffness;
    }
}

void PositionBasedBasicSimulator::IntegrateInitialDynamics(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.PositionBasedCommonSimulator.NumUpdateIterations);

    float const globalDampingCoefficient = 1.0f - pow((1.0f - simulationParameters.PositionBasedCommonSimulator.GlobalDamping), 0.4f);

    vec2f const * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();
    float const * restrict const pointMassBuffer = mPointMassBuffer.data();
    float const * restrict const pointFrozenCoefficientBuffer = object.GetPoints().GetFrozenCoefficientBuffer();
    vec2f const * restrict const pointExternalForceBuffer = mPointExternalForceBuffer.data();
    vec2f * restrict const pointPositionPredictionBuffer = mPointPositionPredictionBuffer.data();

    for (ElementIndex p : object.GetPoints())
    {
        pointVelocityBuffer[p] =
            (pointVelocityBuffer[p] + pointExternalForceBuffer[p] * dt / pointMassBuffer[p] * pointFrozenCoefficientBuffer[p])
            * globalDampingCoefficient;

        pointPositionPredictionBuffer[p] = pointPositionBuffer[p] + pointVelocityBuffer[p] * dt;
    }
}

void PositionBasedBasicSimulator::ProjectConstraints(
    Object const & object,
    SimulationParameters const & /*simulationParameters*/)
{
    vec2f const * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    float const * restrict const pointMassBuffer = mPointMassBuffer.data();
    vec2f * restrict const pointPositionPredictionBuffer = mPointPositionPredictionBuffer.data();

    Springs::Endpoints const * restrict const endpointsBuffer = object.GetSprings().GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = object.GetSprings().GetRestLengthBuffer();
    SpringScalingFactors const * restrict const springScalingFactorsBuffer = mSpringScalingFactorsBuffer.data();

    for (ElementIndex s : object.GetSprings())
    {
        auto const endpointAIndex = endpointsBuffer[s].PointAIndex;
        auto const endpointBIndex = endpointsBuffer[s].PointBIndex;

        vec2f const displacement = pointPositionPredictionBuffer[endpointAIndex] - pointPositionPredictionBuffer[endpointBIndex];
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        float const strain = displacementLength - restLengthBuffer[s];

        vec2f const deltaPredictedPosA = -springDir * springScalingFactorsBuffer[s].EndpointA * strain;
        vec2f const deltaPredictedPosB = springDir * springScalingFactorsBuffer[s].EndpointB * strain;

        pointPositionPredictionBuffer[endpointAIndex] += deltaPredictedPosA;
        pointPositionPredictionBuffer[endpointBIndex] += deltaPredictedPosB;
    }
}

void PositionBasedBasicSimulator::FinalizeDynamics(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.PositionBasedCommonSimulator.NumUpdateIterations);

    vec2f * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f const * restrict const pointPositionPredictionBuffer = mPointPositionPredictionBuffer.data();

    for (ElementIndex p : object.GetPoints())
    {
        pointVelocityBuffer[p] = (pointPositionPredictionBuffer[p] - pointPositionBuffer[p]) / dt;
        pointPositionBuffer[p] = pointPositionPredictionBuffer[p];
    }
}