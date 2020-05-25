/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "UIControls/ScalarTimeSeriesProbeControl.h"

#include <SLabCoreLib/ISimulationEventHandler.h>

#include <wx/sizer.h>
#include <wx/wx.h>

#include <memory>
#include <string>
#include <unordered_map>

class ProbeToolbar final
    : public wxPanel
    , public ISimulationEventHandler
{
public:

    ProbeToolbar(wxWindow* parent);

    virtual ~ProbeToolbar();

    void UpdateSimulation();

public:

    //
    // Simulation event handlers
    //

    virtual void OnSimulationReset() override;

    virtual void OnObjectProbe(
        float totalKineticEnergy,
        float totalPotentialEnergy) override;

    virtual void OnCustomProbe(
        std::string const & name,
        float value) override;

private:

    bool IsActive() const
    {
        return this->IsShown();
    }

    std::unique_ptr<ScalarTimeSeriesProbeControl> AddScalarTimeSeriesProbe(
        std::string const & name,
        int sampleCount);

private:

    //
    // Probes
    //

    wxBoxSizer * mProbesSizer;

    std::unique_ptr<ScalarTimeSeriesProbeControl> mKineticEnergyProbe;
    std::unique_ptr<ScalarTimeSeriesProbeControl> mPotentialEnergyProbe;
    std::unordered_map<std::string, std::unique_ptr<ScalarTimeSeriesProbeControl>> mCustomProbes;
};
