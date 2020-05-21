/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISimulator.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SimulatorRegistry
{
public:

    static std::vector<std::string> const & GetSimulatorTypeNames()
    {
        return mInstance.mSimulatorTypeNames;
    }

    static std::unique_ptr<ISimulator> MakeSimulator(std::string const & simulatorName)
    {
        return mInstance.mSimulatorFactories[simulatorName]();
    }

private:

    SimulatorRegistry();

    template<typename TSimulatorType>
    void RegisterSimulatorType();

private:

    static SimulatorRegistry mInstance;

    std::vector<std::string> mSimulatorTypeNames;
    std::unordered_map<std::string, std::function<std::unique_ptr<ISimulator>()>> mSimulatorFactories;
};