/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISimulator.h"

#include "ILayoutOptimizer.h"

#include <cassert>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class SimulatorRegistry
{
public:

    static std::string const & GetDefaultSimulatorTypeName()
    {
        assert(!mInstance.mSimulatorTypeNames.empty());

        return mInstance.mSimulatorTypeNames[0];
    }

    static std::vector<std::string> const & GetSimulatorTypeNames()
    {
        return mInstance.mSimulatorTypeNames;
    }

    static std::unique_ptr<ISimulator> MakeSimulator(
        std::string const & simulatorName,
        Object const & object,
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager)
    {
        assert(mInstance.mSimulatorFactories.count(simulatorName) == 1);

        return mInstance.mSimulatorFactories[simulatorName](object, simulationParameters, threadManager);
    }

    static ILayoutOptimizer const & GetLayoutOptimizer(std::string const & simulatorName)
    {
        assert(mInstance.mSimulatorLayoutOptimizers.count(simulatorName) == 1);

        return *mInstance.mSimulatorLayoutOptimizers[simulatorName];
    }

private:

    SimulatorRegistry();

    template<typename TSimulatorType>
    void RegisterSimulatorType();

private:

    using factory_function = std::function<std::unique_ptr<ISimulator>(
        Object const & object, 
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager)> ;

    static SimulatorRegistry mInstance;

    std::vector<std::string> mSimulatorTypeNames;
    std::unordered_map<std::string, factory_function> mSimulatorFactories;
    std::unordered_map<std::string, std::unique_ptr<ILayoutOptimizer>> mSimulatorLayoutOptimizers;
};