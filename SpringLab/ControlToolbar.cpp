/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ControlToolbar.h"

#include "WxHelpers.h"

#include <SLabCoreLib/Log.h>
#include <SLabCoreLib/ResourceLocator.h>
#include <SLabCoreLib/Simulator/Common/SimulatorRegistry.h>

#include <wx/bitmap.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>

#include <cassert>

wxEventType const ControlToolbar::wxEVT_TOOLBAR_ACTION = wxNewEventType();

long const ControlToolbar::ID_SIMULATION_CONTROL_PLAY = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_FAST_PLAY = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_PAUSE = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_STEP = wxNewId();

long const ControlToolbar::ID_INITIAL_CONDITIONS_GRAVITY = wxNewId();
long const ControlToolbar::ID_INITIAL_CONDITIONS_MOVE = wxNewId();
long const ControlToolbar::ID_INITIAL_CONDITIONS_PIN = wxNewId();
long const ControlToolbar::ID_INITIAL_CONDITIONS_PARTICLE_FORCE = wxNewId();

long const ControlToolbar::ID_SIMULATOR_TYPE = wxNewId();

long const ControlToolbar::ID_ACTION_RESET = wxNewId();
long const ControlToolbar::ID_ACTION_LOAD_OBJECT = wxNewId();
long const ControlToolbar::ID_ACTION_SETTINGS = wxNewId();

