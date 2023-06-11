/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-08
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator.h"

#include "Log.h"

#include <array>
#include <cassert>
#include <numeric>

FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & threadManager)
    : FSBySpringStructuralIntrinsicsSimulator(
        object,
        simulationParameters,
        threadManager)
{
    // CreateState() on base has been called; our turn now
    CreateState(object, simulationParameters, threadManager);
}

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & threadManager)
{
    FSBySpringStructuralIntrinsicsSimulator::CreateState(object, simulationParameters, threadManager);

    // Clear threading state
    mSpringRelaxationTasks.clear();
    mPointSpringForceBuffers.clear();

    // Number of 4-spring blocks per thread, assuming we use all parallelism
    ElementCount const numberOfSprings = static_cast<ElementCount>(object.GetSprings().GetElementCount());
    ElementCount const numberOfFourSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(threadManager.GetSimulationParallelism()) * 4);

    size_t parallelism;
    if (numberOfFourSpringsPerThread > 0)
    {
        parallelism = threadManager.GetSimulationParallelism();
    }
    else
    {
        // Not enough, use just one thread
        parallelism = 1;
    }

    ElementIndex springStart = 0;
    for (size_t t = 0; t < parallelism; ++t)
    {
        ElementIndex const springEnd = (t < parallelism - 1)
            ? springStart + numberOfFourSpringsPerThread * 4
            : numberOfSprings;

        // Create helper buffer for this thread
        mPointSpringForceBuffers.emplace_back(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero());

        mSpringRelaxationTasks.emplace_back(
            [this, &object, pointSpringForceBuffer = mPointSpringForceBuffers.back().data(), springStart, springEnd]()
            {
                ApplySpringsForcesPseudoVectorized(
                    object,
                    pointSpringForceBuffer,
                    springStart,
                    springEnd);
            });

        springStart = springEnd;
    }

    LogMessage("FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator: numSprings=", object.GetSprings().GetElementCount(), " springPerfectSquareCount=", mSpringPerfectSquareCount,
        " numberOfFourSpringsPerThread=", numberOfFourSpringsPerThread, " numThreads=", parallelism);
}

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::ApplySpringsForces(
    Object const & /*object*/,
    ThreadManager & threadManager)
{
    //
    // Run algo
    //

    threadManager.GetSimulationThreadPool().Run(mSpringRelaxationTasks);
}

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::ApplySpringsForcesPseudoVectorized(
    Object const & object,
    vec2f * restrict pointSpringForceBuffer,
    ElementIndex startSpringIndex,
    ElementCount endSpringIndex)  // Excluded
{
    static_assert(vectorization_float_count<int> == 4);

    vec2f const * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f const * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();

    Springs::Endpoints const * restrict const endpointsBuffer = object.GetSprings().GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = object.GetSprings().GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = mSpringStiffnessCoefficientBuffer.data();
    float const * restrict const dampingCoefficientBuffer = mSpringDampingCoefficientBuffer.data();
    
    ElementIndex s = startSpringIndex;

    //
    // 1. Perfect squares
    //

    ElementCount const endSpringIndexPerfectSquare = std::min(endSpringIndex, mSpringPerfectSquareCount);

    for (; s < endSpringIndexPerfectSquare; s += 4)
    {
        //
        //    J          M   ---  a
        //    |\        /|
        //    | \s0  s1/ |
        //    |  \    /  |
        //  s2|   \  /   |s3
        //    |    \/    |
        //    |    /\    |
        //    |   /  \   |
        //    |  /    \  |
        //    | /      \ |
        //    |/        \|
        //    K          L  ---  b
        //

        //
        // Calculate displacements, string lengths, and spring directions
        //

        ElementIndex const pointJIndex = endpointsBuffer[s + 0].PointAIndex;
        ElementIndex const pointKIndex = endpointsBuffer[s + 1].PointBIndex;
        ElementIndex const pointLIndex = endpointsBuffer[s + 0].PointBIndex;
        ElementIndex const pointMIndex = endpointsBuffer[s + 1].PointAIndex;

        assert(pointJIndex == endpointsBuffer[s + 2].PointAIndex);
        assert(pointKIndex == endpointsBuffer[s + 2].PointBIndex);
        assert(pointLIndex == endpointsBuffer[s + 3].PointBIndex);
        assert(pointMIndex == endpointsBuffer[s + 3].PointAIndex);

        vec2f const pointJPos = pointPositionBuffer[pointJIndex];
        vec2f const pointKPos = pointPositionBuffer[pointKIndex];
        vec2f const pointLPos = pointPositionBuffer[pointLIndex];
        vec2f const pointMPos = pointPositionBuffer[pointMIndex];

        vec2f const s0_dis = pointLPos - pointJPos;
        vec2f const s1_dis = pointKPos - pointMPos;
        vec2f const s2_dis = pointKPos - pointJPos;
        vec2f const s3_dis = pointLPos - pointMPos;

        float const s0_len = s0_dis.length();
        float const s1_len = s1_dis.length();
        float const s2_len = s2_dis.length();
        float const s3_len = s3_dis.length();

        vec2f const s0_dir = s0_dis.normalise(s0_len);
        vec2f const s1_dir = s1_dis.normalise(s1_len);
        vec2f const s2_dir = s2_dis.normalise(s2_len);
        vec2f const s3_dir = s3_dis.normalise(s3_len);
        
        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoint A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]
        //
        // Strategy:
        //
        // ( springLength[s0] - restLength[s0] ) * stiffness[s0]
        // ( springLength[s1] - restLength[s1] ) * stiffness[s1]
        // ( springLength[s2] - restLength[s2] ) * stiffness[s2]
        // ( springLength[s3] - restLength[s3] ) * stiffness[s3]
        //

        float const s0_hookForceMag = (s0_len - restLengthBuffer[s]) * stiffnessCoefficientBuffer[s];
        float const s1_hookForceMag = (s1_len - restLengthBuffer[s + 1]) * stiffnessCoefficientBuffer[s + 1];
        float const s2_hookForceMag = (s2_len - restLengthBuffer[s + 2]) * stiffnessCoefficientBuffer[s + 2];
        float const s3_hookForceMag = (s3_len - restLengthBuffer[s + 3]) * stiffnessCoefficientBuffer[s + 3];

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //

        vec2f const pointJVel = pointVelocityBuffer[pointJIndex];
        vec2f const pointKVel = pointVelocityBuffer[pointKIndex];
        vec2f const pointLVel = pointVelocityBuffer[pointLIndex];
        vec2f const pointMVel = pointVelocityBuffer[pointMIndex];

        vec2f const s0_relVel = pointLVel - pointJVel;
        vec2f const s1_relVel = pointKVel - pointMVel;
        vec2f const s2_relVel = pointKVel - pointJVel;
        vec2f const s3_relVel = pointLVel - pointMVel;

        float const s0_dampForceMag = s0_relVel.dot(s0_dir) * dampingCoefficientBuffer[s];
        float const s1_dampForceMag = s1_relVel.dot(s1_dir) * dampingCoefficientBuffer[s + 1];
        float const s2_dampForceMag = s2_relVel.dot(s2_dir) * dampingCoefficientBuffer[s + 2];
        float const s3_dampForceMag = s3_relVel.dot(s3_dir) * dampingCoefficientBuffer[s + 3];

        //
        // 3. Apply forces: 
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //

        vec2f const s0_forceA = s0_dir * (s0_hookForceMag + s0_dampForceMag);
        vec2f const s1_forceA = s1_dir * (s1_hookForceMag + s1_dampForceMag);
        vec2f const s2_forceA = s2_dir * (s2_hookForceMag + s2_dampForceMag);
        vec2f const s3_forceA = s3_dir * (s3_hookForceMag + s3_dampForceMag);

        pointSpringForceBuffer[pointJIndex] += (s0_forceA + s2_forceA);
        pointSpringForceBuffer[pointLIndex] -= (s0_forceA + s3_forceA);
        pointSpringForceBuffer[pointMIndex] += (s1_forceA + s3_forceA);
        pointSpringForceBuffer[pointKIndex] -= (s1_forceA + s2_forceA);
    }

    //
    // 2. Remaining one-by-one's
    //

    for (; s < endSpringIndex; ++s)
    {
        auto const pointAIndex = endpointsBuffer[s].PointAIndex;
        auto const pointBIndex = endpointsBuffer[s].PointBIndex;

        vec2f const displacement = pointPositionBuffer[pointBIndex] - pointPositionBuffer[pointAIndex];
        float const displacementLength = displacement.length();
        vec2f const springDir = displacement.normalise(displacementLength);

        //
        // 1. Hooke's law
        //

        // Calculate spring force on point A
        float const fSpring =
            (displacementLength - restLengthBuffer[s])
            * stiffnessCoefficientBuffer[s];

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // Calculate damp force on point A
        vec2f const relVelocity = pointVelocityBuffer[pointBIndex] - pointVelocityBuffer[pointAIndex];
        float const fDamp =
            relVelocity.dot(springDir)
            * dampingCoefficientBuffer[s];

        //
        // 3. Apply forces
        //

        vec2f const forceA = springDir * (fSpring + fDamp);
        pointSpringForceBuffer[pointAIndex] += forceA;
        pointSpringForceBuffer[pointBIndex] -= forceA;
    }
}

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    switch (mSpringRelaxationTasks.size())
    {
        case 1:
        {
            IntegrateAndResetSpringForces_N<1>(object, simulationParameters);
            break;
        }

        case 2:
        {
            IntegrateAndResetSpringForces_N<2>(object, simulationParameters);
            break;
        }

        case 3:
        {
            IntegrateAndResetSpringForces_N<3>(object, simulationParameters);
            break;
        }

        case 4:
        {
            IntegrateAndResetSpringForces_N<4>(object, simulationParameters);
            break;
        }

        case 5:
        {
            IntegrateAndResetSpringForces_N<5>(object, simulationParameters);
            break;
        }

        case 6:
        {
            IntegrateAndResetSpringForces_N<6>(object, simulationParameters);
            break;
        }

        case 7:
        {
            IntegrateAndResetSpringForces_N<7>(object, simulationParameters);
            break;
        }

        case 8:
        {
            IntegrateAndResetSpringForces_N<8>(object, simulationParameters);
            break;
        }

        default:
        {
            IntegrateAndResetSpringForces_NN(mSpringRelaxationTasks.size(), object, simulationParameters);
            break;
        }
    }
}

