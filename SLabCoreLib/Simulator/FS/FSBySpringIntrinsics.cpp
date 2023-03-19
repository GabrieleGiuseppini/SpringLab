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

    aligned_to_vword vec2f springForces[4];

    // Word-by-word
    for (; s < springVectorizedCount; s += 4)
    {
        // Notation:
        //   low (left) -> height (right)
        
        //
        // Calculate displacement, string lengths, and spring directions
        // 
        // Strategy:
        //
        // springDir[s0].x  =  displacement[s0].x  /  springLength[s0]
        // springDir[s1].x  =  displacement[s1].x  /  springLength[s1]
        // springDir[s2].x  =  displacement[s2].x  /  springLength[s2]
        // springDir[s3].x  =  displacement[s3].x  /  springLength[s3]
        //
        // springDir[s0].y  =  displacement[s0].y  /  springLength[s0]
        // springDir[s1].y  =  displacement[s1].y  /  springLength[s1]
        // springDir[s2].y  =  displacement[s2].y  /  springLength[s2]
        // springDir[s3].y  =  displacement[s3].y  /  springLength[s3]
        //

        // Spring 0 displacement (s0_position.x, s0_position.y, *, *)
        __m128 const s0pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 0].PointAIndex)));
        __m128 const s0pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 0].PointBIndex)));
        // s0_displacement.x, s0_displacement.y, *, *
        __m128 const s0_displacement_xy = _mm_sub_ps(s0pb_pos_xy, s0pa_pos_xy);

        // Spring 1 displacement (s1_position.x, s1_position.y, *, *)
        __m128 const s1pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 1].PointAIndex)));
        __m128 const s1pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 1].PointBIndex)));
        // s1_displacement.x, s1_displacement.y
        __m128 const s1_displacement_xy = _mm_sub_ps(s1pb_pos_xy, s1pa_pos_xy);

        // s0_displacement.x, s0_displacement.y, s1_displacement.x, s1_displacement.y
        __m128 const s0s1_displacement_xy = _mm_movelh_ps(s0_displacement_xy, s1_displacement_xy); // First argument goes low

        // Spring 2 displacement (s2_position.x, s2_position.y, *, *)
        __m128 const s2pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 2].PointAIndex)));
        __m128 const s2pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 2].PointBIndex)));
        // s2_displacement.x, s2_displacement.y
        __m128 const s2_displacement_xy = _mm_sub_ps(s2pb_pos_xy, s2pa_pos_xy);

        // Spring 3 displacement (s3_position.x, s3_position.y, *, *)
        __m128 const s3pa_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 3].PointAIndex)));
        __m128 const s3pb_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + endpointsBuffer[s + 3].PointBIndex)));
        // s3_displacement.x, s3_displacement.y
        __m128 const s3_displacement_xy = _mm_sub_ps(s3pb_pos_xy, s3pa_pos_xy);

        // s2_displacement.x, s2_displacement.y, s3_displacement.x, s3_displacement.y
        __m128 const s2s3_displacement_xy = _mm_movelh_ps(s2_displacement_xy, s3_displacement_xy); // First argument goes low

        // Shuffle displacements:
        // s0_displacement.x, s1_displacement.x, s2_displacement.x, s3_displacement.x
        __m128 s0s1s2s3_displacement_x = _mm_shuffle_ps(s0s1_displacement_xy, s2s3_displacement_xy, 0x88);
        // s0_displacement.y, s1_displacement.y, s2_displacement.y, s3_displacement.y
        __m128 s0s1s2s3_displacement_y = _mm_shuffle_ps(s0s1_displacement_xy, s2s3_displacement_xy, 0xDD);

        // Calculate spring lengths

        // s0_displacement.x^2, s1_displacement.x^2, s2_displacement.x^2, s3_displacement.x^2
        __m128 const s0s1s2s3_displacement_x2 = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_displacement_x);
        // s0_displacement.y^2, s1_displacement.y^2, s2_displacement.y^2, s3_displacement.y^2
        __m128 const s0s1s2s3_displacement_y2 = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_displacement_y);

        // s0_displacement.x^2 + s0_displacement.y^2, s1_displacement.x^2 + s1_displacement.y^2, s2_displacement..., s3_displacement...
        __m128 const s0s1s2s3_displacement_x2_p_y2 = _mm_add_ps(s0s1s2s3_displacement_x2, s0s1s2s3_displacement_y2);

        __m128 const s0s1s2s3_springLength = _mm_sqrt_ps(s0s1s2s3_displacement_x2_p_y2); // sqrt

        // Calculate spring directions

        __m128 const validMask = _mm_cmpneq_ps(s0s1s2s3_springLength, Zero);

        __m128 s0s1s2s3_sdir_x = _mm_div_ps(s0s1s2s3_displacement_x, s0s1s2s3_springLength);
        __m128 s0s1s2s3_sdir_y = _mm_div_ps(s0s1s2s3_displacement_y, s0s1s2s3_springLength);
        // L==0 => 1/L == 0, to maintain normalized == (0, 0), as in vec2f
        s0s1s2s3_sdir_x = _mm_and_ps(s0s1s2s3_sdir_x, validMask);
        s0s1s2s3_sdir_y = _mm_and_ps(s0s1s2s3_sdir_y, validMask);

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

        __m128 const s0s1s2s3_restLength = _mm_load_ps(restLengthBuffer + s);
        __m128 const s0s1s2s3_stiffness = _mm_load_ps(stiffnessCoefficientBuffer + s);

        __m128 const s0s1s2s3_hooke_forceModuli = _mm_mul_ps(
            _mm_sub_ps(s0s1s2s3_springLength, s0s1s2s3_restLength),
            s0s1s2s3_stiffness);

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //
        // Strategy: 
        //
        // ( relV[s0].x * sprDir[s0].x  +  relV[s0].y * sprDir[s0].y )  *  dampCoeff[s0]
        // ( relV[s1].x * sprDir[s1].x  +  relV[s1].y * sprDir[s1].y )  *  dampCoeff[s1]
        // ( relV[s2].x * sprDir[s2].x  +  relV[s2].y * sprDir[s2].y )  *  dampCoeff[s2]
        // ( relV[s3].x * sprDir[s3].x  +  relV[s3].y * sprDir[s3].y )  *  dampCoeff[s3]
        //

        // Spring 0 rel vel (s0_vel.x, s0_vel.y, *, *)
        __m128 const s0pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + endpointsBuffer[s + 0].PointAIndex)));
        __m128 const s0pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + endpointsBuffer[s + 0].PointBIndex)));
        // s0_relvel_x, s0_relvel_y, *, *
        __m128 const s0_relvel_xy = _mm_sub_ps(s0pb_vel_xy, s0pa_vel_xy);

        // Spring 1 rel vel (s1_vel.x, s1_vel.y, *, *)
        __m128 const s1pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + endpointsBuffer[s + 1].PointAIndex)));
        __m128 const s1pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + endpointsBuffer[s + 1].PointBIndex)));
        // s1_relvel_x, s1_relvel_y, *, *
        __m128 const s1_relvel_xy = _mm_sub_ps(s1pb_vel_xy, s1pa_vel_xy);

        // s0_relvel.x, s0_relvel.y, s1_relvel.x, s1_relvel.y
        __m128 const s0s1_relvel_xy = _mm_movelh_ps(s0_relvel_xy, s1_relvel_xy); // First argument goes low

        // Spring 2 rel vel (s2_vel.x, s2_vel.y, *, *)
        __m128 const s2pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + endpointsBuffer[s + 2].PointAIndex)));
        __m128 const s2pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + endpointsBuffer[s + 2].PointBIndex)));
        // s2_relvel_x, s2_relvel_y, *, *
        __m128 const s2_relvel_xy = _mm_sub_ps(s2pb_vel_xy, s2pa_vel_xy);

        // Spring 3 rel vel (s3_vel.x, s3_vel.y, *, *)
        __m128 const s3pa_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + endpointsBuffer[s + 3].PointAIndex)));
        __m128 const s3pb_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + endpointsBuffer[s + 3].PointBIndex)));
        // s3_relvel_x, s3_relvel_y, *, *
        __m128 const s3_relvel_xy = _mm_sub_ps(s3pb_vel_xy, s3pa_vel_xy);

        // s2_relvel.x, s2_relvel.y, s3_relvel.x, s3_relvel.y
        __m128 const s2s3_relvel_xy = _mm_movelh_ps(s2_relvel_xy, s3_relvel_xy); // First argument goes low

        // Shuffle rel vals:
        // s0_relvel.x, s1_relvel.x, s2_relvel.x, s3_relvel.x
        __m128 s0s1s2s3_relvel_x = _mm_shuffle_ps(s0s1_relvel_xy, s2s3_relvel_xy, 0x88);
        // s0_relvel.y, s1_relvel.y, s2_relvel.y, s3_relvel.y
        __m128 s0s1s2s3_relvel_y = _mm_shuffle_ps(s0s1_relvel_xy, s2s3_relvel_xy, 0xDD);

        // Damping coeffs
        __m128 const s0s1s2s3_dampingCoeff = _mm_load_ps(dampingCoefficientBuffer + s);

        __m128 const s0s1s2s3_damping_forceModuli =
            _mm_mul_ps(                
                _mm_add_ps( // Dot product
                    _mm_mul_ps(s0s1s2s3_relvel_x, s0s1s2s3_sdir_x),
                    _mm_mul_ps(s0s1s2s3_relvel_y, s0s1s2s3_sdir_y)),
                s0s1s2s3_dampingCoeff);

        //
        // 3. Apply forces: 
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //
        // Strategy:
        //
        //  total_forceA[s0].x  =   springDir[s0].x  *  (  hookeForce[s0] + dampingForce[s0] ) 
        //  total_forceA[s1].x  =   springDir[s1].x  *  (  hookeForce[s1] + dampingForce[s1] )
        //  total_forceA[s2].x  =   springDir[s2].x  *  (  hookeForce[s2] + dampingForce[s2] )
        //  total_forceA[s3].x  =   springDir[s3].x  *  (  hookeForce[s3] + dampingForce[s3] )
        //
        //  total_forceA[s0].y  =   springDir[s0].y  *  (  hookeForce[s0] + dampingForce[s0] ) 
        //  total_forceA[s1].y  =   springDir[s1].y  *  (  hookeForce[s1] + dampingForce[s1] )
        //  total_forceA[s2].y  =   springDir[s2].y  *  (  hookeForce[s2] + dampingForce[s2] )
        //  total_forceA[s3].y  =   springDir[s3].y  *  (  hookeForce[s3] + dampingForce[s3] )
        //

        __m128 const tForceModuli = _mm_add_ps(s0s1s2s3_hooke_forceModuli, s0s1s2s3_damping_forceModuli);

        __m128 const s0s1s2s3_tforceA_x =
            _mm_mul_ps(
                s0s1s2s3_sdir_x,
                tForceModuli);

        __m128 const s0s1s2s3_tforceA_y =
            _mm_mul_ps(
                s0s1s2s3_sdir_y,
                tForceModuli);

        //
        // Unpack and add forces:
        //      pointSpringForceBuffer[pointAIndex] += total_forceA;
        //      pointSpringForceBuffer[pointBIndex] -= total_forceA;
        //

        __m128 s0s1_tforceA_xy = _mm_unpacklo_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[0], b[0], a[1], b[1]
        __m128 s2s3_tforceA_xy = _mm_unpackhi_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[2], b[2], a[3], b[3]

        _mm_store_ps(reinterpret_cast<float *>(&(springForces[0])), s0s1_tforceA_xy);
        _mm_store_ps(reinterpret_cast<float *>(&(springForces[2])), s2s3_tforceA_xy);

        pointSpringForceBuffer[endpointsBuffer[s + 0].PointAIndex] += springForces[0];
        pointSpringForceBuffer[endpointsBuffer[s + 0].PointBIndex] -= springForces[0];
        pointSpringForceBuffer[endpointsBuffer[s + 1].PointAIndex] += springForces[1];
        pointSpringForceBuffer[endpointsBuffer[s + 1].PointBIndex] -= springForces[1];
        pointSpringForceBuffer[endpointsBuffer[s + 2].PointAIndex] += springForces[2];
        pointSpringForceBuffer[endpointsBuffer[s + 2].PointBIndex] -= springForces[2];
        pointSpringForceBuffer[endpointsBuffer[s + 3].PointAIndex] += springForces[3];
        pointSpringForceBuffer[endpointsBuffer[s + 3].PointBIndex] -= springForces[3];
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
        // 3. Apply forces
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