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

        vec2f const s0_dir = s0_dis.normalise_approx(s0_len);
        vec2f const s1_dir = s1_dis.normalise_approx(s1_len);
        vec2f const s2_dir = s2_dis.normalise_approx(s2_len);
        vec2f const s3_dir = s3_dis.normalise_approx(s3_len);
        
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
    // 2. Remaining four-by-four's - TODOHERE: should we do directly 1-by-1?
    //

    ElementCount const endSpringIndexVectorized = endSpringIndex - (endSpringIndex % 4);

    for (; s < endSpringIndexVectorized; s += 4)
    {
        // TODO
        aligned_to_vword vec2f tmpSpringForces[4];

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

        __m128 const Zero = _mm_setzero_ps();
        __m128 const validMask = _mm_cmpneq_ps(s0s1s2s3_displacement_x2_p_y2, Zero);

        __m128 const s0s1s2s3_springLength_inv =
            _mm_and_ps(
                _mm_rsqrt_ps(s0s1s2s3_displacement_x2_p_y2),
                validMask);

        __m128 const s0s1s2s3_springLength =
            _mm_and_ps(
                _mm_rcp_ps(s0s1s2s3_springLength_inv),
                validMask);

        // Calculate spring directions
        __m128 const s0s1s2s3_sdir_x = _mm_mul_ps(s0s1s2s3_displacement_x, s0s1s2s3_springLength_inv);
        __m128 const s0s1s2s3_sdir_y = _mm_mul_ps(s0s1s2s3_displacement_y, s0s1s2s3_springLength_inv);

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

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_1(
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

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_2(
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

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_4(
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

void FSBySpringStructuralPseudoIntrinsicsMTVectorizedSimulator::IntegrateAndResetSpringForces_N(
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

            _mm_store_ps(pointSprigForceBufferOfBuffers[b] + p, zero_4);
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
    }
}