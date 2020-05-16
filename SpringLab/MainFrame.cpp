/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-05-15
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "MainFrame.h"

#include "StandardSystemPaths.h"

#include <SLabCoreLib/ImageFileTools.h>
#include <SLabCoreLib/SLabException.h>
#include <SLabCoreLib/SLabOpenGL.h>
#include <SLabCoreLib/Log.h>
#include <SLabCoreLib/Utils.h>
#include <SLabCoreLib/Version.h>

#include <wx/intl.h>
#include <wx/msgdlg.h>
#include <wx/panel.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/string.h>
#include <wx/tooltip.h>

#include <cassert>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <map>
#include <sstream>
#include <thread>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/SLabBBB.xpm"
#endif

long const ID_MAIN_CANVAS = wxNewId();

long const ID_SAVE_SCREENSHOT_MENUITEM = wxNewId();
long const ID_QUIT_MENUITEM = wxNewId();

long const ID_ZOOM_IN_MENUITEM = wxNewId();
long const ID_ZOOM_OUT_MENUITEM = wxNewId();
long const ID_RESET_VIEW_MENUITEM = wxNewId();

long const ID_TOOL_MOVE_MENUITEM = wxNewId();

long const ID_OPEN_LOG_WINDOW_MENUITEM = wxNewId();
long const ID_SHOW_PROBE_PANEL_MENUITEM = wxNewId();
long const ID_FULL_SCREEN_MENUITEM = wxNewId();
long const ID_NORMAL_SCREEN_MENUITEM = wxNewId();

long const ID_ABOUT_MENUITEM = wxNewId();

long const ID_SIMULATION_TIMER = wxNewId();

