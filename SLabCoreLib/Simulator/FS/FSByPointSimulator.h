/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include <memory>
#include <string>

/*
 * Simulator implementing the same spring relaxation algorithm
 * as Floating Sandbox 1.17.5, but pivoted on a by-point visit;
 * not optimized.
 */
class FSByPointSimulator final : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "FS 20 - By Point";
    }

public:

    FSByPointSimulator(
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

    void ApplySpringsForces(Object const & object);

    void IntegrateAndResetSpringForces(
        Object & object,
        SimulationParameters const & simulationParameters);

private:

    //
    // Point buffers
    //

    Buffer<vec2f> mPointSpringForceBuffer;
    Buffer<vec2f> mPointExternalForceBuffer;
    Buffer<float> mPointIntegrationFactorBuffer; // dt^2/Mass or zero when the point is frozen


    //
    // Spring buffers
    //

    Buffer<float> mSpringStiffnessCoefficientBuffer;
    Buffer<float> mSpringDampingCoefficientBuffer;
};