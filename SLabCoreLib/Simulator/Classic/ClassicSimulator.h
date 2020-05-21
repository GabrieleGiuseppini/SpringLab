/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include <string>

/*
 * This simulator calculates the next step's physical properties
 * based on their current values and on the Hookean forces of springs.
 */
class ClassicSimulator final : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "Classic";
    }

public:

    ClassicSimulator(
        Object const & object,
        SimulationParameters const & simulationParameters);

    //////////////////////////////////////////////////////////
    // ISimulator
    //////////////////////////////////////////////////////////

    void OnSimulationParametersChanged(SimulationParameters const & simulationParameters) override;

    void Update(
        Object & object,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) override;

private:

};