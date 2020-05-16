/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ProbePanel.h"

#include <wx/stattext.h>

#include <cassert>

static constexpr int TopPadding = 2;
static constexpr int ProbePadding = 10;

ProbePanel::ProbePanel(wxWindow* parent)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
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
    // Finalize
    //

    SetSizerAndFit(mProbesSizer);
}

ProbePanel::~ProbePanel()
{
}

void ProbePanel::UpdateSimulation()
{
    //
    // Update all probes
    //

    if (IsActive())
    {
        for (auto const & p : mCustomProbes)
        {
            p.second->UpdateSimulation();
        }
    }
}

std::unique_ptr<ScalarTimeSeriesProbeControl> ProbePanel::AddScalarTimeSeriesProbe(
    std::string const & name,
    int sampleCount)
{
    wxBoxSizer * sizer = new wxBoxSizer(wxVERTICAL);

    sizer->AddSpacer(TopPadding);

    auto probe = std::make_unique<ScalarTimeSeriesProbeControl>(this, sampleCount);
    sizer->Add(probe.get(), 1, wxALIGN_CENTRE, 0);

    wxStaticText * label = new wxStaticText(this, wxID_ANY, name, wxDefaultPosition, wxDefaultSize, wxALIGN_CENTRE_HORIZONTAL);
    sizer->Add(label, 0, wxALIGN_CENTRE, 0);

    mProbesSizer->Add(sizer, 1, wxLEFT | wxRIGHT, ProbePadding);

    return probe;
}

///////////////////////////////////////////////////////////////////////////////////////

void ProbePanel::OnSimulationReset()
{
    for (auto const & p : mCustomProbes)
    {
        p.second->Reset();
    }
}

void ProbePanel::OnCustomProbe(
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