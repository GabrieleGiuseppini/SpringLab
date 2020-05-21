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
#include <wx/button.h>
#include <wx/gbsizer.h>
#include <wx/sizer.h>
#include <wx/tglbtn.h>

#include <cassert>

long const ControlToolbar::ID_SIMULATION_CONTROL_PLAY = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_FAST_PLAY = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_PAUSE = wxNewId();
long const ControlToolbar::ID_SIMULATION_CONTROL_STEP = wxNewId();

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
        wxGridSizer * gridSizer = new wxGridSizer(2, 0, 0);

        // Play
        {
            wxBitmapToggleButton * button = new wxBitmapToggleButton(
                this,
                ID_SIMULATION_CONTROL_PLAY,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "simcontrol_play.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            button->Bind(
                wxEVT_TOGGLEBUTTON,
                [this](wxCommandEvent & /*event*/)
                {
                    wxCommandEvent evt(wxEVT_BUTTON, ID_SIMULATION_CONTROL_PLAY);
                    ProcessEvent(evt);
                });

            button->SetToolTip("Starts simulation auto-play step-by-step");

            gridSizer->Add(button);
        }

        // Fast Play
        {
            wxBitmapToggleButton * button = new wxBitmapToggleButton(
                this,
                ID_SIMULATION_CONTROL_FAST_PLAY,
                wxBitmap(
                    (ResourceLocator::GetResourcesFolderPath() / "simcontrol_play_fast.png").string(),
                    wxBITMAP_TYPE_PNG),
                wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

            button->Bind(
                wxEVT_TOGGLEBUTTON,
                [this](wxCommandEvent & /*event*/)
                {
                    wxCommandEvent evt(wxEVT_BUTTON, ID_SIMULATION_CONTROL_FAST_PLAY);
                    ProcessEvent(evt);
                });

            button->SetToolTip("Starts simulation auto-play as fast as possible");

            gridSizer->Add(button);
        }

        vSizer->Add(gridSizer, 0, wxALIGN_CENTER, 0);
    }

    // TODO: others

    this->SetSizer(vSizer);
}

ControlToolbar::~ControlToolbar()
{
}