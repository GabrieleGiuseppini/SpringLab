/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ControlToolbar.h"

#include "WxHelpers.h"

#include <SLabCoreLib/Log.h>
#include <SLabCoreLib/ResourceLocator.h>

#include <wx/bitmap.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>

#include <cassert>

long const ControlToolbar::ID_SIMULATION_CONTROL_PLAY = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_FAST_PLAY = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_PAUSE = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_STEP = wxNewId();

long const ControlToolbar::ID_INITIAL_CONDITIONS_GRAVITY = wxNewId();
long const ControlToolbar::ID_INITIAL_CONDITIONS_MOVE = wxNewId();
long const ControlToolbar::ID_INITIAL_CONDITIONS_PIN = wxNewId();
long const ControlToolbar::ID_INITIAL_CONDITIONS_PARTICLE_FORCE = wxNewId();

ControlToolbar::ControlToolbar(wxWindow* parent)
    : wxPanel(
        parent,
        wxID_ANY,
        wxDefaultPosition,
        wxDefaultSize,
        wxBORDER_SIMPLE)
{
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    wxBoxSizer * vSizer = new wxBoxSizer(wxVERTICAL);

    // Simulation control
    {
        wxGridSizer * gridSizer = new wxGridSizer(2, 2, 2);

        // Play
        {
            auto button = mSimulationControlButtons.emplace_back(
                new wxBitmapToggleButton(
                    this,
                    ID_SIMULATION_CONTROL_PLAY,
                    wxBitmap(
                        (ResourceLocator::GetResourcesFolderPath() / "simcontrol_play.png").string(),
                        wxBITMAP_TYPE_PNG),
                    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT));

            button->Bind(wxEVT_TOGGLEBUTTON, [this, button](wxCommandEvent & /*event*/) { OnSimulationControlButton(button); });

            button->SetToolTip("Start simulation auto-play step-by-step");

            gridSizer->Add(button);
        }

        // Fast Play
        {
            auto button = mSimulationControlButtons.emplace_back(
                new wxBitmapToggleButton(
                    this,
                    ID_SIMULATION_CONTROL_FAST_PLAY,
                    wxBitmap(
                        (ResourceLocator::GetResourcesFolderPath() / "simcontrol_play_fast.png").string(),
                        wxBITMAP_TYPE_PNG),
                    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT));

            button->Bind(wxEVT_TOGGLEBUTTON, [this, button](wxCommandEvent & /*event*/) { OnSimulationControlButton(button); });

            button->SetToolTip("Start simulation auto-play as fast as possible");

            gridSizer->Add(button);
        }

        // Pause
        {
            auto button = mSimulationControlButtons.emplace_back(
                new wxBitmapToggleButton(
                    this,
                    ID_SIMULATION_CONTROL_PAUSE,
                    wxBitmap(
                        (ResourceLocator::GetResourcesFolderPath() / "simcontrol_pause.png").string(),
                        wxBITMAP_TYPE_PNG),
                    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT));

            button->SetValue(true); // We start in pause

            button->Bind(wxEVT_TOGGLEBUTTON, [this, button](wxCommandEvent & /*event*/) { OnSimulationControlButton(button); });

            button->SetToolTip("Pause simulation auto-play (SPACE)");

            gridSizer->Add(button);
        }

        // Step
        {
            auto button = mSimulationControlButtons.emplace_back(
                new wxBitmapToggleButton(
                    this,
                    ID_SIMULATION_CONTROL_STEP,
                    wxBitmap(
                        (ResourceLocator::GetResourcesFolderPath() / "simcontrol_step.png").string(),
                        wxBITMAP_TYPE_PNG),
                    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT));

            button->Bind(wxEVT_TOGGLEBUTTON, [this, button](wxCommandEvent & /*event*/) { OnSimulationControlButton(button); });

            button->SetToolTip("Run a single simulation step (ENTER)");

            gridSizer->Add(button);
        }

        vSizer->Add(gridSizer, 0, wxALIGN_CENTER | wxALL, 5);
    }

    vSizer->AddSpacer(10);

    // Initial conditions
    {
        wxGridSizer * gridSizer = new wxGridSizer(2, 2, 2);

        // Gravity
        {
            auto button = mSimulationControlButtons.emplace_back(
                new wxBitmapToggleButton(
                    this,
                    ID_INITIAL_CONDITIONS_GRAVITY,
                    wxBitmap(
                        (ResourceLocator::GetResourcesFolderPath() / "initialconds_gravity.png").string(),
                        wxBITMAP_TYPE_PNG),
                    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT));

            button->Bind(wxEVT_TOGGLEBUTTON, [this, button](wxCommandEvent & /*event*/) { OnInitialConditionsButton(button); });

            button->SetToolTip("Enable or disable gravity");

            gridSizer->Add(button);
        }

        // Move
        {
            auto button = mSimulationControlButtons.emplace_back(
                new wxBitmapToggleButton(
                    this,
                    ID_INITIAL_CONDITIONS_MOVE,
                    wxBitmap(
                        (ResourceLocator::GetResourcesFolderPath() / "move_cursor_up.png").string(),
                        wxBITMAP_TYPE_PNG),
                    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT));

            button->SetValue(true); // Initial tool

            button->Bind(wxEVT_TOGGLEBUTTON, [this, button](wxCommandEvent & /*event*/) { OnInitialConditionsButton(button); });

            button->SetToolTip("Move particles");

            gridSizer->Add(button);
        }

        // Pin
        {
            auto button = mSimulationControlButtons.emplace_back(
                new wxBitmapToggleButton(
                    this,
                    ID_INITIAL_CONDITIONS_MOVE,
                    wxBitmap(
                        (ResourceLocator::GetResourcesFolderPath() / "pin_cursor.png").string(),
                        wxBITMAP_TYPE_PNG),
                    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT));

            button->Bind(wxEVT_TOGGLEBUTTON, [this, button](wxCommandEvent & /*event*/) { OnInitialConditionsButton(button); });

            button->SetToolTip("Pin particles to their current positions");

            gridSizer->Add(button);
        }

        // Particle Force
        {
            auto button = mSimulationControlButtons.emplace_back(
                new wxBitmapToggleButton(
                    this,
                    ID_INITIAL_CONDITIONS_PARTICLE_FORCE,
                    wxBitmap(
                        (ResourceLocator::GetResourcesFolderPath() / "particle_force_cursor.png").string(),
                        wxBITMAP_TYPE_PNG),
                    wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT));

            button->Bind(wxEVT_TOGGLEBUTTON, [this, button](wxCommandEvent & /*event*/) { OnInitialConditionsButton(button); });

            button->SetToolTip("Set a force on individual particles");

            gridSizer->Add(button);
        }

        vSizer->Add(gridSizer, 0, wxALIGN_CENTER | wxALL, 5);
    }

    // TODO: simulator type

    this->SetSizer(vSizer);
}

