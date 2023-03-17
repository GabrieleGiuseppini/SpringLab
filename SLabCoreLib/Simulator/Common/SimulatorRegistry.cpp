/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SimulatorRegistry.h"

#include "Simulator/Classic/ClassicSimulator.h"
#include "Simulator/FS/FSBaseSimulator.h"
#include "Simulator/FS/FSByPointSimulator.h"
#include "Simulator/FS/FSByPointCompactSimulator.h"
#include "Simulator/FS/FSByPointCompactIntegratingSimulator.h"
#include "Simulator/FS/FSByPointGaussSeidelSimulator.h"

SimulatorRegistry SimulatorRegistry::mInstance;

SimulatorRegistry::SimulatorRegistry()
{
    //
    // Register all simulator types
    //

    RegisterSimulatorType<ClassicSimulator>();
    RegisterSimulatorType<FSBaseSimulator>();
    RegisterSimulatorType<FSByPointSimulator>();
    RegisterSimulatorType<FSByPointCompactSimulator>();
    RegisterSimulatorType<FSByPointCompactIntegratingSimulator>();
    RegisterSimulatorType<FSByPointGaussSeidelSimulator>();
}

template<typename TSimulatorType>
void SimulatorRegistry::RegisterSimulatorType()
{
    std::string const simulatorName = TSimulatorType::GetSimulatorName();

    mSimulatorTypeNames.push_back(simulatorName);
    mSimulatorFactories.emplace(
        simulatorName,
        [](Object const & object, SimulationParameters const & simulationParameters)
        {
            return std::make_unique<TSimulatorType>(object, simulationParameters);
        });
}