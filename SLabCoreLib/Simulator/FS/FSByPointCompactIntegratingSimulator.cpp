/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-17
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSByPointCompactIntegratingSimulator.h"

#include <array>
#include <cassert>

FSByPointCompactIntegratingSimulator::FSByPointCompactIntegratingSimulator(
    Object const & object,
    SimulationParameters const & simulationParameters)
    // Point buffers
    : mPointSpringForceBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    , mPointExternalForceBuffer(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero())
    , mPointIntegrationFactorBuffer(object.GetPoints().GetBufferElementCount(), 0, 0.0f)
    , mConnectedSpringsBuffer(object.GetPoints().GetBufferElementCount() * (sizeof(ElementIndex) + 2 * sizeof(ConnectedSpring) * object.GetSprings().GetElementCount()))
{
    CreateState(object, simulationParameters);
}

void FSByPointCompactIntegratingSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    CreateState(object, simulationParameters);
}

void FSByPointCompactIntegratingSimulator::Update(
    Object & object,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters)
{
    for (size_t i = 0; i < simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations; ++i)
    {
        // Apply spring forces and integrate
        ApplySpringsForcesAndIntegrate(object, simulationParameters);

        // Swap buffers now
        object.GetPoints().SwapPositionBuffers();
        object.GetPoints().SwapVelocityBuffers();
    }
}

///////////////////////////////////////////////////////////////////////////////////////////

void FSByPointCompactIntegratingSimulator::CreateState(
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
    // Visit springs
    //

    // Temp buffer containing max possible connected springs
    struct TmpConnectedSprings
    {
        ElementCount ConnectedSpringsCount;
        std::array<ConnectedSpring, SimulationParameters::MaxSpringsPerPoint> ConnectedSprings;

        TmpConnectedSprings()
            : ConnectedSpringsCount(0)
        {}
    };

    Buffer<TmpConnectedSprings> tmpConnectedSpringsBuffer(object.GetPoints().GetBufferElementCount(), 0, TmpConnectedSprings());

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
        float const springStiffnessCoefficient =
            simulationParameters.FSCommonSimulator.SpringReductionFraction
            * springs.GetMaterialStiffness(springIndex)
            * massFactor
            / dtSquared;

        // Damping coefficient
        // Magnitude of the drag force on the relative velocity component along the spring.
        float const springDampingCoefficient =
            simulationParameters.FSCommonSimulator.SpringDampingCoefficient
            * massFactor
            / dt;

        auto & tmpEndpointAConnectedSprings = tmpConnectedSpringsBuffer[endpointAIndex];
        auto & tmpEndpointBConnectedSprings = tmpConnectedSpringsBuffer[endpointBIndex];

        tmpEndpointAConnectedSprings.ConnectedSprings[tmpEndpointAConnectedSprings.ConnectedSpringsCount].OtherEndpointIndex = endpointBIndex;
        tmpEndpointBConnectedSprings.ConnectedSprings[tmpEndpointBConnectedSprings.ConnectedSpringsCount].OtherEndpointIndex = endpointAIndex;

        tmpEndpointAConnectedSprings.ConnectedSprings[tmpEndpointAConnectedSprings.ConnectedSpringsCount].StiffnessCoefficient = springStiffnessCoefficient;
        tmpEndpointBConnectedSprings.ConnectedSprings[tmpEndpointBConnectedSprings.ConnectedSpringsCount].StiffnessCoefficient = springStiffnessCoefficient;

        tmpEndpointAConnectedSprings.ConnectedSprings[tmpEndpointAConnectedSprings.ConnectedSpringsCount].DampingCoefficient = springDampingCoefficient;
        tmpEndpointBConnectedSprings.ConnectedSprings[tmpEndpointBConnectedSprings.ConnectedSpringsCount].DampingCoefficient = springDampingCoefficient;

        tmpEndpointAConnectedSprings.ConnectedSprings[tmpEndpointAConnectedSprings.ConnectedSpringsCount].RestLength = springs.GetRestLength(springIndex);
        tmpEndpointBConnectedSprings.ConnectedSprings[tmpEndpointBConnectedSprings.ConnectedSpringsCount].RestLength = springs.GetRestLength(springIndex);

        tmpEndpointAConnectedSprings.ConnectedSpringsCount++;
        tmpEndpointBConnectedSprings.ConnectedSpringsCount++;
    }

    //
    // Compact now
    //

    std::uint8_t * restrict connectedSpringsBuffer = mConnectedSpringsBuffer.data();

    for (auto pointIndex : points)
    {
        ElementCount & connectedSpringsCount = *reinterpret_cast<ElementCount *>(connectedSpringsBuffer);
        connectedSpringsCount = tmpConnectedSpringsBuffer[pointIndex].ConnectedSpringsCount;
        connectedSpringsBuffer += sizeof(ElementCount);

        for (ElementIndex s = 0; s < connectedSpringsCount; ++s)
        {
            ConnectedSpring & connectedSpring = *reinterpret_cast<ConnectedSpring *>(connectedSpringsBuffer);
            connectedSpring.StiffnessCoefficient = tmpConnectedSpringsBuffer[pointIndex].ConnectedSprings[s].StiffnessCoefficient;
            connectedSpring.DampingCoefficient = tmpConnectedSpringsBuffer[pointIndex].ConnectedSprings[s].DampingCoefficient;
            connectedSpring.RestLength = tmpConnectedSpringsBuffer[pointIndex].ConnectedSprings[s].RestLength;
            connectedSpring.OtherEndpointIndex = tmpConnectedSpringsBuffer[pointIndex].ConnectedSprings[s].OtherEndpointIndex;
            connectedSpringsBuffer += sizeof(ConnectedSpring);
        }
    }
}

