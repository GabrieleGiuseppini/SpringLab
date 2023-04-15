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
    SimulationParameters const & simulationParameters)
    : FSBySpringStructuralIntrinsicsSimulator(
        object,
        simulationParameters)
{
    // CreateState() from base has been called; our turn now
    CreateState(object, simulationParameters);
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    FSBySpringStructuralIntrinsicsSimulator::CreateState(object, simulationParameters);

    // Clear threading state
    mThreadPool.reset();
    mSpringRelaxationTasks.clear();
    mPointSpringForceBuffers.clear();

    // Number of 4-spring blocks per thread
    assert(simulationParameters.Common.NumberOfThreads > 0);
    ElementCount const numberOfSprings = static_cast<ElementCount>(object.GetSprings().GetElementCount());
    ElementCount const numberOfFourSpringsPerThread = numberOfSprings / (static_cast<ElementCount>(simulationParameters.Common.NumberOfThreads) * 4);

    size_t numThreads;
    if (numberOfFourSpringsPerThread > 0)
    {
        numThreads = simulationParameters.Common.NumberOfThreads;
    }
    else
    {
        // Not enough, use just one thread
        numThreads = 1;
    }

    ElementIndex springStart = 0;
    for (size_t t = 0; t < numThreads; ++t)
    {
        ElementIndex const springEnd = (t < numThreads - 1)
            ? springStart + numberOfFourSpringsPerThread * 4
            : numberOfSprings;

        // Create helper buffer for this thread
        mPointSpringForceBuffers.emplace_back(object.GetPoints().GetBufferElementCount(), 0, vec2f::zero());

        mSpringRelaxationTasks.emplace_back(
            [this, &object, pointSpringForceBuffer = mPointSpringForceBuffers.back().data(), springStart, springEnd]()
            {
                FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForces(
                    object,
                    pointSpringForceBuffer,
                    springStart,
                    springEnd);
            });

        springStart = springEnd;
    }

    mThreadPool = std::make_unique<TaskThreadPool>(numThreads);
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::ApplySpringsForces(
    Object const & /*object*/)
{
    //
    // Run algo
    //

    mThreadPool->Run(mSpringRelaxationTasks);
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces(
    Object & object,
    SimulationParameters const & simulationParameters)
{
    switch (mThreadPool->GetParallelism())
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
#if !FS_IS_ARCHITECTURE_X86_32() && !FS_IS_ARCHITECTURE_X86_64()
#error Unsupported Architecture
#endif    

    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f * const restrict positionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * const restrict velocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f const * const restrict externalForceBuffer = mPointExternalForceBuffer.data();
    float const * const restrict integrationFactorBuffer = mPointIntegrationFactorBuffer.data();

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    assert(mPointSpringForceBuffers.size() == 1);
    vec2f * const restrict springForceBuffer = mPointSpringForceBuffers[0].data();
    
    size_t const count = object.GetPoints().GetBufferElementCount();
    assert((count % 2) == 0);

    __m128 const zero_4 = _mm_setzero_ps();
    __m128 const dt_4 = _mm_load1_ps(&dt);
    __m128 const velocityFactor_4 = _mm_load1_ps(&velocityFactor);

    for (size_t i = 0; i < count; i += 2)
    {
        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        // vec2f const deltaPos =
        //    velocityBuffer[i] * dt
        //    + (springForceBuffer[i] + externalForceBuffer[i]) * integrationFactorBuffer[i];
        __m128 const deltaPos_2 =
            _mm_add_ps(
                _mm_mul_ps(
                    _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(velocityBuffer + i))),
                    dt_4),
                _mm_add_ps(
                    _mm_add_ps(
                        _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(springForceBuffer + i))),
                        _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(externalForceBuffer + i)))),
                    _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(integrationFactorBuffer + i)))));

        // positionBuffer[i] += deltaPos;
        __m128 pos_2 = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(positionBuffer + i)));
        pos_2 = _mm_add_ps(pos_2, deltaPos_2);
        _mm_store_ps(reinterpret_cast<float *>(positionBuffer + i), pos_2);

        // velocityBuffer[i] = deltaPos * velocityFactor;
        __m128 const vel_2 =
            _mm_mul_ps(
                deltaPos_2,
                velocityFactor_4);
        _mm_store_ps(reinterpret_cast<float *>(velocityBuffer + i), vel_2);

        // Zero out spring force now that we've integrated it
        _mm_store_ps(reinterpret_cast<float *>(springForceBuffer + i), zero_4);
    }
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_2(
    Object & object,
    SimulationParameters const & simulationParameters)
{
#if !FS_IS_ARCHITECTURE_X86_32() && !FS_IS_ARCHITECTURE_X86_64()
#error Unsupported Architecture
#endif    

    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f * const restrict positionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * const restrict velocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f const * const restrict externalForceBuffer = mPointExternalForceBuffer.data();
    float const * const restrict integrationFactorBuffer = mPointIntegrationFactorBuffer.data();

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    assert(mPointSpringForceBuffers.size() == 2);
    vec2f * const restrict springForceBuffer1 = mPointSpringForceBuffers[0].data();
    vec2f * const restrict springForceBuffer2 = mPointSpringForceBuffers[1].data();

    size_t const count = object.GetPoints().GetBufferElementCount();
    for (size_t i = 0; i < count; ++i)
    {
        vec2f const springForce = springForceBuffer1[i] + springForceBuffer2[i];
        springForceBuffer1[i] = vec2f::zero();
        springForceBuffer2[i] = vec2f::zero();

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        vec2f const deltaPos =
            velocityBuffer[i] * dt
            + (springForce + externalForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;
    }
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_4(
    Object & object,
    SimulationParameters const & simulationParameters)
{
#if !FS_IS_ARCHITECTURE_X86_32() && !FS_IS_ARCHITECTURE_X86_64()
#error Unsupported Architecture
#endif    

    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f * const restrict positionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * const restrict velocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f const * const restrict externalForceBuffer = mPointExternalForceBuffer.data();
    float const * const restrict integrationFactorBuffer = mPointIntegrationFactorBuffer.data();

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    assert(mPointSpringForceBuffers.size() == 4);
    vec2f * const restrict springForceBuffer1 = mPointSpringForceBuffers[0].data();
    vec2f * const restrict springForceBuffer2 = mPointSpringForceBuffers[1].data();
    vec2f * const restrict springForceBuffer3 = mPointSpringForceBuffers[2].data();
    vec2f * const restrict springForceBuffer4 = mPointSpringForceBuffers[3].data();

    size_t const count = object.GetPoints().GetBufferElementCount();
    for (size_t i = 0; i < count; ++i)
    {
        vec2f const springForce = springForceBuffer1[i] + springForceBuffer2[i] + springForceBuffer3[i] + springForceBuffer4[i];
        springForceBuffer1[i] = vec2f::zero();
        springForceBuffer2[i] = vec2f::zero();
        springForceBuffer3[i] = vec2f::zero();
        springForceBuffer4[i] = vec2f::zero();

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        vec2f const deltaPos =
            velocityBuffer[i] * dt
            + (springForce + externalForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;
    }
}

void FSBySpringStructuralIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_N(
    Object & object,
    SimulationParameters const & simulationParameters)
{
#if !FS_IS_ARCHITECTURE_X86_32() && !FS_IS_ARCHITECTURE_X86_64()
#error Unsupported Architecture
#endif    

    float const dt = simulationParameters.Common.SimulationTimeStepDuration / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations);

    vec2f * const restrict positionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f * const restrict velocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f const * const restrict externalForceBuffer = mPointExternalForceBuffer.data();
    float const * const restrict integrationFactorBuffer = mPointIntegrationFactorBuffer.data();

    float const globalDamping =
        1.0f -
        pow((1.0f - simulationParameters.FSCommonSimulator.GlobalDamping),
            12.0f / static_cast<float>(simulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations));

    // Pre-divide damp coefficient by dt to provide the scalar factor which, when multiplied with a displacement,
    // provides the final, damped velocity
    float const velocityFactor = (1.0f - globalDamping) / dt;

    ///////////////////////

    size_t const count = object.GetPoints().GetBufferElementCount();
    for (size_t i = 0; i < count; ++i)
    {
        vec2f springForce = vec2f::zero();
        for (size_t b = 0; b < mPointSpringForceBuffers.size(); ++b)
        {
            springForce += mPointSpringForceBuffers[b][i];
            mPointSpringForceBuffers[b][i] = vec2f::zero();
        }

        //
        // Verlet integration (fourth order, with velocity being first order)
        //

        vec2f const deltaPos =
            velocityBuffer[i] * dt
            + (springForce + externalForceBuffer[i]) * integrationFactorBuffer[i];

        positionBuffer[i] += deltaPos;
        velocityBuffer[i] = deltaPos * velocityFactor;
    }
}

