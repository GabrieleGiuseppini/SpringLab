/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringIntrinsics.h"

#include "SysSpecifics.h"

#include <cassert>

FSBySpringIntrinsics::FSBySpringIntrinsics(
    Object const & object,
    SimulationParameters const & simulationParameters)
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

void FSBySpringIntrinsics::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    CreateState(object, simulationParameters);
}

void FSBySpringIntrinsics::Update(
    Object & object,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters)
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

void FSBySpringIntrinsics::CreateState(
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

void FSBySpringIntrinsics::ApplySpringsForces(Object const & object)
{
#if !FS_IS_ARCHITECTURE_X86_32() && !FS_IS_ARCHITECTURE_X86_64()
#error Unsupported Architecture
#endif

    vec2f const * restrict const pointPositionBuffer = object.GetPoints().GetPositionBuffer();
    vec2f const * restrict const pointVelocityBuffer = object.GetPoints().GetVelocityBuffer();
    vec2f * restrict const pointSpringForceBuffer = mPointSpringForceBuffer.data();

    Springs::Endpoints const * restrict const endpointsBuffer = object.GetSprings().GetEndpointsBuffer();
    float const * restrict const restLengthBuffer = object.GetSprings().GetRestLengthBuffer();
    float const * restrict const stiffnessCoefficientBuffer = mSpringStiffnessCoefficientBuffer.data();
    float const * restrict const dampingCoefficientBuffer = mSpringDampingCoefficientBuffer.data();

    __m128 const Zero = _mm_setzero_ps();

    ElementCount const springCount = object.GetSprings().GetElementCount();
    ElementCount const springVectorizedCount = springCount - (springCount % 4);
    ElementIndex s = 0;

    // Word-by-word
    for (; s < springVectorizedCount; s += 4)
    {
        //
        // Calculate displacement
        // 
        
        // Spring 0 displacement
        __m128 const s0pa_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 0].PointAIndex)));
        __m128 const s0pb_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 0].PointBIndex)));
        // pos[s0.B].x - pos[s0.A].x, pos[s0.B].y - pos[s0.A].y, *, *
        __m128 const s0_displacement = _mm_sub_ps(s0pb_pos, s0pa_pos);

        // Spring 1 displacement
        __m128 const s1pa_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 1].PointAIndex)));
        __m128 const s1pb_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 1].PointBIndex)));
        // pos[s1.B].x - pos[s1.A].x, pos[s1.B].y - pos[s1.A].y, *, *
        __m128 const s1_displacement = _mm_sub_ps(s1pb_pos, s1pa_pos);

        // s0_displacement.x, s0_displacement.y, s1_displacement.x, s1_displacement.y
        __m128 const s0s1_displacement = _mm_movelh_ps(s0_displacement, s1_displacement);

        // Spring 2 displacement
        __m128 const s2pa_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 2].PointAIndex)));
        __m128 const s2pb_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 2].PointBIndex)));
        // pos[s2.B].x - pos[s2.A].x, pos[s2.B].y - pos[s2.A].y, *, *
        __m128 const s2_displacement = _mm_sub_ps(s2pb_pos, s2pa_pos);

        // Spring 3 displacement
        __m128 const s3pa_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 3].PointAIndex)));
        __m128 const s3pb_pos = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 3].PointBIndex)));
        // pos[s3.B].x - pos[s3.A].x, pos[s3.B].y - pos[s3.A].y, *, *
        __m128 const s3_displacement = _mm_sub_ps(s3pb_pos, s3pa_pos);

        // s2_displacement.x, s2_displacement.y, s3_displacement.x, s3_displacement.y
        __m128 const s2s3_displacement = _mm_movelh_ps(s2_displacement, s3_displacement);

        // Shuffle displacements:
        // s0_displacement.x, s1_displacement.x, s2_displacement.x, s3_displacement.x
        __m128 s0s1s2s3_displacement_x = _mm_shuffle_ps(s0s1_displacement, s2s3_displacement, 0x88);
        // s0_displacement.y, s1_displacement.y, s2_displacement.y, s3_displacement.y
        __m128 s0s1s2s3_displacement_y = _mm_shuffle_ps(s0s1_displacement, s2s3_displacement, 0xDD);

        //
        // Calculate spring lengths
        //

        // s0_displacement.x^2, s1_displacement.x^2, s2_displacement.x^2, s3_displacement.x^2
        __m128 const s0s1s2s3_displacement_x2 = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_displacement_x);
        // s0_displacement.y^2, s1_displacement.y^2, s2_displacement.y^2, s3_displacement.y^2
        __m128 const s0s1s2s3_displacement_y2 = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_displacement_y);

        // s0_displacement.x^2 + s0_displacement.y^2, s1_displacement.x^2 + s1_displacement.y^2, s2_displacement..., s3_displacement...
        __m128 const s0s1s2s3_displacement_x2_p_y2 = _mm_add_ps(s0s1s2s3_displacement_x2, s0s1s2s3_displacement_y2);

        __m128 const s0s1s2s3_springLength = _mm_sqrt_ps(s0s1s2s3_displacement_x2_p_y2); // sqrt

        //
        // Calculate spring directions
        //

        __m128 const validMask = _mm_cmpneq_ps(s0s1s2s3_springLength, Zero);

        __m128 s0s1s2s3_dir_x = _mm_div_ps(s0s1s2s3_displacement_x, s0s1s2s3_springLength);
        __m128 s0s1s2s3_dir_y = _mm_div_ps(s0s1s2s3_displacement_y, s0s1s2s3_springLength);
        // L==0 => 1/L == 0, to maintain normalized == (0, 0), as in vec2f
        s0s1s2s3_dir_x = _mm_and_ps(s0s1s2s3_dir_x, validMask);
        s0s1s2s3_dir_y = _mm_and_ps(s0s1s2s3_dir_y, validMask);

        //////////////////////////////////////////////////////////////////////////////////////////////

        //
        // 1. Hooke's law
        //

        // Calculate springs' forces' moduli - for endpoints A:
        //    (displacementLength[s] - restLength[s]) * stiffness[s]

        __m128 const s0s1s2s3_restLength = _mm_load_ps(restLengthBuffer + s);
        __m128 const s0s1s2s3_stiffness = _mm_load_ps(stiffnessCoefficientBuffer + s);

        __m128 const s0s1s2s3_hooke_forceModuli = _mm_mul_ps(
            _mm_sub_ps(s0s1s2s3_springLength, s0s1s2s3_restLength),
            s0s1s2s3_stiffness);

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring
        //

        // TODOHERE
    }

    // One-by-one
    for (; s < springCount; ++s)
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
        // Apply forces
        //

        vec2f const forceA = springDir * (fSpring + fDamp);
        pointSpringForceBuffer[pointAIndex] += forceA;
        pointSpringForceBuffer[pointBIndex] -= forceA;
    }
}

void FSBySpringIntrinsics::IntegrateAndResetSpringForces(
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

        // Zero out spring force now that we've integrated it
        springForceBuffer[i] = vec2f::zero();
    }
}