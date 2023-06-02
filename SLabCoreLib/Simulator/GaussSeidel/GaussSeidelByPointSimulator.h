/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-06-02
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include <memory>
#include <string>

/*
 * Simulator implementing a Gauss-Seidel solve per-point.
 */
class GaussSeidelByPointSimulator : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "Gauss-Seidel - By Point";
    }

public:

    GaussSeidelByPointSimulator(
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

    void Integrate(
        Object & object,
        SimulationParameters const & simulationParameters);

    void RelaxSprings(
        Object & object,
        SimulationParameters const & simulationParameters);

private:

    //
    // Point buffers
    //

    Buffer<vec2f> mPointExternalForceBuffer;
    Buffer<float> mPointIntegrationFactorBuffer; // dt^2/Mass or zero when the point is frozen


    //
    // Spring buffers
    //

    Buffer<float> mSpringStiffnessCoefficientBuffer;
    Buffer<float> mSpringDampingCoefficientBuffer;
};