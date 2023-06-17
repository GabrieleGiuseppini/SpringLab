/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-04-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringStructuralIntrinsicsMTVectorizedSimulator.h"

#include "Log.h"

#include <array>
#include <cassert>
#include <numeric>

FSBySpringStructuralIntrinsicsMTVectorizedSimulator::FSBySpringStructuralIntrinsicsMTVectorizedSimulator(
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

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & threadManager)
{
    FSBySpringStructuralIntrinsicsSimulator::CreateState(object, simulationParameters, threadManager);

    // Clear threading state
    mSpringRelaxationTasks.clear();
    mPointSpringForceBuffers.clear();
    mPointSpringForceBuffersVectorized.clear();

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
        mPointSpringForceBuffersVectorized.emplace_back(reinterpret_cast<float *>(mPointSpringForceBuffers.back().data()));

        mSpringRelaxationTasks.emplace_back(
            [this, &object, pointSpringForceBuffer = mPointSpringForceBuffers.back().data(), springStart, springEnd]()
            {
                FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForcesVectorized(
                    object,
                    pointSpringForceBuffer,
                    springStart,
                    springEnd);
            });

        springStart = springEnd;
    }

    LogMessage("FSBySpringStructuralIntrinsicsMTVectorizedSimulator: numSprings=", object.GetSprings().GetElementCount(), " springPerfectSquareCount=", mSpringPerfectSquareCount,
        " numberOfFourSpringsPerThread=", numberOfFourSpringsPerThread, " numThreads=", parallelism);
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::ApplySpringsForces(
    Object const & /*object*/,
    ThreadManager & threadManager)
{
    //
    // Run algo
    //

    threadManager.GetSimulationThreadPool().Run(mSpringRelaxationTasks);
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    switch (mSpringRelaxationTasks.size())
    {
        case 1:
        {
            IntegrateAndResetSpringForces_1(object, simulationParameters);
            break;
        }

        case 2:
        {
            IntegrateAndResetSpringForces_2(object, simulationParameters);
            break;
        }

        case 4:
        {
            IntegrateAndResetSpringForces_4(object, simulationParameters);
            break;
        }

        default:
        {
            IntegrateAndResetSpringForces_N(object, simulationParameters);
            break;
        }
    }
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_1(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    float * const restrict positionBuffer = reinterpret_cast<float *>(object.GetPoints().GetPositionBuffer());
    float * const restrict velocityBuffer = reinterpret_cast<float *>(object.GetPoints().GetVelocityBuffer());
    float const * const restrict externalForceBuffer = reinterpret_cast<float *>(mPointExternalForceBuffer.data());
    float const * const restrict integrationFactorBuffer = reinterpret_cast<float *>(mPointIntegrationFactorBuffer.data());

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    assert(mPointSpringForceBuffers.size() == 1);
    float * const restrict springForceBuffer = reinterpret_cast<float *>(mPointSpringForceBuffers[0].data());

    size_t const count = object.GetPoints().GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (springForceBuffer[i] + externalForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring force now that we've integrated it
        springForceBuffer[i] = 0.0f;
    }
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_2(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    float * const restrict positionBuffer = reinterpret_cast<float *>(object.GetPoints().GetPositionBuffer());
    float * const restrict velocityBuffer = reinterpret_cast<float *>(object.GetPoints().GetVelocityBuffer());
    float const * const restrict externalForceBuffer = reinterpret_cast<float *>(mPointExternalForceBuffer.data());
    float const * const restrict integrationFactorBuffer = reinterpret_cast<float *>(mPointIntegrationFactorBuffer.data());

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    assert(mPointSpringForceBuffers.size() == 2);
    float * const restrict springForceBuffer1 = reinterpret_cast<float *>(mPointSpringForceBuffers[0].data());
    float * const restrict springForceBuffer2 = reinterpret_cast<float *>(mPointSpringForceBuffers[1].data());

    size_t const count = object.GetPoints().GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        float const springForce = springForceBuffer1[i] + springForceBuffer2[i];

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (springForce + externalForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        springForceBuffer1[i] = 0.0f;
        springForceBuffer2[i] = 0.0f;
    }
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_4(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    //
    // This implementation is compiled with vectorized instructions by VS 2022
    //

    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    float * const restrict positionBuffer = reinterpret_cast<float *>(object.GetPoints().GetPositionBuffer());
    float * const restrict velocityBuffer = reinterpret_cast<float *>(object.GetPoints().GetVelocityBuffer());
    float const * const restrict externalForceBuffer = reinterpret_cast<float *>(mPointExternalForceBuffer.data());
    float const * const restrict integrationFactorBuffer = reinterpret_cast<float *>(mPointIntegrationFactorBuffer.data());

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    assert(mPointSpringForceBuffers.size() == 4);
    float * const restrict springForceBuffer1 = reinterpret_cast<float *>(mPointSpringForceBuffers[0].data());
    float * const restrict springForceBuffer2 = reinterpret_cast<float *>(mPointSpringForceBuffers[1].data());
    float * const restrict springForceBuffer3 = reinterpret_cast<float *>(mPointSpringForceBuffers[2].data());
    float * const restrict springForceBuffer4 = reinterpret_cast<float *>(mPointSpringForceBuffers[3].data());

    size_t const count = object.GetPoints().GetBufferElementCount() * 2; // Two components per vector
    for (size_t i = 0; i < count; ++i)
    {
        float const springForce = springForceBuffer1[i] + springForceBuffer2[i] + springForceBuffer3[i] + springForceBuffer4[i];

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        float const deltaPos =
            velocityBuffer[i] * dt
            + (springForce + externalForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;

        // Zero out spring forces now that we've integrated them
        springForceBuffer1[i] = 0.0f;
        springForceBuffer2[i] = 0.0f;
        springForceBuffer3[i] = 0.0f;
        springForceBuffer4[i] = 0.0f;
    }
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_N(
    Object & object,
    SimulationParameters const & simulationParameters)
{
#if !FS_IS_ARCHITECTURE_X86_32() && !FS_IS_ARCHITECTURE_X86_64()
#error Unsupported Architecture
#endif
    static_assert(vectorization_float_count<int> >= 4);

    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    float * const restrict positionBuffer = reinterpret_cast<float *>(object.GetPoints().GetPositionBuffer());
    float * const restrict velocityBuffer = reinterpret_cast<float *>(object.GetPoints().GetVelocityBuffer());
    float const * const restrict externalForceBuffer = reinterpret_cast<float *>(mPointExternalForceBuffer.data());
    float const * const restrict integrationFactorBuffer = reinterpret_cast<float *>(mPointIntegrationFactorBuffer.data());

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    size_t const nBuffers = mPointSpringForceBuffersVectorized.size();
    float * restrict * restrict const pointSprigForceBufferOfBuffers = mPointSpringForceBuffersVectorized.data();

    assert((object.GetPoints().GetBufferElementCount() % 2) == 0);
    size_t const count = object.GetPoints().GetBufferElementCount() * 2; // Two components per vector

    __m128 const zero_4 = _mm_setzero_ps();
    __m128 const dt_4 = _mm_load1_ps(&dt);
    __m128 const velocityFactor_4 = _mm_load1_ps(&velocityFactor);

    for (size_t p = 0; p < count; p += 4)
    {
        __m128 springForce_2 = zero_4;
        for (size_t b = 0; b < nBuffers; ++b)
        {
            springForce_2 =
                _mm_add_ps(
                    springForce_2,
                    _mm_load_ps(pointSprigForceBufferOfBuffers[b] + p));
        }

        // vec2f const deltaPos =
        //    velocityBuffer[i] * dt
        //    + (springForceBuffer[i] + externalForceBuffer[i]) * integrationFactorBuffer[i];
        __m128 const deltaPos_2 =
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_load_ps(velocityBuffer + p),
                    dt_4),
                _mm_mul_ps(
                    _mm_add_ps(
                        springForce_2,
                        _mm_load_ps(externalForceBuffer + p)),
                    _mm_load_ps(integrationFactorBuffer + p)));

        // positionBuffer[i] += deltaPos;
        __m128 pos_2 = _mm_load_ps(positionBuffer + p);
        pos_2 = _mm_add_ps(pos_2, deltaPos_2);
        _mm_store_ps(positionBuffer + p, pos_2);

        // velocityBuffer[i] = deltaPos * velocityFactor;
        __m128 const vel_2 =
            _mm_mul_ps(
                deltaPos_2,
                velocityFactor_4);
        _mm_store_ps(velocityBuffer + p, vel_2);

        for (size_t b = 0; b < nBuffers; ++b)
        {
            _mm_store_ps(pointSprigForceBufferOfBuffers[b] + p, zero_4);
        }
    }
}