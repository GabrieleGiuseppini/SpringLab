/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FSBySpringStructuralIntrinsicsSimulator.h"

#include "Log.h"
#include "SysSpecifics.h"

#include <array>
#include <cassert>

/*
 * This simulator divides the whole set of springs into two disjoint subsets:
 * 
 * - The first subset is comprised of a sequence of 4 springs all sharing the same
 *   four endpoints; the spring relaxation algorithm for this subset can then be implemented
 *   quite efficiently by leveraging the property that we only need to load 4 points for 4 springs,
 *   instead of 8;
 * - The second subset is comprised of all leftover springs; the spring relaxation algorithm 
     for this subset is the trivial algorithm which requires 2 point loads for each spring.
 */

FSBySpringStructuralIntrinsicsSimulator::FSBySpringStructuralIntrinsicsSimulator(
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

    assert(object.GetSimulatorSpecificStructure().SpringProcessingBlockSizes.size() == 1);
    mSpringPerfectSquareCount = object.GetSimulatorSpecificStructure().SpringProcessingBlockSizes[0];
}

void FSBySpringStructuralIntrinsicsSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    CreateState(object, simulationParameters);
}

void FSBySpringStructuralIntrinsicsSimulator::Update(
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

void FSBySpringStructuralIntrinsicsSimulator::CreateState(
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

void FSBySpringStructuralIntrinsicsSimulator::ApplySpringsForces(Object const & object)
{
    // This implementation is for 4-float SSE
#if !FS_IS_ARCHITECTURE_X86_32() && !FS_IS_ARCHITECTURE_X86_64()
#error Unsupported Architecture
#endif    
    static_assert(vectorization_float_count<int> == 4);

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

    aligned_to_vword vec2f tmpSpringForces[4];

    //
    // 1. Perfect squares
    //
    
    for (; s < mSpringPerfectSquareCount; s += 4)
    {
        // XMM register notation:
        //   low (left, or top) -> height (right, or bottom)

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
        // Steps:
        // 
        // l_pos_x   -   j_pos_x   =  s0_dis_x
        // l_pos_y   -   j_pos_y   =  s0_dis_y
        // k_pos_x   -   m_pos_x   =  s1_dis_x
        // k_pos_y   -   m_pos_y   =  s1_dis_y
        // 
        // Swap 2H with 2L in first register, then:
        // 
        // k_pos_x   -   j_pos_x   =  s2_dis_x
        // k_pos_y   -   j_pos_y   =  s2_dis_y
        // l_pos_x   -   m_pos_x   =  s3_dis_x
        // l_pos_y   -   m_pos_y   =  s3_dis_y
        // 

        ElementIndex const pointJIndex = endpointsBuffer[s + 0].PointAIndex;
        ElementIndex const pointKIndex = endpointsBuffer[s + 1].PointBIndex;
        ElementIndex const pointLIndex = endpointsBuffer[s + 0].PointBIndex;
        ElementIndex const pointMIndex = endpointsBuffer[s + 1].PointAIndex;

        assert(pointJIndex == endpointsBuffer[s + 2].PointAIndex);
        assert(pointKIndex == endpointsBuffer[s + 2].PointBIndex);
        assert(pointLIndex == endpointsBuffer[s + 3].PointBIndex);
        assert(pointMIndex == endpointsBuffer[s + 3].PointAIndex);

        // ?_pos_x
        // ?_pos_y
        // *
        // *
        __m128 const j_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + pointJIndex)));
        __m128 const k_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + pointKIndex)));
        __m128 const l_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + pointLIndex)));
        __m128 const m_pos_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointPositionBuffer + pointMIndex)));
        
        __m128 const jm_pos_xy = _mm_movelh_ps(j_pos_xy, m_pos_xy); // First argument goes low
        __m128 lk_pos_xy = _mm_movelh_ps(l_pos_xy, k_pos_xy); // First argument goes low
        __m128 const s0s1_dis_xy = _mm_sub_ps(lk_pos_xy, jm_pos_xy);
        lk_pos_xy = _mm_shuffle_ps(lk_pos_xy, lk_pos_xy, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 const s2s3_dis_xy = _mm_sub_ps(lk_pos_xy, jm_pos_xy);

        // Shuffle:
        //
        // s0_dis_x     s0_dis_y
        // s1_dis_x     s1_dis_y
        // s2_dis_x     s2_dis_y
        // s3_dis_x     s3_dis_y
        __m128 s0s1s2s3_dis_x = _mm_shuffle_ps(s0s1_dis_xy, s2s3_dis_xy, 0x88);
        __m128 s0s1s2s3_dis_y = _mm_shuffle_ps(s0s1_dis_xy, s2s3_dis_xy, 0xDD);

        // Calculate spring lengths
        __m128 const s0s1s2s3_springLength = 
            _mm_sqrt_ps(
                _mm_add_ps(
                    _mm_mul_ps(s0s1s2s3_dis_x, s0s1s2s3_dis_x),
                    _mm_mul_ps(s0s1s2s3_dis_y, s0s1s2s3_dis_y)));

        // Calculate spring directions
        __m128 const validMask = _mm_cmpneq_ps(s0s1s2s3_springLength, Zero); // SL==0 => 1/SL==0, to maintain "normalized == (0, 0)", as in vec2f        
        __m128 const s0s1s2s3_sdir_x = 
            _mm_and_ps(
                _mm_div_ps(
                    s0s1s2s3_dis_x, 
                    s0s1s2s3_springLength),
            validMask);
        __m128 const s0s1s2s3_sdir_y = 
            _mm_and_ps(
                _mm_div_ps(
                    s0s1s2s3_dis_y, 
                    s0s1s2s3_springLength),
            validMask);

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

        __m128 const s0s1s2s3_hooke_forceModuli = 
            _mm_mul_ps(
                _mm_sub_ps(
                    s0s1s2s3_springLength, 
                    _mm_load_ps(restLengthBuffer + s)),
                _mm_load_ps(stiffnessCoefficientBuffer + s));

        //
        // 2. Damper forces
        //
        // Damp the velocities of each endpoint pair, as if the points were also connected by a damper
        // along the same direction as the spring, for endpoint A:
        //      relVelocity.dot(springDir) * dampingCoeff[s]
        //
        // Strategy: 
        // 
        // (s0_relv_x * s0_sdir_x  +  s0_relv_y * s0_sdir_y) * dampCoeff[s0]
        // (s1_relv_x * s1_sdir_x  +  s1_relv_y * s1_sdir_y) * dampCoeff[s1]
        // (s2_relv_x * s2_sdir_x  +  s2_relv_y * s2_sdir_y) * dampCoeff[s2]
        // (s3_relv_x * s3_sdir_x  +  s3_relv_y * s3_sdir_y) * dampCoeff[s3]
        //

        // ?_vel_x
        // ?_vel_y
        // *
        // *
        __m128 const j_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + pointJIndex)));
        __m128 const k_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + pointKIndex)));
        __m128 const l_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + pointLIndex)));
        __m128 const m_vel_xy = _mm_castpd_ps(_mm_load_sd(reinterpret_cast<double const * restrict>(pointVelocityBuffer + pointMIndex)));

        __m128 const jm_vel_xy = _mm_movelh_ps(j_vel_xy, m_vel_xy); // First argument goes low
        __m128 lk_vel_xy = _mm_movelh_ps(l_vel_xy, k_vel_xy); // First argument goes low
        __m128 const s0s1_rvel_xy = _mm_sub_ps(lk_vel_xy, jm_vel_xy);
        lk_vel_xy = _mm_shuffle_ps(lk_vel_xy, lk_vel_xy, _MM_SHUFFLE(1, 0, 3, 2));
        __m128 const s2s3_rvel_xy = _mm_sub_ps(lk_vel_xy, jm_vel_xy);

        __m128 s0s1s2s3_rvel_x = _mm_shuffle_ps(s0s1_rvel_xy, s2s3_rvel_xy, 0x88);
        __m128 s0s1s2s3_rvel_y = _mm_shuffle_ps(s0s1_rvel_xy, s2s3_rvel_xy, 0xDD);

        __m128 const s0s1s2s3_damping_forceModuli =
            _mm_mul_ps(                
                _mm_add_ps( // Dot product
                    _mm_mul_ps(s0s1s2s3_rvel_x, s0s1s2s3_sdir_x),
                    _mm_mul_ps(s0s1s2s3_rvel_y, s0s1s2s3_sdir_y)),
                _mm_load_ps(dampingCoefficientBuffer + s));

        //
        // 3. Apply forces: 
        //      force A = springDir * (hookeForce + dampingForce)
        //      force B = - forceA
        //
        // Strategy:
        //
        //  s0_tforce_a_x  =   s0_sdir_x  *  (  hookeForce[s0] + dampingForce[s0] ) 
        //  s1_tforce_a_x  =   s1_sdir_x  *  (  hookeForce[s1] + dampingForce[s1] )
        //  s2_tforce_a_x  =   s2_sdir_x  *  (  hookeForce[s2] + dampingForce[s2] )
        //  s3_tforce_a_x  =   s3_sdir_x  *  (  hookeForce[s3] + dampingForce[s3] )
        //
        //  s0_tforce_a_y  =   s0_sdir_y  *  (  hookeForce[s0] + dampingForce[s0] ) 
        //  s1_tforce_a_y  =   s1_sdir_y  *  (  hookeForce[s1] + dampingForce[s1] )
        //  s2_tforce_a_y  =   s2_sdir_y  *  (  hookeForce[s2] + dampingForce[s2] )
        //  s3_tforce_a_y  =   s3_sdir_y  *  (  hookeForce[s3] + dampingForce[s3] )
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
        // s0_a_sforce += s0_a_tforce + s2_a_tforce
        // s1_a_sforce += s1_a_tforce + s3_a_tforce
        // s0_b_sforce += -s0_a_tforce - s3_a_tforce
        // s1_b_sforce += -s1_a_tforce - s2_a_tforce

        __m128 s0s1_tforceA_xy = _mm_unpacklo_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[0], b[0], a[1], b[1]
        __m128 s2s3_tforceA_xy = _mm_unpackhi_ps(s0s1s2s3_tforceA_x, s0s1s2s3_tforceA_y); // a[2], b[2], a[3], b[3]

        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[0])), s0s1_tforceA_xy);
        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[2])), s2s3_tforceA_xy);

        pointSpringForceBuffer[pointJIndex] += tmpSpringForces[0] + tmpSpringForces[2];        
        pointSpringForceBuffer[pointKIndex] -= tmpSpringForces[1] + tmpSpringForces[2];
        pointSpringForceBuffer[pointLIndex] -= tmpSpringForces[0] + tmpSpringForces[3];
        pointSpringForceBuffer[pointMIndex] += tmpSpringForces[1] + tmpSpringForces[3];
    }

    //
    // 2. Remaining four-by-four's
    //

    for (; s < springVectorizedCount; s += 4)
    {
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

        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[0])), s0s1_tforceA_xy);
        _mm_store_ps(reinterpret_cast<float *>(&(tmpSpringForces[2])), s2s3_tforceA_xy);

        pointSpringForceBuffer[endpointsBuffer[s + 0].PointAIndex] += tmpSpringForces[0];
        pointSpringForceBuffer[endpointsBuffer[s + 0].PointBIndex] -= tmpSpringForces[0];
        pointSpringForceBuffer[endpointsBuffer[s + 1].PointAIndex] += tmpSpringForces[1];
        pointSpringForceBuffer[endpointsBuffer[s + 1].PointBIndex] -= tmpSpringForces[1];
        pointSpringForceBuffer[endpointsBuffer[s + 2].PointAIndex] += tmpSpringForces[2];
        pointSpringForceBuffer[endpointsBuffer[s + 2].PointBIndex] -= tmpSpringForces[2];
        pointSpringForceBuffer[endpointsBuffer[s + 3].PointAIndex] += tmpSpringForces[3];
        pointSpringForceBuffer[endpointsBuffer[s + 3].PointBIndex] -= tmpSpringForces[3];
    }

    //
    // 3. One-by-one
    //

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

