/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "FastMSSBasicSimulator.h"

#include <cassert>
#include <vector>

FastMSSBasicSimulator::FastMSSBasicSimulator(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & /*threadManager*/)
{
    CreateState(object, simulationParameters);
}

void FastMSSBasicSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters,
    ThreadManager const & /*threadManager*/)
{
    CreateState(object, simulationParameters);
}

void FastMSSBasicSimulator::Update(
    Object & object,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters,
    ThreadManager & /*threadManager*/)
{
    float const dt = simulationParameters.Common.SimulationTimeStepDuration;

    auto const nParticles = object.GetPoints().GetElementCount();

    //
    // The object's state is our last-produced next state, *plus* any user-applied state modifications;
    // so, we take it as our current state
    //

    auto currentState = Eigen::Map<Eigen::VectorXf>(
        reinterpret_cast<float *>(object.GetPoints().GetPositionBuffer()),
        nParticles * 2);

    //
    // Calculate inertial term:
    //
    //  M * (2q(n) − q(n−1))
    //
    // Where we allow damping to skew the equation.
    //
    // Realizing that the inertial term is simply the current positions plus
    // the current velocities (i.e. (q(n) - q(n-1))/dt), we rewrite is as follows:
    //
    //  M * q(n) + d * v(n) * dt
    //

    auto currentVelocities = Eigen::Map<Eigen::VectorXf>(
        reinterpret_cast<float *>(object.GetPoints().GetVelocityBuffer()),
        nParticles * 2);

    float const damping = simulationParameters.FastMSSCommonSimulator.GlobalDamping;

    Eigen::VectorXf const inertialTerm = mM * (currentState + currentVelocities * damping * dt);

    //
    // Optimize, alternating between local and global steps
    //

    // Save current state as initial state
    Eigen::VectorXf const initialState = currentState;

    for (size_t i = 0; i < simulationParameters.FastMSSCommonSimulator.NumLocalGlobalStepIterations; ++i)
    {
        // Calculate spring directions based on current state
        Eigen::VectorXf const springDirections = RunLocalStep(
            currentState,
            object.GetSprings());

        // Calculate new current state (updating points' position buffer)
        currentState = RunGlobalStep(
            inertialTerm,
            springDirections,
            mExternalForces,
            simulationParameters); 
    }

    //
    // Fix points
    //

    for (auto const p : object.GetPoints())
    {
        currentState[2 * p] =
            object.GetPoints().GetFrozenCoefficient(p) * currentState[2 * p]
            + (1.0f - object.GetPoints().GetFrozenCoefficient(p)) * initialState[2 * p];
        currentState[2 * p + 1] =
            object.GetPoints().GetFrozenCoefficient(p) * currentState[2 * p + 1]
            + (1.0f - object.GetPoints().GetFrozenCoefficient(p)) * initialState[2 * p + 1];
    }

    //
    // Calculate velocities
    //

    currentVelocities = (currentState - initialState) / dt;
}

///////////////////////////////////////////////////////////////////////////////////////////