template<size_t N>
void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_N(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f * const restrict positionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * const restrict velocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f const * const restrict externalForceBuffer = mPointExternalForceBuffer.data();
    vec2f const * const restrict integrationFactorBuffer = mPointIntegrationFactorBuffer.data();

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    size_t const count = object.GetPoints().GetBufferElementCount();
    for (ElementIndex p : object.GetPoints())
    {
        vec2f springForce = vec2f::zero();
        for (size_t t = 0; t < N; ++t)
        {
            springForce += mPointSpringForceBuffers[t][p];
        }

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        vec2f const deltaPos =
            velocityBuffer[p] * dt
            + (springForce + externalForceBuffer[p]) * integrationFactorBuffer[p];

        positionBuffer[p] += deltaPos;
        velocityBuffer[p] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        for (size_t t = 0; t < N; ++t)
        {
            mPointSpringForceBuffers[t][p] = vec2f::zero();
        }
    }
}

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_NN(
    size_t n,
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f * const restrict positionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * const restrict velocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f const * const restrict externalForceBuffer = mPointExternalForceBuffer.data();
    vec2f const * const restrict integrationFactorBuffer = mPointIntegrationFactorBuffer.data();

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    size_t const count = object.GetPoints().GetBufferElementCount();
    for (ElementIndex p : object.GetPoints())
    {
        vec2f springForce = vec2f::zero();
        for (size_t t = 0; t < n; ++t)
        {
            springForce += mPointSpringForceBuffers[t][p];
        }

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        vec2f const deltaPos =
            velocityBuffer[p] * dt
            + (springForce + externalForceBuffer[p]) * integrationFactorBuffer[p];

        positionBuffer[p] += deltaPos;
        velocityBuffer[p] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        for (size_t t = 0; t < n; ++t)
        {
            mPointSpringForceBuffers[t][p] = vec2f::zero();
        }
    }
}
