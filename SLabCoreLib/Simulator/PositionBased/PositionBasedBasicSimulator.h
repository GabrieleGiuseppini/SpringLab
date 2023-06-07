/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-24
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include <memory>
#include <string>

/*
 * Basic, naive implementation of a mass-spring-damper system, based on Position-Based Dynamics
 * from Muller (https://matthias-research.github.io/pages/publications/posBasedDyn.pdf).
 */
class PositionBasedBasicSimulator final : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "Position Based - Basic";
    }

public:

    PositionBasedBasicSimulator(
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

    void IntegrateInitialDynamics(
        Object & object,
        SimulationParameters const & simulationParameters);

    void ProjectConstraints(
        Object const & object,
        SimulationParameters const & simulationParameters);

    void FinalizeDynamics(
        Object & object,
        SimulationParameters const & simulationParameters);

private:

    //
    // Point buffers
    //
    
    Buffer<float> mPointMassBuffer;
    Buffer<vec2f> mPointExternalForceBuffer;
    Buffer<vec2f> mPointPositionPredictionBuffer;


    //
    // Spring buffers
    //

    struct SpringScalingFactors
    {
        float EndpointA;
        float EndpointB;

        SpringScalingFactors()
            : EndpointA(0.0f)
            , EndpointB(0.0f)
        {}

        SpringScalingFactors(
            float endpointA,
            float endpointB)
            : EndpointA(endpointA)
            , EndpointB(endpointB)
        {}
    };

    Buffer<SpringScalingFactors> mSpringScalingFactorsBuffer;
};