void FSBySpringStructuralIntrinsicsSimulator::IntegrateAndResetSpringForces(
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

/////////////////////////////////////////////////

ILayoutOptimizer::LayoutRemap FSBySpringStructuralIntrinsicsLayoutOptimizer::Remap(
    ObjectBuildPointIndexMatrix const & pointMatrix,
    std::vector<ObjectBuildPoint> const & points,
    std::vector<ObjectBuildSpring> const & springs) const
{
    IndexRemap optimalPointRemap(points.size());
    IndexRemap optimalSpringRemap(springs.size());

    std::vector<bool> remappedPointMask(points.size(), false);
    std::vector<bool> remappedSpringMask(springs.size(), false);
    std::vector<bool> springFlipMask(springs.size(), false);

    // Build Point Pair -> Old Spring Index table
    PointPairToIndexMap pointPairToOldSpringIndexMap;
    for (ElementIndex s = 0; s < springs.size(); ++s)
    {
        pointPairToOldSpringIndexMap.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(springs[s].PointAIndex, springs[s].PointBIndex),
            std::forward_as_tuple(s));
    }

    //
    // 1. Find all "complete squares" from left-bottom
    //
    // A complete square looks like:
    // 
    //  If A is "even":
    // 
    //  D  C
    //  |\/|
    //  |/\|
    //  A  B
    // 
    // Else (A is "odd"):
    // 
    //  D--C
    //   \/
    //   /\
    //  A--B
    // 
    // For each perfect square, we re-order springs and their endpoints of each spring so that:
    //  - The first two springs of the perfect square are the cross springs
    //  - The endpoints A's of the cross springs are to be connected, and likewise 
    //    the endpoint B's
    //

    ElementCount perfectSquareCount = 0;

    std::array<PointPair, 4> perfectSquareSpringEndpoints;

    for (int y = 0; y < pointMatrix.height; ++y)
    {
        for (int x = 0; x < pointMatrix.width; ++x)
        {
            // Check if this is vertex A of a square
            if (pointMatrix[{x, y}]
                && x < pointMatrix.width - 1 && pointMatrix[{x + 1, y}]
                && y < pointMatrix.height - 1 && pointMatrix[{x + 1, y + 1}]
                && pointMatrix[{x, y + 1}])
            {
                ElementIndex const a = *pointMatrix[{x, y}];
                ElementIndex const b = *pointMatrix[{x + 1, y}];
                ElementIndex const c = *pointMatrix[{x + 1, y + 1}];
                ElementIndex const d = *pointMatrix[{x, y + 1}];

                // Check existence - and availability - of all springs now

                ElementIndex crossSpringACIndex;
                if (auto const springIt = pointPairToOldSpringIndexMap.find({ a, c });
                    springIt != pointPairToOldSpringIndexMap.cend() && !remappedSpringMask[springIt->second])
                {
                    crossSpringACIndex = springIt->second;
                }
                else
                {
                    continue;
                }

                ElementIndex crossSpringBDIndex;
                if (auto const springIt = pointPairToOldSpringIndexMap.find({ b, d });
                    springIt != pointPairToOldSpringIndexMap.cend() && !remappedSpringMask[springIt->second])
                {
                    crossSpringBDIndex = springIt->second;
                }
                else
                {
                    continue;
                }

                if ((x + y) % 2 == 0)
                {
                    // Even: check AD, BC

                    ElementIndex sideSpringADIndex;
                    if (auto const springIt = pointPairToOldSpringIndexMap.find({ a, d });
                        springIt != pointPairToOldSpringIndexMap.cend() && !remappedSpringMask[springIt->second])
                    {
                        sideSpringADIndex = springIt->second;
                    }
                    else
                    {
                        continue;
                    }

                    ElementIndex sideSpringBCIndex;
                    if (auto const springIt = pointPairToOldSpringIndexMap.find({ b, c });
                        springIt != pointPairToOldSpringIndexMap.cend() && !remappedSpringMask[springIt->second])
                    {
                        sideSpringBCIndex = springIt->second;
                    }
                    else
                    {
                        continue;
                    }

                    // It'a a perfect square

                    // Re-order springs and make sure they have the right directions:
                    //  A->C
                    //  B->D
                    //  A->D
                    //  B->C

                    optimalSpringRemap.AddOld(crossSpringACIndex);
                    remappedSpringMask[crossSpringACIndex] = true;
                    if (springs[crossSpringACIndex].PointBIndex != c)
                    {
                        assert(springs[crossSpringACIndex].PointBIndex == a);
                        springFlipMask[crossSpringACIndex] = true;
                    }

                    optimalSpringRemap.AddOld(crossSpringBDIndex);
                    remappedSpringMask[crossSpringBDIndex] = true;
                    if (springs[crossSpringBDIndex].PointBIndex != d)
                    {
                        assert(springs[crossSpringBDIndex].PointBIndex == b);
                        springFlipMask[crossSpringBDIndex] = true;
                    }

                    optimalSpringRemap.AddOld(sideSpringADIndex);
                    remappedSpringMask[sideSpringADIndex] = true;
                    if (springs[sideSpringADIndex].PointBIndex != d)
                    {
                        assert(springs[sideSpringADIndex].PointBIndex == a);
                        springFlipMask[sideSpringADIndex] = true;
                    }

                    optimalSpringRemap.AddOld(sideSpringBCIndex);
                    remappedSpringMask[sideSpringBCIndex] = true;
                    if (springs[sideSpringBCIndex].PointBIndex != c)
                    {
                        assert(springs[sideSpringBCIndex].PointBIndex == b);
                        springFlipMask[sideSpringBCIndex] = true;
                    }
                }
                else
                {
                    // Odd: check AB, CD

                    ElementIndex sideSpringABIndex;
                    if (auto const springIt = pointPairToOldSpringIndexMap.find({ a, b });
                        springIt != pointPairToOldSpringIndexMap.cend() && !remappedSpringMask[springIt->second])
                    {
                        sideSpringABIndex = springIt->second;
                    }
                    else
                    {
                        continue;
                    }

                    ElementIndex sideSpringCDIndex;
                    if (auto const springIt = pointPairToOldSpringIndexMap.find({ c, d });
                        springIt != pointPairToOldSpringIndexMap.cend() && !remappedSpringMask[springIt->second])
                    {
                        sideSpringCDIndex = springIt->second;
                    }
                    else
                    {
                        continue;
                    }

                    // It'a a perfect square

                    // Re-order springs abd make sure they have the right directions:
                    //  A->C
                    //  D->B
                    //  A->B
                    //  D->C

                    optimalSpringRemap.AddOld(crossSpringACIndex);
                    remappedSpringMask[crossSpringACIndex] = true;
                    if (springs[crossSpringACIndex].PointBIndex != c)
                    {
                        assert(springs[crossSpringACIndex].PointBIndex == a);
                        springFlipMask[crossSpringACIndex] = true;
                    }

                    optimalSpringRemap.AddOld(crossSpringBDIndex);
                    remappedSpringMask[crossSpringBDIndex] = true;
                    if (springs[crossSpringBDIndex].PointBIndex != b)
                    {
                        assert(springs[crossSpringBDIndex].PointBIndex == d);
                        springFlipMask[crossSpringBDIndex] = true;
                    }

                    optimalSpringRemap.AddOld(sideSpringABIndex);
                    remappedSpringMask[sideSpringABIndex] = true;
                    if (springs[sideSpringABIndex].PointBIndex != b)
                    {
                        assert(springs[sideSpringABIndex].PointBIndex == a);
                        springFlipMask[sideSpringABIndex] = true;
                    }

                    optimalSpringRemap.AddOld(sideSpringCDIndex);
                    remappedSpringMask[sideSpringCDIndex] = true;
                    if (springs[sideSpringCDIndex].PointBIndex != c)
                    {
                        assert(springs[sideSpringCDIndex].PointBIndex == d);
                        springFlipMask[sideSpringCDIndex] = true;
                    }
                }

                // If we're here, this was a perfect square

                // Remap points

                if (!remappedPointMask[a])
                {
                    optimalPointRemap.AddOld(a);
                    remappedPointMask[a] = true;
                }

                if (!remappedPointMask[b])
                {
                    optimalPointRemap.AddOld(b);
                    remappedPointMask[b] = true;
                }

                if (!remappedPointMask[c])
                {
                    optimalPointRemap.AddOld(c);
                    remappedPointMask[c] = true;
                }

                if (!remappedPointMask[d])
                {
                    optimalPointRemap.AddOld(d);
                    remappedPointMask[d] = true;
                }

                ++perfectSquareCount;
            }
        }
    }

    ObjectSimulatorSpecificStructure simulatorSpecificStructure;
    simulatorSpecificStructure.SpringProcessingBlockSizes.emplace_back(perfectSquareCount);

    //
    // Map leftovers now
    //

    LogMessage("LayoutOptimizer: ", std::count(remappedPointMask.cbegin(), remappedPointMask.cend(), false), " leftover points, ",
        std::count(remappedSpringMask.cbegin(), remappedSpringMask.cend(), false), " leftover springs");

    for (ElementIndex p = 0; p < points.size(); ++p)
    {
        if (!remappedPointMask[p])
        {
            optimalPointRemap.AddOld(p);
        }
    }

    for (ElementIndex s = 0; s < springs.size(); ++s)
    {
        if (!remappedSpringMask[s])
        {
            optimalSpringRemap.AddOld(s);
        }
    }

    return LayoutRemap(
        std::move(optimalPointRemap),
        std::move(optimalSpringRemap),
        std::move(springFlipMask),
        std::move(simulatorSpecificStructure));
}