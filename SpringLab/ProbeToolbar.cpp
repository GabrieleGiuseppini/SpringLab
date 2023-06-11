/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-15
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ProbeToolbar.h"

#include <wx/gbsizer.h>
#include <wx/stattext.h>

#include <cassert>
#include <iomanip>
#include <sstream>

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
    // Setup UI
    //

    {
        int constexpr HMargin = 10;

        wxBoxSizer * hSizer = new wxBoxSizer(wxHORIZONTAL);

        hSizer->AddSpacer(HMargin);

        // Fixed scalars
        {
            int constexpr TextCtrlWidth = 100;

            wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

            // Num Springs
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Springs:"));
                gridSizer->Add(
                    label,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,
                    0);

                mNumSpringsTextCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(TextCtrlWidth, -1), wxTE_RIGHT | wxTE_READONLY);
                gridSizer->Add(
                    mNumSpringsTextCtrl,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND,
                    0);
            }

            // Bending
            {
                auto label = new wxStaticText(this, wxID_ANY, _("Bending:"));
                gridSizer->Add(
                    label,
                    wxGBPosition(1, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,
                    0);

                mBendingTextCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(TextCtrlWidth, -1), wxTE_RIGHT | wxTE_READONLY);
                gridSizer->Add(
                    mBendingTextCtrl,
                    wxGBPosition(1, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND,
                    0);
            }

            // Update duration
            {
                auto label1 = new wxStaticText(this, wxID_ANY, _("CPU time:"));
                gridSizer->Add(
                    label1,
                    wxGBPosition(2, 0),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,
                    0);

                mLastSimulationDurationTextCtrl = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(TextCtrlWidth, -1), wxTE_RIGHT | wxTE_READONLY);
                gridSizer->Add(
                    mLastSimulationDurationTextCtrl,
                    wxGBPosition(2, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND,
                    0);

                auto label2 = new wxStaticText(this, wxID_ANY, _("us"));
                gridSizer->Add(
                    label2,
                    wxGBPosition(2, 2),
                    wxGBSpan(1, 1),
                    wxALIGN_LEFT | wxALIGN_CENTER_VERTICAL,
                    0);
            }

            hSizer->Add(
                gridSizer,
                0,
                0,
                0);
        }

        // Probes
        {
            mProbesSizer = new wxBoxSizer(wxHORIZONTAL);

            mKineticEnergyProbe = AddScalarTimeSeriesProbe("Kinetic Energy", 200);
            mPotentialEnergyProbe = AddScalarTimeSeriesProbe("Potential Energy", 200);

            hSizer->Add(
                mProbesSizer,
                0,
                0,
                0);
        }

        hSizer->AddSpacer(HMargin);

        SetSizer(hSizer);
    }
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

void ProbeToolbar::OnSimulationReset(size_t numSprings)
{
    mNumSpringsTextCtrl->SetValue(std::to_string(numSprings));
    mBendingTextCtrl->SetValue("");
    mLastSimulationDurationTextCtrl->SetValue("");

    mKineticEnergyProbe->Reset();
    mPotentialEnergyProbe->Reset();

    for (auto const & p : mCustomProbes)
    {
        p.second->Reset();
    }

    mSimulationDurationRunningAverage.Reset(0.0f);
}

void ProbeToolbar::OnMeasurement(
    float totalKineticEnergy,
    float totalPotentialEnergy,
    std::optional<float> bending,
    std::chrono::nanoseconds lastSimulationDuration,
    std::chrono::nanoseconds /*avgSimulationDuration*/)
{
    // Bending
    if (bending)
    {
        std::ostringstream ss;
        ss.fill('0');
        ss << std::fixed << std::setprecision(2) << *bending;

        mBendingTextCtrl->SetValue(ss.str());
    }
    else
    {
        mBendingTextCtrl->SetValue("");
    }
    
    // Simulation time
    {
        float const lastSimulationDurationMicroSeconds =
            static_cast<float>(std::chrono::duration_cast<std::chrono::nanoseconds>(lastSimulationDuration).count())
            / 1000.0f;

        mSimulationDurationRunningAverage.Update(lastSimulationDurationMicroSeconds);

        std::ostringstream ss;
        ss.fill('0');
        ss << std::fixed << std::setprecision(2) << mSimulationDurationRunningAverage.GetCurrentAverage();

        mLastSimulationDurationTextCtrl->SetValue(ss.str());
    }

    // Time series
    mKineticEnergyProbe->RegisterSample(totalKineticEnergy);
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