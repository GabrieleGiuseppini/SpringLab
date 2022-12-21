/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-13
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISimulationEventHandler.h"

#include <vector>

class EventDispatcher final : public ISimulationEventHandler
{
public:

    EventDispatcher()
        : mSinks()
    {
    }

public:

    virtual void OnSimulationReset() override
    {
        for (auto sink : mSinks)
        {
            sink->OnSimulationReset();
        }
    }

    virtual void OnMeasurement(
        float totalKineticEnergy,
        float totalPotentialEnergy,
        std::optional<float> bending,
        std::chrono::microseconds lastSimulationDuration,
        std::chrono::microseconds avgSimulationDuration) override
    {
        for (auto sink : mSinks)
        {
            sink->OnMeasurement(
                totalKineticEnergy, 
                totalPotentialEnergy,
                bending,
                lastSimulationDuration,
                avgSimulationDuration);
        }
    }

    virtual void OnCustomProbe(
        std::string const & name,
        float value) override
    {
        for (auto sink : mSinks)
        {
            sink->OnCustomProbe(name, value);
        }
    }

public:

    void RegisterEventHandler(ISimulationEventHandler * sink)
    {
        mSinks.push_back(sink);
    }

private:

    // The registered sinks
    std::vector<ISimulationEventHandler *> mSinks;
};