MainFrame::MainFrame(wxApp * mainApp)
    : mMainApp(mainApp)
    , mSimulationController()
{
    Create(
        nullptr,
        wxID_ANY,
        std::string(APPLICATION_NAME_WITH_SHORT_VERSION),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_FRAME_STYLE,
        _T("Main Frame"));

    SetIcon(wxICON(BBB_SLAB_ICON));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Maximize();
    Centre();

    mMainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(-1, -1), wxWANTS_CHARS);
    mMainPanel->Bind(wxEVT_CHAR_HOOK, &MainFrame::OnKeyDown, this);
    mMainFrameSizer = new wxBoxSizer(wxVERTICAL);


    //
    // Build main GL canvas and activate GL context
    //

    // Note: Using the wxWidgets 3.1 style does not work on OpenGL 4 drivers; it forces a 1.1.0 context

    int mainGLCanvasAttributes[] =
    {
        WX_GL_RGBA,
        WX_GL_DOUBLEBUFFER,
        WX_GL_DEPTH_SIZE,      16,
        WX_GL_STENCIL_SIZE,    1,

        // Cannot specify CORE_PROFILE or else wx tries OpenGL 3.0 and fails if it's not supported
        //WX_GL_CORE_PROFILE,

        // Useless to specify version as Glad will always take the max
        //WX_GL_MAJOR_VERSION,    GameOpenGL::MinOpenGLVersionMaj,
        //WX_GL_MINOR_VERSION,    GameOpenGL::MinOpenGLVersionMin,

        0, 0
    };

    mMainGLCanvas = std::make_unique<wxGLCanvas>(
        mMainPanel,
        ID_MAIN_CANVAS,
        mainGLCanvasAttributes,
        wxDefaultPosition,
        wxSize(1, 1),
        0L,
        _T("Main GL Canvas"));

    mMainGLCanvas->Connect(wxEVT_PAINT, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasPaint, 0, this);
    mMainGLCanvas->Connect(wxEVT_SIZE, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasResize, 0, this);
    mMainGLCanvas->Connect(wxEVT_LEFT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasLeftDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_LEFT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasLeftUp, 0, this);
    mMainGLCanvas->Connect(wxEVT_RIGHT_DOWN, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasRightDown, 0, this);
    mMainGLCanvas->Connect(wxEVT_RIGHT_UP, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasRightUp, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOTION, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseMove, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOUSEWHEEL, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasMouseWheel, 0, this);
    mMainGLCanvas->Connect(wxEVT_MOUSE_CAPTURE_LOST, (wxObjectEventFunction)&MainFrame::OnMainGLCanvasCaptureMouseLost, 0, this);

    mMainFrameSizer->Add(
        mMainGLCanvas.get(),
        1,                  // Occupy all available vertical space
        wxEXPAND,           // Expand also horizontally
        0);                 // Border

    // Take context for this canvas
    mMainGLCanvasContext = std::make_unique<wxGLContext>(mMainGLCanvas.get());


    //
    // Build menu
    //

    wxMenuBar * mainMenuBar = new wxMenuBar();


    // File

    wxMenu * fileMenu = new wxMenu();

    wxMenuItem * saveScreenshotMenuItem = new wxMenuItem(fileMenu, ID_SAVE_SCREENSHOT_MENUITEM, _("Save Screenshot\tCtrl+C"), wxEmptyString, wxITEM_NORMAL);
    fileMenu->Append(saveScreenshotMenuItem);
    Connect(ID_SAVE_SCREENSHOT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveScreenshotMenuItemSelected);

    fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

    wxMenuItem* quitMenuItem = new wxMenuItem(fileMenu, ID_QUIT_MENUITEM, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
    fileMenu->Append(quitMenuItem);
    Connect(ID_QUIT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnQuit);

    mainMenuBar->Append(fileMenu, _("&File"));


    // Controls

    wxMenu * controlsMenu = new wxMenu();

    wxMenuItem * zoomInMenuItem = new wxMenuItem(controlsMenu, ID_ZOOM_IN_MENUITEM, _("Zoom In\t+"), wxEmptyString, wxITEM_NORMAL);
    controlsMenu->Append(zoomInMenuItem);
    Connect(ID_ZOOM_IN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomInMenuItemSelected);

    wxMenuItem * zoomOutMenuItem = new wxMenuItem(controlsMenu, ID_ZOOM_OUT_MENUITEM, _("Zoom Out\t-"), wxEmptyString, wxITEM_NORMAL);
    controlsMenu->Append(zoomOutMenuItem);
    Connect(ID_ZOOM_OUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnZoomOutMenuItemSelected);

    controlsMenu->Append(new wxMenuItem(controlsMenu, wxID_SEPARATOR));

    wxMenuItem * resetViewMenuItem = new wxMenuItem(controlsMenu, ID_RESET_VIEW_MENUITEM, _("Reset View\tHOME"), wxEmptyString, wxITEM_NORMAL);
    controlsMenu->Append(resetViewMenuItem);
    Connect(ID_RESET_VIEW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnResetViewMenuItemSelected);

    mainMenuBar->Append(controlsMenu, _("Controls"));


    // Tools

    mToolsMenu = new wxMenu();

    wxMenuItem * toolMoveMenuItem = new wxMenuItem(mToolsMenu, ID_TOOL_MOVE_MENUITEM, _("Move\tM"), wxEmptyString, wxITEM_RADIO);
    mToolsMenu->Append(toolMoveMenuItem);
    Connect(ID_TOOL_MOVE_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnToolMoveMenuItemSelected);

    mainMenuBar->Append(mToolsMenu, _("Tools"));


    // Options

    wxMenu * optionsMenu = new wxMenu();

    wxMenuItem * openLogWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_LOG_WINDOW_MENUITEM, _("Open Log Window\tCtrl+L"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(openLogWindowMenuItem);
    Connect(ID_OPEN_LOG_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenLogWindowMenuItemSelected);

    mShowProbePanelMenuItem = new wxMenuItem(optionsMenu, ID_SHOW_PROBE_PANEL_MENUITEM, _("Show Probe Panel\tCtrl+P"), wxEmptyString, wxITEM_CHECK);
    optionsMenu->Append(mShowProbePanelMenuItem);
    mShowProbePanelMenuItem->Check(false);
    Connect(ID_SHOW_PROBE_PANEL_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnShowProbePanelMenuItemSelected);

    optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

    mFullScreenMenuItem = new wxMenuItem(optionsMenu, ID_FULL_SCREEN_MENUITEM, _("Full Screen\tF11"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(mFullScreenMenuItem);
    Connect(ID_FULL_SCREEN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnFullScreenMenuItemSelected);
    mFullScreenMenuItem->Enable(!StartInFullScreenMode);

    mNormalScreenMenuItem = new wxMenuItem(optionsMenu, ID_NORMAL_SCREEN_MENUITEM, _("Normal Screen\tESC"), wxEmptyString, wxITEM_NORMAL);
    optionsMenu->Append(mNormalScreenMenuItem);
    Connect(ID_NORMAL_SCREEN_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnNormalScreenMenuItemSelected);
    mNormalScreenMenuItem->Enable(StartInFullScreenMode);

    mainMenuBar->Append(optionsMenu, _("Options"));


    // Help

    wxMenu * helpMenu = new wxMenu();

    wxMenuItem * aboutMenuItem = new wxMenuItem(helpMenu, ID_ABOUT_MENUITEM, _("About\tF2"), _("Show credits and other I'vedunnit stuff"), wxITEM_NORMAL);
    helpMenu->Append(aboutMenuItem);
    Connect(ID_ABOUT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnAboutMenuItemSelected);

    mainMenuBar->Append(helpMenu, _("Help"));

    SetMenuBar(mainMenuBar);


    //
    // Probe panel
    //

    mProbePanel = std::make_unique<ProbePanel>(mMainPanel);

    mMainFrameSizer->Add(mProbePanel.get(), 0, wxEXPAND); // Expand horizontally

    mMainFrameSizer->Hide(mProbePanel.get());


    //
    // Finalize frame
    //

    mMainPanel->SetSizer(mMainFrameSizer);


    //
    // Initialize tooltips
    //

    wxToolTip::Enable(true);
    wxToolTip::SetDelay(200);


    //
    // Create Simulation Controller
    //

    try
    {
        mSimulationController = SimulationController::Create(
            [this]()
            {
                // Activate context
                mMainGLCanvasContext->SetCurrent(*mMainGLCanvas);
            },
            [this]()
            {
                assert(!!mMainGLCanvas);
                mMainGLCanvas->SwapBuffers();
            });
    }
    catch (std::exception const & e)
    {
        OnError("Error during initialization of simulation controller: " + std::string(e.what()), true);

        return;
    }

    //
    // Create Tool Controller
    //

    // Set initial tool
    ToolType initialToolType = ToolType::Move;
    mToolsMenu->Check(ID_TOOL_MOVE_MENUITEM, true);

    try
    {
        mToolController = std::make_unique<ToolController>(
            initialToolType,
            this,
            mSimulationController);
    }
    catch (std::exception const & e)
    {
        OnError("Error during initialization of tool controller: " + std::string(e.what()), true);

        return;
    }

    this->mMainApp->Yield();

    //
    // Register event handlers
    //

    mSimulationController->RegisterEventHandler(mProbePanel.get());


    //
    // PostInitialize
    //

    this->Show(true);

    if (StartInFullScreenMode)
        this->ShowFullScreen(true, wxFULLSCREEN_NOBORDER);

    //
    // Initialize timers
    //

    mSimulationTimer = std::make_unique<wxTimer>(this, ID_SIMULATION_TIMER);
    Connect(ID_SIMULATION_TIMER, wxEVT_TIMER, (wxObjectEventFunction)&MainFrame::OnSimulationTimer);
    mSimulationTimer->Start(0);
}

MainFrame::~MainFrame()
{
}

//
// App event handlers
//

void MainFrame::OnQuit(wxCommandEvent & /*event*/)
{
    Close();
}

void MainFrame::OnKeyDown(wxKeyEvent & event)
{
    assert(!!mSimulationController);

    if (event.GetKeyCode() == WXK_LEFT)
    {
        // Left
        mSimulationController->Pan(vec2f(-20.0, 0.0f));
    }
    else if (event.GetKeyCode() == WXK_UP)
    {
        // Up
        mSimulationController->Pan(vec2f(00.0f, -20.0f));
    }
    else if (event.GetKeyCode() == WXK_RIGHT)
    {
        // Right
        mSimulationController->Pan(vec2f(20.0f, 0.0f));
    }
    else if (event.GetKeyCode() == WXK_DOWN)
    {
        // Down
        mSimulationController->Pan(vec2f(0.0f, 20.0f));
    }
    else if (event.GetKeyCode() == '/')
    {
        assert(!!mToolController);

        // Query

        vec2f screenCoords = mToolController->GetMouseScreenCoordinates();
        vec2f worldCoords = mSimulationController->ScreenToWorld(screenCoords);

        LogMessage(worldCoords.toString(), ":");

        mSimulationController->QueryNearestPointAt(screenCoords);
    }

    event.Skip();
}

/* TODO
void MainFrame::OnGameTimerTrigger(wxTimerEvent & event)
{
    // Update SHIFT key state
    if (wxGetKeyState(WXK_SHIFT))
    {
        if (!mIsShiftKeyDown)
        {
            mIsShiftKeyDown = true;
            mToolController->OnShiftKeyDown();
        }
    }
    else
    {
        if (mIsShiftKeyDown)
        {
            mIsShiftKeyDown = false;
            mToolController->OnShiftKeyUp();
        }
    }


    //
    // Run a game step
    //

    try
    {
        // Update game - will also render
        assert(!!mGameController);
        mGameController->RunGameIteration();

        // Update probe panel
        assert(!!mProbePanel);
        mProbePanel->UpdateSimulation();

        // Do after-render chores
        AfterGameRender();
    }
    catch (std::exception const & e)
    {
        OnError("Error during simulation step: " + std::string(e.what()), true);

        return;
    }
}
*/

//
// Main canvas event handlers
//

void MainFrame::OnMainGLCanvasPaint(wxPaintEvent & event)
{
    if (!!mSimulationController)
    {
        mSimulationController->Render();
    }

    event.Skip();
}

void MainFrame::OnMainGLCanvasResize(wxSizeEvent & event)
{
    LogMessage("OnMainGLCanvasResize: ", event.GetSize().GetX(), "x", event.GetSize().GetY());

    if (!!mSimulationController
        && event.GetSize().GetX() > 0
        && event.GetSize().GetY() > 0)
    {
        mSimulationController->SetCanvasSize(
            event.GetSize().GetX(),
            event.GetSize().GetY());
    }
}

void MainFrame::OnMainGLCanvasLeftDown(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnLeftMouseDown();
}

void MainFrame::OnMainGLCanvasLeftUp(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnLeftMouseUp();
}

void MainFrame::OnMainGLCanvasRightDown(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnRightMouseDown();
}

void MainFrame::OnMainGLCanvasRightUp(wxMouseEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->OnRightMouseUp();
}

void MainFrame::OnMainGLCanvasMouseMove(wxMouseEvent & event)
{
    assert(!!mToolController);
    mToolController->OnMouseMove(event.GetX(), event.GetY());
}

void MainFrame::OnMainGLCanvasMouseWheel(wxMouseEvent & event)
{
    assert(!!mSimulationController);
    mSimulationController->AdjustZoom(powf(1.002f, event.GetWheelRotation()));
}

void MainFrame::OnMainGLCanvasCaptureMouseLost(wxMouseCaptureLostEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->UnsetTool();
}

//
// Menu event handlers
//

void MainFrame::OnSaveScreenshotMenuItemSelected(wxCommandEvent & /*event*/)
{
    //
    // Take screenshot
    //

    assert(!!mSimulationController);
    auto screenshotImage = mSimulationController->TakeScreenshot();


    //
    // Choose filename
    //

    std::filesystem::path screenshotFilePath;

    do
    {
        auto now = std::chrono::system_clock::now();
        auto now_time_t = std::chrono::system_clock::to_time_t(now);
        auto const tm = std::localtime(&now_time_t);

        std::stringstream ssFilename;
        ssFilename.fill('0');
        ssFilename
            << std::setw(4) << (1900 + tm->tm_year) << std::setw(2) << (1 + tm->tm_mon) << std::setw(2) << tm->tm_mday
            << "_"
            << std::setw(2) << tm->tm_hour << std::setw(2) << tm->tm_min << std::setw(2) << tm->tm_sec
            << "_"
            << std::setw(3) << std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch() % std::chrono::seconds(1)).count()
            << "_"
            << "SpringLab.png";

        screenshotFilePath =
            StandardSystemPaths::GetInstance().GetUserPicturesGameFolderPath()
            / std::filesystem::path(ssFilename.str());

    } while (std::filesystem::exists(screenshotFilePath));


    //
    // Save screenshot
    //

    try
    {
        ImageFileTools::SaveImage(
            screenshotFilePath,
            screenshotImage);
    }
    catch (std::filesystem::filesystem_error const & fex)
    {
        OnError(
            std::string("Could not save screenshot to file \"") + screenshotFilePath.string() + "\": " + fex.what(),
            false);
    }
}

void MainFrame::OnResetViewMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mSimulationController);

    mSimulationController->ResetPan();
    mSimulationController->ResetZoom();
}

