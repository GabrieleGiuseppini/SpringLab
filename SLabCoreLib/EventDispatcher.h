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

    virtual void OnObjectProbe(
        float totalKineticEnergy,
        float totalPotentialEnergy) override
    {
        for (auto sink : mSinks)
        {
            sink->OnObjectProbe(totalKineticEnergy, totalPotentialEnergy);
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
