/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/tglbtn.h>

#include <memory>
#include <string>

class ControlToolbar final : public wxPanel
{
public:

    static wxEventType const wxEVT_TOOLBAR_ACTION;

    static long const ID_SIMULATION_CONTROL_PLAY;
    static long const ID_SIMULATION_CONTROL_FAST_PLAY;
    static long const ID_SIMULATION_CONTROL_PAUSE;
    static long const ID_SIMULATION_CONTROL_STEP;

    static long const ID_INITIAL_CONDITIONS_GRAVITY;
    static long const ID_INITIAL_CONDITIONS_MOVE;
    static long const ID_INITIAL_CONDITIONS_PIN;
    static long const ID_INITIAL_CONDITIONS_PARTICLE_FORCE;

    static long const ID_SIMULATOR_TYPE;

    static long const ID_ACTION_RESET;
    static long const ID_ACTION_LOAD_OBJECT;
    static long const ID_ACTION_SETTINGS;

    static long const ID_VIEW_CONTROL_GRID;

public:

    ControlToolbar(wxWindow* parent);

    virtual ~ControlToolbar();

    bool ProcessKeyDown(
        int keyCode,
        int keyModifiers);

private:

    void OnSimulationControlButton(wxBitmapToggleButton * button);
    void OnSimulationControlStepButton();
    void OnInitialConditionsButton(wxBitmapToggleButton * button);
    void OnActionResetButton();
    void OnActionLoadObjectButton();
    void OnActionSettingsButton();
    void OnSimulatorTypeChoiceChanged();
    void OnViewControlButton(wxBitmapToggleButton * button);

private:

    wxBitmapToggleButton * mSimulationControlPlayButton;
    wxBitmapToggleButton * mSimulationControlFastPlayButton;
    wxBitmapToggleButton * mSimulationControlPauseButton;
    wxButton * mSimulationControlStepButton;

    wxBitmapToggleButton * mInitialConditionsGravityButton;
    wxBitmapToggleButton * mInitialConditionsMoveButton;
    wxBitmapToggleButton * mInitialConditionsPinButton;
    wxBitmapToggleButton * mInitialConditionsParticleForceButton;

    wxBitmapToggleButton * mViewControlGridButton;

    wxChoice * mSimulatorTypeChoice;
};
