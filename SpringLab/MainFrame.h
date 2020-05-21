/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-05-15
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AboutDialog.h"
#include "ControlToolbar.h"
#include "LoggingDialog.h"
#include "ProbeToolbar.h"
#include "ToolController.h"

#include <SLabCoreLib/SimulationController.h>

#include <wx/filedlg.h>
#include <wx/frame.h>
#include <wx/glcanvas.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/timer.h>

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

/*
 * The main window of the game's GUI.
 */
class MainFrame : public wxFrame
{
private:

    static constexpr bool StartInFullScreenMode = true;

public:

    MainFrame(wxApp * mainApp);

    virtual ~MainFrame();

private:

    wxPanel * mMainPanel;
    wxBoxSizer * mMainPanelVSizer;
    wxBoxSizer * mMainPanelTopHSizer;

    //
    // Canvas
    //

    std::unique_ptr<wxGLCanvas> mMainGLCanvas;
    std::unique_ptr<wxGLContext> mMainGLCanvasContext;

    //
    // Controls that we're interacting with
    //

    wxMenuItem * mFullScreenMenuItem;
    wxMenuItem * mNormalScreenMenuItem;

    //
    // Toolbars
    //

    ControlToolbar * mControlToolbar;
    ProbeToolbar * mProbeToolbar;

    //
    // Dialogs
    //

    std::unique_ptr<AboutDialog> mAboutDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;

    //
    // Timers
    //

    std::unique_ptr<wxTimer> mSimulationTimer;

private:

    //
    // Event handlers
    //

    // App
    void OnQuit(wxCommandEvent & event);
    void OnKeyDown(wxKeyEvent & event);

    // Main GL canvas
    void OnMainGLCanvasPaint(wxPaintEvent & event);
    void OnMainGLCanvasResize(wxSizeEvent& event);
    void OnMainGLCanvasLeftDown(wxMouseEvent& event);
    void OnMainGLCanvasLeftUp(wxMouseEvent& event);
    void OnMainGLCanvasRightDown(wxMouseEvent& event);
    void OnMainGLCanvasRightUp(wxMouseEvent& event);
    void OnMainGLCanvasMouseMove(wxMouseEvent& event);
    void OnMainGLCanvasMouseWheel(wxMouseEvent& event);
    void OnMainGLCanvasCaptureMouseLost(wxMouseCaptureLostEvent & event);

    // Menu
    void OnLoadObjectMenuItemSelected(wxCommandEvent & event);
    void OnResetMenuItemSelected(wxCommandEvent & event);
    void OnSaveScreenshotMenuItemSelected(wxCommandEvent & event);
    void OnZoomInMenuItemSelected(wxCommandEvent & event);
    void OnZoomOutMenuItemSelected(wxCommandEvent & event);
    void OnResetViewMenuItemSelected(wxCommandEvent & event);
    void OnOpenLogWindowMenuItemSelected(wxCommandEvent & event);
    void OnFullScreenMenuItemSelected(wxCommandEvent & event);
    void OnNormalScreenMenuItemSelected(wxCommandEvent & event);
    void OnAboutMenuItemSelected(wxCommandEvent & event);

    // Control toolbar
    void OnSimulationControlPlay(wxCommandEvent & event);
    void OnSimulationControlFastPlay(wxCommandEvent & event);
    void OnSimulationControlPause(wxCommandEvent & event);
    void OnSimulationControlStep(wxCommandEvent & event);
    void OnInitialConditionsGravity(wxCommandEvent & event);
    void OnInitialConditionsMove(wxCommandEvent & event);
    void OnInitialConditionsPin(wxCommandEvent & event);
    void OnInitialConditionsParticleForce(wxCommandEvent & event);
    void OnSimulatorTypeChanged(wxCommandEvent & event);

    // Timers
    void OnSimulationTimer(wxTimerEvent & event);

private:

    void OnError(
        std::string const & message,
        bool die);

private:

    wxApp * const mMainApp;

    //
    // Helpers
    //

    std::shared_ptr<SimulationController> mSimulationController;
    std::unique_ptr<ToolController> mToolController;
};
