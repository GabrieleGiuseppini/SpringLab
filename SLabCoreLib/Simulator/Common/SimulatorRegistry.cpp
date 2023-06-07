/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SimulatorRegistry.h"

#include "Simulator/Classic/ClassicSimulator.h"
#include "Simulator/FastMSS/FastMSSBasicSimulator.h"
#include "Simulator/FS/FSBaseSimulator.h"
#include "Simulator/FS/FSByPointSimulator.h"
#include "Simulator/FS/FSByPointCompactSimulator.h"
#include "Simulator/FS/FSByPointCompactIntegratingSimulator.h"
#include "Simulator/FS/FSBySpringIntrinsicsSimulator.h"
#include "Simulator/FS/FSBySpringIntrinsicsLayoutOptimizationSimulator.h"
#include "Simulator/FS/FSBySpringStructuralIntrinsicsSimulator.h"
#include "Simulator/FS/FSBySpringStructuralIntrinsicsMTSimulator.h"
#include "Simulator/FS/FSBySpringStructuralIntrinsicsMTVectorizedSimulator.h"
#include "Simulator/GaussSeidel/GaussSeidelByPointSimulator.h"
#include "Simulator/PositionBased/PositionBasedBasicSimulator.h"

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
    RegisterSimulatorType<FSBySpringStructuralIntrinsicsSimulator>();
    RegisterSimulatorType<FSBySpringStructuralIntrinsicsMTSimulator>();
    RegisterSimulatorType<FSBySpringStructuralIntrinsicsMTVectorizedSimulator>();
    RegisterSimulatorType<FSByPointSimulator>();
    RegisterSimulatorType<FSByPointCompactSimulator>();
    RegisterSimulatorType<FSByPointCompactIntegratingSimulator>();
    RegisterSimulatorType<GaussSeidelByPointSimulator>();
    RegisterSimulatorType<PositionBasedBasicSimulator>();
    RegisterSimulatorType<FastMSSBasicSimulator>();
}

/////////////////////////////////////

namespace {

    template<typename, typename = void>
    constexpr bool has_layout_optimizer = false;

    template<typename TSimulatorType>
    constexpr bool has_layout_optimizer<TSimulatorType, std::void_t<typename TSimulatorType::layout_optimizer>> = true;


    template <class TSimulatorType>
    std::unique_ptr<ILayoutOptimizer> make_simulator_optimizer()
    {
        if constexpr (has_layout_optimizer<TSimulatorType>)
        {
            return std::make_unique<typename TSimulatorType::layout_optimizer>();
        }
        else
        {
            return std::unique_ptr<IdempotentLayoutOptimizer>(new IdempotentLayoutOptimizer());
        }
    }
}

template<typename TSimulatorType>
void SimulatorRegistry::RegisterSimulatorType()
{
    std::string const simulatorName = TSimulatorType::GetSimulatorName();

    mSimulatorTypeNames.push_back(simulatorName);

    mSimulatorFactories.emplace(
        simulatorName,
        [](Object const & object, SimulationParameters const & simulationParameters, ThreadManager const & threadManager)
        {
            return std::make_unique<TSimulatorType>(object, simulationParameters, threadManager);
        });

    static_assert(has_layout_optimizer<FSBySpringIntrinsicsLayoutOptimizationSimulator>);

    mSimulatorLayoutOptimizers.emplace(
        simulatorName,
        make_simulator_optimizer<TSimulatorType>());
}