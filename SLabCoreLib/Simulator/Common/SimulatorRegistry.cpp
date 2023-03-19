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
#include "Simulator/FS/FSBySpringIntrinsicsSimulator.h"
#include "Simulator/FS/FSBySpringIntrinsicsLayoutOptimizationSimulator.h"

#include <type_traits>

SimulatorRegistry SimulatorRegistry::mInstance;

SimulatorRegistry::SimulatorRegistry()
{
    //
    // Register all simulator types
    //

    RegisterSimulatorType<ClassicSimulator>();
    RegisterSimulatorType<FSBaseSimulator>();
    RegisterSimulatorType<FSBySpringIntrinsicsSimulator>();
    RegisterSimulatorType<FSBySpringIntrinsicsLayoutOptimizationSimulator>();
    RegisterSimulatorType<FSByPointSimulator>();
    RegisterSimulatorType<FSByPointCompactSimulator>();
    RegisterSimulatorType<FSByPointCompactIntegratingSimulator>();    
}

/////////////////////////////////////

namespace {

    template <typename TSimulatorType, typename = void>
    struct has_layout_optimizer : std::false_type {};

    template <typename TSimulatorType>
    struct has_layout_optimizer<TSimulatorType, decltype((void)TSimulatorType::layout_optimizer, void())> : std::true_type {};

    template <class TSimulatorType>
    std::unique_ptr<ILayoutOptimizer> make_simulator_optimizer(std::false_type)
    {
        return std::unique_ptr<IdempotentLayoutOptimizer>(new IdempotentLayoutOptimizer());
    }

    template <class TSimulatorType>
    std::unique_ptr<ILayoutOptimizer> make_simulator_optimizer(std::true_type)
    {
        return std::make_unique<TSimulatorType::layout_optimizer>();
    }
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

    mSimulatorLayoutOptimizers.emplace(
        simulatorName,
        make_simulator_optimizer<TSimulatorType>(has_layout_optimizer<TSimulatorType>{}));
}