long const ControlToolbar::ID_VIEW_CONTROL_GRID = wxNewId();

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

    // Simulator type
    {
        {
            wxStaticText * label = new wxStaticText(this, wxID_ANY, "Simulator:");

            vSizer->Add(label, 0, wxALIGN_LEFT | wxLEFT | wxTOP, 5);
        }

        {
            mSimulatorTypeChoice = new wxChoice(
                this,
                ID_SIMULATOR_TYPE,
                wxDefaultPosition, wxDefaultSize);

            // Populate
            for (auto const & simulatorTypeName : SimulatorRegistry::GetSimulatorTypeNames())
            {
                mSimulatorTypeChoice->Append(simulatorTypeName);
            }

            mSimulatorTypeChoice->Select(0); // Select first

            mSimulatorTypeChoice->Bind(wxEVT_CHOICE, [this](wxCommandEvent & /*event*/) { OnSimulatorTypeChoiceChanged(); });

            mSimulatorTypeChoice->SetToolTip("Change the algorithm for the simulation of the mass-spring network");

            vSizer->Add(mSimulatorTypeChoice, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 5);
        }
    }

    vSizer->AddSpacer(10);

    // Simulation control
    {
        wxGridSizer * gridSizer = new wxGridSizer(2, 2, 2);

        // Play
        {
            mSimulationControlPlayButton = new wxBitmapToggleButton(
                this,
                ID_SIMULATION_CONTROL_PLAY,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "simcontrol_play.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            mSimulationControlPlayButton->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent & /*event*/) { OnSimulationControlButton(mSimulationControlPlayButton); });

            mSimulationControlPlayButton->SetToolTip("Start simulation auto-play step-by-step");

            gridSizer->Add(mSimulationControlPlayButton);
        }

        // Fast Play
        {
            mSimulationControlFastPlayButton = new wxBitmapToggleButton(
                this,
                ID_SIMULATION_CONTROL_FAST_PLAY,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "simcontrol_play_fast.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            mSimulationControlFastPlayButton->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent & /*event*/) { OnSimulationControlButton(mSimulationControlFastPlayButton); });

            mSimulationControlFastPlayButton->SetToolTip("Start simulation auto-play as fast as possible");

            gridSizer->Add(mSimulationControlFastPlayButton);
        }

        // Pause
        {
            mSimulationControlPauseButton = new wxBitmapToggleButton(
                this,
                ID_SIMULATION_CONTROL_PAUSE,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "simcontrol_pause.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            mSimulationControlPauseButton->SetValue(true); // We start in pause

            mSimulationControlPauseButton->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent & /*event*/) { OnSimulationControlButton(mSimulationControlPauseButton); });

            mSimulationControlPauseButton->SetToolTip("Pause simulation auto-play (SPACE)");

            gridSizer->Add(mSimulationControlPauseButton);
        }

        // Step
        {
            mSimulationControlStepButton = new wxButton(
                this,
                ID_SIMULATION_CONTROL_STEP,
                wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            mSimulationControlStepButton->SetBitmap(
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "simcontrol_step.png").string(),
                    wxBITMAP_TYPE_PNG));

            mSimulationControlStepButton->Enable(true); // We start in pause

            mSimulationControlStepButton->Bind(wxEVT_BUTTON, [this](wxCommandEvent & /*event*/) { OnSimulationControlStepButton(); });

            mSimulationControlStepButton->SetToolTip("Run a single simulation step (ENTER)");

            gridSizer->Add(mSimulationControlStepButton);
        }

        vSizer->Add(gridSizer, 0, wxALIGN_CENTER | wxALL, 5);
    }

    vSizer->AddSpacer(10);

    // Initial conditions
    {
        wxGridSizer * gridSizer = new wxGridSizer(2, 2, 2);

        // Gravity
        {
            mInitialConditionsGravityButton = new wxBitmapToggleButton(
                this,
                ID_INITIAL_CONDITIONS_GRAVITY,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "initialconds_gravity.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            mInitialConditionsGravityButton->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent & /*event*/) { OnInitialConditionsButton(mInitialConditionsGravityButton); });

            mInitialConditionsGravityButton->SetToolTip("Enable or disable gravity");

            gridSizer->Add(mInitialConditionsGravityButton);
        }

        // Move
        {
            mInitialConditionsMoveButton = new wxBitmapToggleButton(
                this,
                ID_INITIAL_CONDITIONS_MOVE,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "move_cursor_up.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            mInitialConditionsMoveButton->SetValue(true); // Initial tool

            mInitialConditionsMoveButton->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent & /*event*/) { OnInitialConditionsButton(mInitialConditionsMoveButton); });

            mInitialConditionsMoveButton->SetToolTip("Move particles (M)");

            gridSizer->Add(mInitialConditionsMoveButton);
        }

        // Pin
        {
            mInitialConditionsPinButton = new wxBitmapToggleButton(
                this,
                ID_INITIAL_CONDITIONS_PIN,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "pin_cursor.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            mInitialConditionsPinButton->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent & /*event*/) { OnInitialConditionsButton(mInitialConditionsPinButton); });

            mInitialConditionsPinButton->SetToolTip("Pin particles to their current positions (P)");

            gridSizer->Add(mInitialConditionsPinButton);
        }

        // Particle Force
        {
            mInitialConditionsParticleForceButton = new wxBitmapToggleButton(
                this,
                ID_INITIAL_CONDITIONS_PARTICLE_FORCE,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "particle_force_cursor.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            // Not used at the moment
            mInitialConditionsParticleForceButton->Enable(false);

            mInitialConditionsParticleForceButton->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent & /*event*/) { OnInitialConditionsButton(mInitialConditionsParticleForceButton); });

            mInitialConditionsParticleForceButton->SetToolTip("Set a force on individual particles");

            gridSizer->Add(mInitialConditionsParticleForceButton);
        }

        vSizer->Add(gridSizer, 0, wxALIGN_CENTER | wxALL, 5);
    }

    vSizer->AddSpacer(10);

    // Action
    {
        wxGridSizer * gridSizer = new wxGridSizer(2, 2, 2);

        // Reset
        {
            auto button = new wxButton(
                this,
                ID_ACTION_RESET,
                wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            button->SetBitmap(
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "reset_icon.png").string(),
                    wxBITMAP_TYPE_PNG));

            button->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent & /*event*/)
                {
                    OnActionResetButton();
                });

            button->SetToolTip("Reset the simulation");

            gridSizer->Add(button);
        }

        // Load
        {
            auto button = new wxButton(
                this,
                ID_ACTION_LOAD_OBJECT,
                wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            button->SetBitmap(
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "open_icon.png").string(),
                    wxBITMAP_TYPE_PNG));

            button->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent & /*event*/)
                {
                    OnActionLoadObjectButton();
                });

            button->SetToolTip("Load a new object");

            gridSizer->Add(button);
        }

        // Settings
        {
            auto button = new wxButton(
                this,
                ID_ACTION_SETTINGS,
                wxEmptyString, wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            button->SetBitmap(
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "settings_icon.png").string(),
                    wxBITMAP_TYPE_PNG));

            button->Bind(
                wxEVT_BUTTON,
                [this](wxCommandEvent & /*event*/)
                {
                    OnActionSettingsButton();
                });

            button->SetToolTip("Adjust the simulation's settings");

            gridSizer->Add(button);
        }

        vSizer->Add(gridSizer, 0, wxALIGN_CENTER | wxALL, 5);
    }

    vSizer->AddSpacer(10);

    // View Control
    {
        wxGridSizer * gridSizer = new wxGridSizer(2, 2, 2);

        // Grid
        {
            mViewControlGridButton = new wxBitmapToggleButton(
                this,
                ID_VIEW_CONTROL_GRID,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "view_grid.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            mViewControlGridButton->Bind(wxEVT_TOGGLEBUTTON, [this](wxCommandEvent & /*event*/) { OnViewControlButton(mViewControlGridButton); });

            mViewControlGridButton->SetToolTip("Enable or disable grid");

            gridSizer->Add(mViewControlGridButton);
        }

        vSizer->Add(gridSizer, 0, wxALIGN_CENTER | wxALL, 5);
    }

    vSizer->AddSpacer(10);

    this->SetSizer(vSizer);
}

