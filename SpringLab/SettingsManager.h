/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <SLabCoreLib/Settings.h>
#include <SLabCoreLib/SimulationController.h>

enum class SLabSettings : size_t
{
    CommonSimulationTimeStepDuration = 0,
    CommonMassAdjustment,
    CommonGravityAdjustment,
    CommonDoApplyGravity,
    CommonGlobalDamping,

    ClassicSimulatorSpringReductionFraction,
    ClassicSimulatorSpringDampingCoefficient,
    ClassicSimulatorSpringForceInertia,

    DoRenderAssignedParticleForces,

    _Last = DoRenderAssignedParticleForces
};

class SettingsManager final : public BaseSettingsManager<SLabSettings>
{
public:

    SettingsManager(
        std::shared_ptr<SimulationController> simulationController,
        std::filesystem::path const & rootUserSettingsDirectoryPath);

private:

    static BaseSettingsManagerFactory MakeSettingsFactory(
        std::shared_ptr<SimulationController> simulationController);
};