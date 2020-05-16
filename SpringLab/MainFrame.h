/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-05-15
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "AboutDialog.h"
#include "LoggingDialog.h"
#include "ProbePanel.h"

#include <SLabCoreLib/SimulationController.h>

#include <wx/filedlg.h>
#include <wx/frame.h>
#include <wx/glcanvas.h>
#include <wx/menu.h>
#include <wx/sizer.h>

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

    //
    // Canvas
    //

    std::unique_ptr<wxGLCanvas> mMainGLCanvas;
    std::unique_ptr<wxGLContext> mMainGLCanvasContext;

    //
    // Controls that we're interacting with
    //

    wxBoxSizer * mMainFrameSizer;
    wxMenu * mToolsMenu;
    wxMenuItem * mShowProbePanelMenuItem;
    wxMenuItem * mFullScreenMenuItem;
    wxMenuItem * mNormalScreenMenuItem;
    std::unique_ptr<ProbePanel> mProbePanel;


    //
    // Dialogs
    //

    std::unique_ptr<AboutDialog> mAboutDialog;
    std::unique_ptr<LoggingDialog> mLoggingDialog;

private:

    //
    // Event handlers
    //

    // App
    void OnQuit(wxCommandEvent & event);
    void OnPaint(wxPaintEvent & event);
    void OnKeyDown(wxKeyEvent & event);

    // Main GL canvas
    void OnMainGLCanvasResize(wxSizeEvent& event);
    void OnMainGLCanvasLeftDown(wxMouseEvent& event);
    void OnMainGLCanvasLeftUp(wxMouseEvent& event);
    void OnMainGLCanvasRightDown(wxMouseEvent& event);
    void OnMainGLCanvasRightUp(wxMouseEvent& event);
    void OnMainGLCanvasMouseMove(wxMouseEvent& event);
    void OnMainGLCanvasMouseWheel(wxMouseEvent& event);

    // Menu
    void OnZoomInMenuItemSelected(wxCommandEvent & event);
    void OnZoomOutMenuItemSelected(wxCommandEvent & event);
    void OnResetViewMenuItemSelected(wxCommandEvent & event);
    void OnSaveScreenshotMenuItemSelected(wxCommandEvent & event);

    void OnToolMoveMenuItemSelected(wxCommandEvent & event);

    void OnOpenLogWindowMenuItemSelected(wxCommandEvent & event);
    void OnShowProbePanelMenuItemSelected(wxCommandEvent & event);
    void OnShowStatusTextMenuItemSelected(wxCommandEvent & event);
    void OnFullScreenMenuItemSelected(wxCommandEvent & event);
    void OnNormalScreenMenuItemSelected(wxCommandEvent & event);
    void OnAboutMenuItemSelected(wxCommandEvent & event);

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


    //
    // State
    //

    bool mIsShiftKeyDown;
};