ControlToolbar::~ControlToolbar()
{
}

bool ControlToolbar::ProcessKeyDown(
    int keyCode,
    int keyModifiers)
{
    if (keyCode == WXK_SPACE)
    {
        // Pause
        if (!mSimulationControlPauseButton->GetValue())
        {
            mSimulationControlPauseButton->SetFocus();
            mSimulationControlPauseButton->SetValue(true);
            OnSimulationControlButton(mSimulationControlPauseButton);
            return true;
        }
    }
    else if (keyCode == WXK_RETURN)
    {
        // Step
        if (mSimulationControlPauseButton->GetValue())
        {
            OnSimulationControlStepButton();
            return true;
        }
    }
    else if (keyCode == 'M')
    {
        // Move
        if (!mInitialConditionsMoveButton->GetValue())
        {
            mInitialConditionsMoveButton->SetFocus();
            mInitialConditionsMoveButton->SetValue(true);
            OnInitialConditionsButton(mInitialConditionsMoveButton);
            return true;
        }
    }
    else if (keyCode == 'P')
    {
        // Pin
        if (!mInitialConditionsPinButton->GetValue())
        {
            mInitialConditionsPinButton->SetFocus();
            mInitialConditionsPinButton->SetValue(true);
            OnInitialConditionsButton(mInitialConditionsPinButton);
            return true;
        }
    }
    else if (keyCode == 'R' && keyModifiers == wxMOD_CONTROL)
    {
        // Reset
        OnActionResetButton();
        return true;
    }
    else if (keyCode == 'O' && keyModifiers == wxMOD_CONTROL)
    {
        // Load
        OnActionLoadObjectButton();
        return true;
    }
    else if (keyCode == 'S' && keyModifiers == wxMOD_CONTROL)
    {
        // Settings
        OnActionSettingsButton();
        return true;
    }

    return false;
}