void MainFrame::OnZoomInMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mSimulationController);
    mSimulationController->AdjustZoom(1.05f);
}

void MainFrame::OnZoomOutMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mSimulationController);
    mSimulationController->AdjustZoom(1.0f / 1.05f);
}

////////////////////////////////////////////////////////////////////////////

void MainFrame::OnToolMoveMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Move);
}

////////////////////////////////////////////////////////////////////////////

void MainFrame::OnOpenLogWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mLoggingDialog)
    {
        mLoggingDialog = std::make_unique<LoggingDialog>(this);
    }

    mLoggingDialog->Open();
}

void MainFrame::OnShowProbePanelMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mProbePanel);

    if (mShowProbePanelMenuItem->IsChecked())
    {
        mMainFrameSizer->Show(mProbePanel.get());
    }
    else
    {
        mMainFrameSizer->Hide(mProbePanel.get());
    }

    mMainFrameSizer->Layout();
}

void MainFrame::OnFullScreenMenuItemSelected(wxCommandEvent & /*event*/)
{
    mFullScreenMenuItem->Enable(false);
    mNormalScreenMenuItem->Enable(true);

    this->ShowFullScreen(true, wxFULLSCREEN_NOBORDER);
}

void MainFrame::OnNormalScreenMenuItemSelected(wxCommandEvent & /*event*/)
{
    mFullScreenMenuItem->Enable(true);
    mNormalScreenMenuItem->Enable(false);

    this->ShowFullScreen(false);
}

void MainFrame::OnAboutMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mAboutDialog)
    {
        mAboutDialog = std::make_unique<AboutDialog>(this);
    }

    mAboutDialog->Open();
}

void MainFrame::OnSimulationTimer(wxTimerEvent & /*event*/)
{
    assert(!!mToolController);

    // Update tools's SHIFT state
    if (wxGetKeyState(WXK_SHIFT))
    {
        if (!mToolController->IsShiftKeyDown())
            mToolController->OnShiftKeyDown();
    }
    else
    {
        if (mToolController->IsShiftKeyDown())
            mToolController->OnShiftKeyUp();
    }

    // Update tools
    mToolController->Update();

    // Render
    mMainGLCanvas->Refresh();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void MainFrame::OnError(
    std::string const & message,
    bool die)
{
    //
    // Show message
    //

    wxMessageBox(message, wxT("Simulation Disaster"), wxICON_ERROR);

    if (die)
    {
        //
        // Exit
        //

        this->Destroy();
    }
}