void FSByPointCompactIntegratingSimulator::ApplySpringsForcesAndIntegrate(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    // Buffers
    vec2f const * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * restrict const newPointPositionBuffer = object.GetPoints().GetSecondaryPositionBuffer();
    vec2f const * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f * restrict const newPointVelocityBuffer = object.GetPoints().GetSecondaryVelocityBuffer();
    vec2f const * const restrict externalForceBuffer = mPointExternalForceBuffer.data();
    float const * const restrict integrationFactorBuffer = mPointIntegrationFactorBuffer.data();

    std::uint8_t const * restrict connectedSpringsBuffer = mConnectedSpringsBuffer.data();

    ElementCount const pointCount = object.GetPoints().GetElementCount();
    for (ElementIndex pointIndex = 0; pointIndex < pointCount; ++pointIndex)
    {
        vec2f const & thisPointPosition = pointPositionBuffer[pointIndex];
        vec2f const & thisPointVelocity = pointVelocityBuffer[pointIndex];

        vec2f pointForces = externalForceBuffer[pointIndex];

        ElementCount const connectedSpringsCount = *reinterpret_cast<ElementCount const *>(connectedSpringsBuffer);
        connectedSpringsBuffer += sizeof(ElementCount);
        for (ElementIndex s = 0; s < connectedSpringsCount; ++s)
        {
            ConnectedSpring const & connectedSpring = *reinterpret_cast<ConnectedSpring const *>(connectedSpringsBuffer);
            connectedSpringsBuffer += sizeof(ConnectedSpring);

            ElementIndex const otherEndpointIndex = connectedSpring.OtherEndpointIndex;

            vec2f const displacement = pointPositionBuffer[otherEndpointIndex] - thisPointPosition;
            float const displacementLength = displacement.length();
            vec2f const springDir = displacement.normalise(displacementLength);

            //
            // 1. Hooke's law
            //

            // Calculate spring force on point
            float const fSpring =
                (displacementLength - connectedSpring.RestLength)
                * connectedSpring.StiffnessCoefficient;

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
                * connectedSpring.DampingCoefficient;

            //
            // Apply forces
            //

            pointForces += springDir * (fSpring + fDamp);
        }

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        vec2f const deltaPos =
            thisPointVelocity * dt
            + pointForces * integrationFactorBuffer[pointIndex];

        newPointPositionBuffer[pointIndex] = thisPointPosition + deltaPos;
        newPointVelocityBuffer[pointIndex] = deltaPos * velocityFactor;
    }
}