void ControlToolbar::OnSimulationControlButton(wxBitmapToggleButton * button)
{
    if (button->GetId() == ID_SIMULATION_CONTROL_PLAY)
    {
        if (button->GetValue())
        {
            // Set all others to off
            mSimulationControlFastPlayButton->SetValue(false);
            mSimulationControlPauseButton->SetValue(false);

            // Disable step
            mSimulationControlStepButton->Enable(false);

            // Fire event
            wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_SIMULATION_CONTROL_PLAY);
            ProcessEvent(evt);
        }
        else
        {
            // Set back to on
            mSimulationControlPlayButton->SetValue(true);
        }
    }
    else if (button->GetId() == ID_SIMULATION_CONTROL_FAST_PLAY)
    {
        if (button->GetValue())
        {
            // Set all others to off
            mSimulationControlPlayButton->SetValue(false);
            mSimulationControlPauseButton->SetValue(false);

            // Disable step
            mSimulationControlStepButton->Enable(false);

            // Fire event
            wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_SIMULATION_CONTROL_FAST_PLAY);
            ProcessEvent(evt);
        }
        else
        {
            // Set back to on
            mSimulationControlFastPlayButton->SetValue(true);
        }
    }
    else
    {
        assert(button->GetId() == ID_SIMULATION_CONTROL_PAUSE);

        if (button->GetValue())
        {
            // Set all others to off
            mSimulationControlPlayButton->SetValue(false);
            mSimulationControlFastPlayButton->SetValue(false);

            // Enable step
            mSimulationControlStepButton->Enable(true);

            // Fire event
            wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_SIMULATION_CONTROL_PAUSE);
            ProcessEvent(evt);
        }
        else
        {
            // Set back to on
            mSimulationControlPauseButton->SetValue(true);
        }
    }
}

void ControlToolbar::OnSimulationControlStepButton()
{
    // Fire event
    wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_SIMULATION_CONTROL_STEP);
    ProcessEvent(evt);
}

void ControlToolbar::OnInitialConditionsButton(wxBitmapToggleButton * button)
{
    if (button->GetId() == ID_INITIAL_CONDITIONS_GRAVITY)
    {
        // Fire event
        wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_INITIAL_CONDITIONS_GRAVITY);
        evt.SetInt(button->GetValue() ? 1 : 0);
        ProcessEvent(evt);
    }
    else if (button->GetId() == ID_INITIAL_CONDITIONS_MOVE
        || button->GetId() == ID_INITIAL_CONDITIONS_PIN
        || button->GetId() == ID_INITIAL_CONDITIONS_PARTICLE_FORCE)
    {
        if (button->GetValue())
        {
            // Turn off all others
            if (button->GetId() != mInitialConditionsMoveButton->GetId())
                mInitialConditionsMoveButton->SetValue(false);
            if (button->GetId() != mInitialConditionsPinButton->GetId())
                mInitialConditionsPinButton->SetValue(false);
            if (button->GetId() != mInitialConditionsParticleForceButton->GetId())
                mInitialConditionsParticleForceButton->SetValue(false);

            // Fire event
            wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, button->GetId());
            ProcessEvent(evt);
        }
        else
        {
            // Turn back on
            button->SetValue(true);
        }
    }
}

void ControlToolbar::OnActionResetButton()
{
    wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_ACTION_RESET);
    ProcessEvent(evt);
}

void ControlToolbar::OnActionLoadObjectButton()
{
    wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_ACTION_LOAD_OBJECT);
    ProcessEvent(evt);
}

void ControlToolbar::OnActionSettingsButton()
{
    wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_ACTION_SETTINGS);
    ProcessEvent(evt);
}

void ControlToolbar::OnSimulatorTypeChoiceChanged()
{
    // Fire event
    wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_SIMULATOR_TYPE);
    evt.SetString(mSimulatorTypeChoice->GetString(mSimulatorTypeChoice->GetSelection()));
    ProcessEvent(evt);
}

void ControlToolbar::OnViewControlButton(wxBitmapToggleButton * button)
{
    if (button->GetId() == ID_VIEW_CONTROL_GRID)
    {
        // Fire event
        wxCommandEvent evt(wxEVT_TOOLBAR_ACTION, ID_VIEW_CONTROL_GRID);
        evt.SetInt(button->GetValue() ? 1 : 0);
        ProcessEvent(evt);
    }
}