void FastMSSBasicSimulator::CreateState(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    std::vector<Eigen::Triplet<float>> triplets;
    
    float const dtSquared = simulationParameters.Common.SimulationTimeStepDuration * simulationParameters.Common.SimulationTimeStepDuration;
    auto const nParticles = object.GetPoints().GetElementCount();
    auto const nSprings = object.GetSprings().GetElementCount();

    //
    // Compute L
    //
    //        pa.x  pa.y  ...  pb.x  pb.y
    //  pa.x   +k               -k
    //  pa.y         +k               -k
    //  ....
    //  pb.x   -k               +k
    //  pb.y         -k               +k
    //

    mL.resize(2 * nParticles, 2 * nParticles);

    triplets.clear();
    for (auto const s : object.GetSprings())
    {
        float const k =
            simulationParameters.FastMSSCommonSimulator.SpringStiffnessCoefficient
            * object.GetSprings().GetMaterialStiffness(s);

        auto const pa = object.GetSprings().GetEndpointAIndex(s);
        auto const pb = object.GetSprings().GetEndpointBIndex(s);

        // x
        triplets.emplace_back(2 * pa, 2 * pa, 1.0f * k);
        triplets.emplace_back(2 * pa, 2 * pb, -1.0f * k);
        triplets.emplace_back(2 * pb, 2 * pa, -1.0f * k);
        triplets.emplace_back(2 * pb, 2 * pb, 1.0f * k);

        // y
        triplets.emplace_back(2 * pa + 1, 2 * pa + 1, 1.0f * k);
        triplets.emplace_back(2 * pa + 1, 2 * pb + 1, -1.0f * k);
        triplets.emplace_back(2 * pb + 1, 2 * pa + 1, -1.0f * k);
        triplets.emplace_back(2 * pb + 1, 2 * pb + 1, 1.0f * k);

    }

    mL.setFromTriplets(triplets.cbegin(), triplets.cend());

    //
    // Compute J
    //
    //        s(1)  s(2)
    //  pa.x   +k
    //  pa.y         +k
    //  ....
    //  pb.x   -k
    //  pb.y         -k
    //

    mJ.resize(2 * nParticles, 2 * nSprings);

    triplets.clear();
    for (auto const s : object.GetSprings())
    {
        float const k =
            object.GetSprings().GetMaterialStiffness(s)
            * simulationParameters.FastMSSCommonSimulator.SpringStiffnessCoefficient;
        
        auto const pa = object.GetSprings().GetEndpointAIndex(s);
        auto const pb = object.GetSprings().GetEndpointBIndex(s);

        // x
        triplets.emplace_back(2 * pa, 2 * s, 1.0f * k);
        triplets.emplace_back(2 * pb, 2 * s, -1.0f * k);

        // y
        triplets.emplace_back(2 * pa + 1, 2 * s + 1, 1.0f * k);
        triplets.emplace_back(2 * pb + 1, 2 * s + 1, -1.0f * k);
    }

    mJ.setFromTriplets(triplets.cbegin(), triplets.cend());

    //
    // Compute M
    //
    //        p.x  p.y
    //  p.x    m
    //  p.y         m
    //

    mM.resize(2 * nParticles, 2 * nParticles);

    triplets.clear();
    for (auto const p : object.GetPoints())
    {
        float const m =
            object.GetPoints().GetMass(p)
            * simulationParameters.Common.MassAdjustment;

        // x
        triplets.emplace_back(2 * p, 2 * p, m);

        // y
        triplets.emplace_back(2 * p + 1, 2 * p + 1, m);
    }

    mM.setFromTriplets(triplets.cbegin(), triplets.cend());

    //
    // Pre-factor system matrix:
    //
    //  M + h^2L
    //

    Eigen::SparseMatrix<float> const A = mM + dtSquared * mL;
    mCholenskySystemMatrix.compute(A);

    //
    // Calculate external forces
    //

    mExternalForces = Eigen::VectorXf(nParticles * 2);

    for (auto const p : object.GetPoints())
    {
        vec2f const totalForce =
            // Gravity
            simulationParameters.Common.AssignedGravity * object.GetPoints().GetMass(p) * simulationParameters.Common.MassAdjustment
            // External forces
            + object.GetPoints().GetAssignedForce(p);

        mExternalForces(2 * p) = totalForce.x;
        mExternalForces(2 * p + 1) = totalForce.y;
    }
}

Eigen::VectorXf FastMSSBasicSimulator::RunLocalStep(
    Eigen::Map<Eigen::VectorXf> const & currentState,
    Springs const & springs)
{
    //
    // Calculate optimal spring directions based on current state (fixing positions)
    //

    Eigen::VectorXf springDirections(springs.GetElementCount() * 2);

    for (auto const s : springs)
    {
        auto const pa = springs.GetEndpointAIndex(s);
        auto const pb = springs.GetEndpointBIndex(s);

        vec2f const xa = vec2f(currentState[2 * pa], currentState[2 * pa + 1]);
        vec2f const xb = vec2f(currentState[2 * pb], currentState[2 * pb + 1]);

        vec2f const dir =
            (xa - xb).normalise()
            * springs.GetRestLength(s);

        springDirections[2 * s] = dir.x;
        springDirections[2 * s + 1] = dir.y;
    }

    return springDirections;
}

Eigen::VectorXf FastMSSBasicSimulator::RunGlobalStep(
    Eigen::VectorXf const & inertialTerm,
    Eigen::VectorXf const & springDirections,
    Eigen::VectorXf const & externalForces,
    SimulationParameters const & simulationParameters)
{
    //
    // Produce new current state by computing optimal positions (fixing directions)
    //

    float const dtSquared = simulationParameters.Common.SimulationTimeStepDuration * simulationParameters.Common.SimulationTimeStepDuration;

    //
    // Compute b vector (external forces and inertia)
    //

    Eigen::VectorXf const b = 
        inertialTerm
        + dtSquared * mJ * springDirections
        + dtSquared * externalForces;

    //
    // Solve system and return new state
    //

    return mCholenskySystemMatrix.solve(b);
}

