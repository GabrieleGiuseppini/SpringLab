/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSByPointSimulator.h"

#include <cassert>

FSByPointSimulator::FSByPointSimulator(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & /*threadManager*/)
    // Point buffers
    : mPointSpringForceBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    , mPointExternalForceBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    , mPointIntegrationFactorBuffer(object.GetPoints().GetBufferElementCount(), 0, 0.0f)
    // Spring buffers
    , mSpringStiffnessCoefficientBuffer(object.GetSprings().GetBufferElementCount(), 0, 0.0f)
    , mSpringDampingCoefficientBuffer(object.GetSprings().GetBufferElementCount(), 0, 0.0f)
{
    CreateState(object, simulationParameters);
}

void FSByPointSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & /*threadManager*/)
{
    CreateState(object, simulationParameters);
}

void FSByPointSimulator::Update(
    Object & object,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters,
    ThreadManager & /*threadManager*/)
{
    for (size_t i = 0; i < simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations; ++i)
    {
        // Apply spring forces
        ApplySpringsForces(object);

        // Integrate spring and external forces,
        // and reset spring forces
        IntegrateAndResetSpringForces(object, simulationParameters);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

void FSByPointSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);
    float const dtSquared = dt * dt;

    //
    // Initialize point buffers
    //

    Points const & points = object.GetPoints();

    for (auto pointIndex : points)
    {
        mPointSpringForceBuffer[pointIndex] = vec2f::zero();

        mPointExternalForceBuffer[pointIndex] =
            simulationParameters.Common.AssignedGravity * points.GetMass(pointIndex) * simulationParameters.Common.MassAdjustment
            + points.GetAssignedForce(pointIndex);

        mPointIntegrationFactorBuffer[pointIndex] =
            dtSquared
            / (points.GetMass(pointIndex) * simulationParameters.Common.MassAdjustment)
            * points.GetFrozenCoefficient(pointIndex);
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

void FSByPointSimulator::ApplySpringsForces(Object const & object)
{
    vec2f const * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f const * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f * restrict const pointSpringForceBuffer = mPointSpringForceBuffer.data();

    float const * restrict const restLengthBuffer = object.GetSprings().GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = mSpringStiffnessCoefficientBuffer.data();
    float const * restrict const dampingCoefficientBuffer = mSpringDampingCoefficientBuffer.data();

    ElementCount const pointCount = object.GetPoints().GetElementCount();
    for (ElementIndex pointIndex = 0; pointIndex < pointCount; ++pointIndex)
    {
        vec2f const & thisPointPosition = pointPositionBuffer[pointIndex];
        vec2f const & thisPointVelocity = pointVelocityBuffer[pointIndex];

        vec2f pointForce = vec2f::zero();

        for (auto const & connectedSpring : object.GetPoints().GetConnectedSprings(pointIndex))
        {
            ElementIndex const otherEndpointIndex = connectedSpring.OtherEndpointIndex;

            vec2f const displacement = pointPositionBuffer[otherEndpointIndex] - thisPointPosition;
            float const displacementLength = displacement.length();
            vec2f const springDir = displacement.normalise(displacementLength);

            //
            // 1. Hooke's law
            //

            // Calculate spring force on point
            float const fSpring =
                (displacementLength - restLengthBuffer[connectedSpring.SpringIndex])
                * stiffnessCoefficientBuffer[connectedSpring.SpringIndex];

            //
            // 2. Damper forces
            //
            // Damp the velocities of the two points, as if the points were also connected by a damper
            // along the same direction as the spring
            //

            // Calculate damp force on point A
            vec2f const relVelocity = pointVelocityBuffer[otherEndpointIndex] - thisPointVelocity;
            float const fDamp =
                relVelocity.dot(springDir)
                * dampingCoefficientBuffer[connectedSpring.SpringIndex];

            //
            // Apply forces
            //

            pointForce += springDir * (fSpring + fDamp);
        }

        pointSpringForceBuffer[pointIndex] = pointForce;
    }
}

void FSByPointSimulator::IntegrateAndResetSpringForces(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f * const restrict positionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * const restrict velocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f * const restrict springForceBuffer = mPointSpringForceBuffer.data();
    vec2f const * const restrict externalForceBuffer = mPointExternalForceBuffer.data();
    float const * const restrict integrationFactorBuffer = mPointIntegrationFactorBuffer.data();

    float const globalDamping = 
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    size_t const count = object.GetPoints().GetBufferElementCount();
    for (size_t i = 0; i < count; ++i)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        vec2f const deltaPos =
            velocityBuffer[i] * dt
            + (springForceBuffer[i] + externalForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // TODOHERE: this not needed?
        ////// Zero out spring force now that we've integrated it
        ////springForceBuffer[i] = vec2f::zero();
    }
}