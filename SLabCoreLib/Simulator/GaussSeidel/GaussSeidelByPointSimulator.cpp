/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "GaussSeidelByPointSimulator.h"

#include <cassert>

GaussSeidelByPointSimulator::GaussSeidelByPointSimulator(
    Object const & object,
    SimulationParameters const & simulationParameters)
    // Point buffers
    : mPointExternalForceBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    , mPointIntegrationFactorBuffer(object.GetPoints().GetBufferElementCount(), 0, 0.0f)
    // Spring buffers
    , mSpringStiffnessCoefficientBuffer(object.GetSprings().GetBufferElementCount(), 0, 0.0f)
    , mSpringDampingCoefficientBuffer(object.GetSprings().GetBufferElementCount(), 0, 0.0f)
{
    CreateState(object, simulationParameters);
}

void GaussSeidelByPointSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    CreateState(object, simulationParameters);
}

void GaussSeidelByPointSimulator::Update(
    Object & object,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters)
{
    for (size_t i = 0; i < simulationParameters.GaussSeidelCommonSimulator.NumMechanicalDynamicsIterations; ++i)
    {
        // Integrate external forces and current velocities
        Integrate(object, simulationParameters);

        // Relax springs - updating positions and velocities
        RelaxSprings(object, simulationParameters);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

void GaussSeidelByPointSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.GaussSeidelCommonSimulator.NumMechanicalDynamicsIterations);
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
            simulationParameters.GaussSeidelCommonSimulator.SpringReductionFraction
            * springs.GetMaterialStiffness(springIndex)
            * massFactor
            / dtSquared;

        // Damping coefficient
        // Magnitude of the drag force on the relative velocity component along the spring.
        mSpringDampingCoefficientBuffer[springIndex] =
            simulationParameters.GaussSeidelCommonSimulator.SpringDampingCoefficient
            * massFactor
            / dt;
    }
}

void GaussSeidelByPointSimulator::Integrate(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.GaussSeidelCommonSimulator.NumMechanicalDynamicsIterations);

    float * const restrict positionBuffer = reinterpret_cast<float *>(object.GetPoints().GetPositionBuffer());
    float * const restrict velocityBuffer = reinterpret_cast<float *>(object.GetPoints().GetVelocityBuffer());
    float const * const restrict externalForceBuffer = reinterpret_cast<float *>(mPointExternalForceBuffer.data());
    float const * const restrict integrationFactorBuffer = reinterpret_cast<float *>(mPointIntegrationFactorBuffer.data());

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.GaussSeidelCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.GaussSeidelCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    size_t const count = object.GetPoints().GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + externalForceBuffer[i] * integrationFactorBuffer[i / 2];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;
    }
}

void GaussSeidelByPointSimulator::RelaxSprings(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.GaussSeidelCommonSimulator.NumMechanicalDynamicsIterations);

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.GaussSeidelCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.GaussSeidelCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    vec2f * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();
    float const * const restrict integrationFactorBuffer = reinterpret_cast<float *>(mPointIntegrationFactorBuffer.data());

    float const * restrict const restLengthBuffer = object.GetSprings().GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = mSpringStiffnessCoefficientBuffer.data();
    float const * restrict const dampingCoefficientBuffer = mSpringDampingCoefficientBuffer.data();

    for (auto const pointIndex: object.GetPoints())
    {
        vec2f const & thisPointPosition = pointPositionBuffer[pointIndex];
        vec2f const & thisPointVelocity = pointVelocityBuffer[pointIndex];

        vec2f springForces = vec2f::zero();
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

            springForces += springDir * (fSpring + fDamp);
        }

        // Integrate spring forces and update point's velocity
        vec2f const deltaPos = springForces * integrationFactorBuffer[pointIndex];
        pointPositionBuffer[pointIndex] += deltaPos;
        pointVelocityBuffer[pointIndex] += deltaPos * velocityFactor;
    }
}
