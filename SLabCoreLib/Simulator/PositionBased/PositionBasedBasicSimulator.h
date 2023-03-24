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
        SimulationParameters const & simulationParameters);

    //////////////////////////////////////////////////////////
    // ISimulator
    //////////////////////////////////////////////////////////

    void OnStateChanged(
        Object const & object,
        SimulationParameters const & simulationParameters) override;

    void Update(
        Object & object,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) override;

private:

    void CreateState(
        Object const & object,
        SimulationParameters const & simulationParameters);

private:

    //
    // Point buffers
    //

    Buffer<vec2f> mPointExternalForceBuffer;


    //
    // Spring buffers
    //

    Buffer<float> mSpringStiffnessCoefficientBuffer;
    Buffer<float> mSpringDampingCoefficientBuffer;
};