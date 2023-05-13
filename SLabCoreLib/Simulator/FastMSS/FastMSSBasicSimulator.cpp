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
    SimulationParameters const & simulationParameters)
{
    CreateState(object, simulationParameters);
}

void FastMSSBasicSimulator::OnStateChanged(
    Object const & object,
    SimulationParameters const & simulationParameters)
{
    CreateState(object, simulationParameters);
}

void FastMSSBasicSimulator::Update(
    Object & object,
    float /*currentSimulationTime*/,
    SimulationParameters const & simulationParameters)
{
    //
    // The object's state is our last-produced next state, *plus* any user-applied state modifications;
    // so, we take it as our current state (effectively wiping our last production).
    //

    Eigen::Map<Eigen::VectorXf> currentState = Eigen::Map<Eigen::VectorXf>(
        reinterpret_cast<float *>(object.GetPoints().GetPositionBuffer()),
        object.GetPoints().GetElementCount() * 2);

    //
    // Calculate inertial term:
    //
    //  M * (2q(n) − q(n−1))
    //
    // Where we allow damping to skew the equation.
    //

    float const damping = simulationParameters.FastMSSCommonSimulator.GlobalDamping;

    Eigen::VectorXf const inertialTem = mM * ((damping + 1.0f) * currentState - damping * mPreviousState);

    //
    // Shift current state (i.e. now's object state) into previous state
    // (as we're now done with previous state; should technically do this at bottom
    // but that would require to have an additional buffer to calculate next state 
    // instead of calculating in-place in current state)
    //

    mPreviousState = currentState;

    //
    // Optimize, alternating between local and global steps
    //

    for (size_t i = 0; i < simulationParameters.FastMSSCommonSimulator.NumLocalGlobalStepIterations; ++i)
    {
        RunLocalStep(currentState);  // Calculates spring directions based on current state
        currentState = RunGlobalStep(); // Calculates new current state (updating points' position buffer)
        // TODO: double-check that by assigning to currentState we're effectively changing the object's point positions buffer
    }
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
    for (auto s : object.GetSprings())
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
    for (auto s : object.GetSprings())
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
    for (auto p : object.GetPoints())
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
    // Pre-factor system matrix
    //

    Eigen::SparseMatrix<float> const A = mM + dtSquared * mL;
    mCholenskySystemMatrix.compute(A);

    // 
    // Initialize previous state
    //

    mPreviousState = Eigen::PlainObjectBase<Eigen::VectorXf>::Map(
        reinterpret_cast<float const *>(object.GetPoints().GetPositionBuffer()), 
        nParticles * 2);

    // TODO: external forces
}

void FastMSSBasicSimulator::RunLocalStep(Eigen::Map<Eigen::VectorXf> const & currentState)
{
    //
    // Calculate optimal spring directions based on current state (fixing positions)
    //

    // TODO
    (void)currentState;
}

Eigen::VectorXf FastMSSBasicSimulator::RunGlobalStep()
{
    //
    // Produce new current state by computing optimal positions (fixing directions)
    //

    // TODO    
}

