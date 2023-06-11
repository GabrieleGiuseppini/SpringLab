/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-15
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SLabTypes.h"

#include <chrono>
#include <optional>
#include <string>

/*
 * These interfaces define the methods that simulation event handlers must implement.
 *
 * The methods are default-implemented to facilitate implementation of handlers that
 * only care about a subset of the events.
 */

struct ISimulationEventHandler
{
    virtual void OnSimulationReset(size_t /*numSprings*/)
    {
        // Default-implemented
    }

    virtual void OnMeasurement(
        float /*totalKineticEnergy*/,
        float /*totalPotentialEnergy*/,
        std::optional<float> /*bending*/,
        std::chrono::nanoseconds /*lastSimulationDuration*/,
        std::chrono::nanoseconds /*avgSimulationDuration*/)
    {
        // Default-implemented
    }

    virtual void OnCustomProbe(
        std::string const & /*name*/,
        float /*value*/)
    {
        // Default-implemented
    }
};
