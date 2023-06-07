/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include <Eigen/Dense>
#include <Eigen/Sparse>

#include <memory>
#include <string>

/*
 * Implementation of "Fast simulation of mass-spring systems", from:
 * Liu, T., Bargteil, A. W., Obrien, J. F., & Kavan, L. (2013). Fast simulation of mass-spring systems. ACM Transactions on Graphics,32(6), 1-7. doi:10.1145/2508363.2508406
 *
 * Adapted from https://github.com/sam007961/FastMassSpring.
 */
class FastMSSBasicSimulator final : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "Fast MSS - Basic";
    }

public:

    FastMSSBasicSimulator(
        Object const & object,
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager);

    //////////////////////////////////////////////////////////
    // ISimulator
    //////////////////////////////////////////////////////////

    void OnStateChanged(
        Object const & object,
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager) override;

    void Update(
        Object & object,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters,
        ThreadManager & threadManager) override;

private:

    void CreateState(
        Object const & object,
        SimulationParameters const & simulationParameters);

    // Returns new spring directions
    Eigen::VectorXf RunLocalStep(
        Eigen::Map<Eigen::VectorXf> const & currentState,
        Springs const & springs);

    // Return new positions (state)
    Eigen::VectorXf RunGlobalStep(
        Eigen::VectorXf const & inertialTerm,
        Eigen::VectorXf const & springDirections,
        Eigen::VectorXf const & externalForces,
        SimulationParameters const & simulationParameters);

private:

    // External forces
    Eigen::VectorXf mExternalForces;

    // L, J, M matrices
    Eigen::SparseMatrix<float> mL;
    Eigen::SparseMatrix<float> mJ;
    Eigen::SparseMatrix<float> mM;

    // System matrix (points coeff in system)
    Eigen::SimplicialLLT<Eigen::SparseMatrix<float>> mCholenskySystemMatrix;
};