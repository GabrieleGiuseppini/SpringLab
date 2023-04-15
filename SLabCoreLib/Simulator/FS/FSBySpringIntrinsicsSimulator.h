/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-18
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include <memory>
#include <string>

/*
 * Simulator implementing the same spring relaxation algorithm
 * as Floating Sandbox 1.17.5, optimized with intrinsics.
 */
class FSBySpringIntrinsicsSimulator : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "FS 10 - By Spring - Instrinsics";
    }

public:

    FSBySpringIntrinsicsSimulator(
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
    Buffer<vec2f> mPointIntegrationFactorBuffer; // dt^2/Mass or zero when the point is frozen; identical elements, one for x and one for y


    //
    // Spring buffers
    //

    Buffer<float> mSpringStiffnessCoefficientBuffer;
    Buffer<float> mSpringDampingCoefficientBuffer;
};