ControlToolbar::~ControlToolbar()
{
}

bool ControlToolbar::ProcessKeyDown(
    int keyCode,
    int /*keyModifiers*/)
{
    if (keyCode == WXK_SPACE)
    {
        // Pause
        mSimulationControlButtons[2]->SetValue(true);
        OnSimulationControlButton(mSimulationControlButtons[2]);
        return true;
    }
    else if (keyCode == WXK_RETURN)
    {
        // Step
        mSimulationControlButtons[3]->SetValue(true);
        OnSimulationControlButton(mSimulationControlButtons[3]);
        return true;
    }

    return false;
}

void ControlToolbar::OnSimulationControlButton(wxBitmapToggleButton * button)
{
    if (button->GetValue())
    {
        // Turn off all others
        for (auto btn : mSimulationControlButtons)
        {
            if (btn->GetId() != button->GetId())
                btn->SetValue(false);
        }

        // Fire event
        wxCommandEvent evt(wxEVT_BUTTON, button->GetId());
        ProcessEvent(evt);
    }
    else
    {
        // Re-turn on this one
        button->SetValue(true);
    }
}

void ControlToolbar::OnInitialConditionsButton(wxBitmapToggleButton * button)
{
    if (button->GetId() == ID_INITIAL_CONDITIONS_GRAVITY)
    {
        // Fire event
        wxCommandEvent evt(wxEVT_BUTTON, button->GetId());
        evt.SetId(button->GetValue() ? 1 : 0);
        ProcessEvent(evt);
    }
    else
    {
        if (button->GetValue())
        {
            // Turn off all others
            for (auto btn : mInitialConditionsButtons)
            {
                if (btn->GetId() != button->GetId()
                    && btn->GetId() != ID_INITIAL_CONDITIONS_GRAVITY)
                    btn->SetValue(false);
            }

            // Fire event
            wxCommandEvent evt(wxEVT_BUTTON, button->GetId());
            ProcessEvent(evt);
        }
        else
        {
            // Re-turn on this one
            button->SetValue(true);
        }
    }
}