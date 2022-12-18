/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SettingsManager.h"

#include <cctype>
#include <sstream>

std::string MangleSettingName(std::string && settingName);

#define ADD_SETTING(type, name)                      \
    factory.AddSetting<type>(                           \
        SLabSettings::name,                             \
        MangleSettingName(#name),                       \
        [simulationController]() -> type { return simulationController->Get##name(); }, \
        [simulationController](auto const & v) { simulationController->Set##name(v); }, \
		[simulationController](auto const & v) { simulationController->Set##name(v); });

BaseSettingsManager<SLabSettings>::BaseSettingsManagerFactory SettingsManager::MakeSettingsFactory(
    std::shared_ptr<SimulationController> simulationController)
{
    BaseSettingsManagerFactory factory;

    ADD_SETTING(float, CommonSimulationTimeStepDuration);
    ADD_SETTING(float, CommonMassAdjustment);
    ADD_SETTING(float, CommonGravityAdjustment);
    ADD_SETTING(bool, CommonDoApplyGravity);
    ADD_SETTING(float, CommonGlobalDamping);

    ADD_SETTING(float, ClassicSimulatorSpringStiffnessCoefficient);
    ADD_SETTING(float, ClassicSimulatorSpringDampingCoefficient);

    ADD_SETTING(bool, DoRenderAssignedParticleForces);

    return factory;
}

SettingsManager::SettingsManager(
    std::shared_ptr<SimulationController> simulationController,
    std::filesystem::path const & rootUserSettingsDirectoryPath)
    : BaseSettingsManager<SLabSettings>(
        MakeSettingsFactory(simulationController),
        rootUserSettingsDirectoryPath,
        rootUserSettingsDirectoryPath)
{}

std::string MangleSettingName(std::string && settingName)
{
    std::stringstream ss;

    bool isFirst = true;
    for (char ch : settingName)
    {
        if (std::isupper(ch))
        {
            if (!isFirst)
            {
                ss << '_';
            }

            ss << static_cast<char>(std::tolower(ch));
        }
        else
        {
            ss << ch;
        }

        isFirst = false;
    }

    return ss.str();
}