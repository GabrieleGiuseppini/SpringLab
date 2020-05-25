/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ProbeToolbar.h"

#include <wx/stattext.h>

#include <cassert>

static constexpr int TopPadding = 2;
static constexpr int ProbePadding = 10;
static constexpr int ProbeHeight = 80;

ProbeToolbar::ProbeToolbar(wxWindow* parent)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxSize(-1, -1),
        wxBORDER_SIMPLE | wxCLIP_CHILDREN)
{
#ifdef __WXMSW__
    SetDoubleBuffered(true);
#endif

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));


    //
    // Create probes' sizer
    //

    mProbesSizer = new wxBoxSizer(wxHORIZONTAL);


    //
    // Create default probes
    //

    mKineticEnergyProbe = AddScalarTimeSeriesProbe("Kinetic Energy", 200);
    mPotentialEnergyProbe = AddScalarTimeSeriesProbe("Potential Energy", 200);
    mProbesSizer->Layout();


    //
    // Finalize
    //

    SetSizer(mProbesSizer);
}

ProbeToolbar::~ProbeToolbar()
{
}

void ProbeToolbar::UpdateSimulation()
{
    //
    // Update all probes
    //

    if (IsActive())
    {
        mKineticEnergyProbe->UpdateSimulation();
        mPotentialEnergyProbe->UpdateSimulation();

        for (auto const & p : mCustomProbes)
        {
            p.second->UpdateSimulation();
        }
    }
}

std::unique_ptr<ScalarTimeSeriesProbeControl> ProbeToolbar::AddScalarTimeSeriesProbe(
    std::string const & name,
    int sampleCount)
{
    wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

    sizer->AddSpacer(TopPadding);

    auto probe = std::make_unique<ScalarTimeSeriesProbeControl>(this, sampleCount, ProbeHeight);
    sizer->Add(probe.get(), 1, wxALIGN_CENTRE, 0);

    wxStaticText * label = new wxStaticText(this, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(label, 0, wxALIGN_CENTRE, 0);

    mProbesSizer->Add(sizer, 1, wxLEFT | wxRIGHT, ProbePadding);

    return probe;
}

///////////////////////////////////////////////////////////////////////////////////////

void ProbeToolbar::OnSimulationReset()
{
    mKineticEnergyProbe->Reset();
    mPotentialEnergyProbe->Reset();

    for (auto const & p : mCustomProbes)
    {
        p.second->Reset();
    }
}

void ProbeToolbar::OnObjectProbe(
    float totalKineticEnergy,
    float totalPotentialEnergy)
{
    assert(!!mKineticEnergyProbe);
    mKineticEnergyProbe->RegisterSample(totalKineticEnergy);

    assert(!!mPotentialEnergyProbe);
    mPotentialEnergyProbe->RegisterSample(totalPotentialEnergy);
}

void ProbeToolbar::OnCustomProbe(
    std::string const & name,
    float value)
{
    auto & probe = mCustomProbes[name];
    if (!probe)
    {
        probe = AddScalarTimeSeriesProbe(name, 100);
        mProbesSizer->Layout();
    }

    probe->RegisterSample(value);
}