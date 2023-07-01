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

long const ID_LOAD_OBJECT_MENUITEM = wxNewId();
long const ID_MAKE_OBJECT_MENUITEM = wxNewId();
long const ID_RESET_MENUITEM = wxNewId();
long const ID_SAVE_SCREENSHOT_MENUITEM = wxNewId();
long const ID_QUIT_MENUITEM = wxNewId();

long const ID_ZOOM_IN_MENUITEM = wxNewId();
long const ID_ZOOM_OUT_MENUITEM = wxNewId();
long const ID_RESET_VIEW_MENUITEM = wxNewId();

long const ID_OPEN_SETTINGS_WINDOW_MENUITEM = wxNewId();
long const ID_RELOAD_LAST_MODIFIED_SETTINGS_MENUITEM = wxNewId();
long const ID_OPEN_LOG_WINDOW_MENUITEM = wxNewId();
long const ID_FULL_SCREEN_MENUITEM = wxNewId();
long const ID_NORMAL_SCREEN_MENUITEM = wxNewId();

long const ID_ABOUT_MENUITEM = wxNewId();

long const ID_SIMULATION_TIMER = wxNewId();

MainFrame::MainFrame(wxApp * mainApp)
    : mIsMouseCapturedByGLCanvas(false)
    , mMainApp(mainApp)
    , mSimulationController()
    , mSettingsManager()
    , mToolController()
    , mSimulationControlState(static_cast<SimulationControlStateType>(0))
    , mSimulationControlImpulse(false)
    , mLastSimulationStepTimestamp(std::chrono::steady_clock::time_point::min())
{
    Create(
        nullptr,
        wxID_ANY,
        std::string(APPLICATION_NAME_WITH_SHORT_VERSION),
        wxDefaultPosition,
        wxDefaultSize,
        wxDEFAULT_FRAME_STYLE | wxMAXIMIZE,
        _T("Main Frame"));

    SetIcon(wxICON(BBB_SLAB_ICON));
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
    Centre();

    Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnMainFrameClose, this);

    mMainPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS);
    mMainPanel->Bind(wxEVT_CHAR_HOOK, &MainFrame::OnKeyDown, this);

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
        wxDefaultSize,
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

    // Take context for this canvas
    mMainGLCanvasContext = std::make_unique<wxGLContext>(mMainGLCanvas.get());

    // Activate context
    mMainGLCanvasContext->SetCurrent(*mMainGLCanvas);


    //
    // Layout panel
    //

    mMainPanelVSizer = new wxBoxSizer(wxVERTICAL);

    // Top
    {
        mMainPanelTopHSizer = new wxBoxSizer(wxHORIZONTAL);

        // Control toolbar
        mControlToolbar = new ControlToolbar(mMainPanel);
        mControlToolbar->Connect(ControlToolbar::ID_SIMULATION_CONTROL_PLAY, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnSimulationControlPlay, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_SIMULATION_CONTROL_FAST_PLAY, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnSimulationControlFastPlay, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_SIMULATION_CONTROL_PAUSE, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnSimulationControlPause, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_SIMULATION_CONTROL_STEP, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnSimulationControlStep, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_INITIAL_CONDITIONS_GRAVITY, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnInitialConditionsGravity, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_INITIAL_CONDITIONS_MOVE, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnInitialConditionsMove, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_INITIAL_CONDITIONS_PIN, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnInitialConditionsPin, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_INITIAL_CONDITIONS_PARTICLE_FORCE, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnInitialConditionsParticleForce, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_SIMULATOR_TYPE, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnSimulatorTypeChanged, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_ACTION_RESET, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnResetMenuItemSelected, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_ACTION_LOAD_OBJECT, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnLoadObjectMenuItemSelected, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_ACTION_SETTINGS, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnOpenSettingsWindowMenuItemSelected, 0, this);
        mControlToolbar->Connect(ControlToolbar::ID_VIEW_CONTROL_GRID, ControlToolbar::wxEVT_TOOLBAR_ACTION, (wxObjectEventFunction)&MainFrame::OnViewControlGridToggled, 0, this);

        mMainPanelTopHSizer->Add(
            mControlToolbar,
            0,                  // Use own horizontal size
            wxEXPAND,           // Expand vertically
            0);                 // Border

        // Canvas
        mMainPanelTopHSizer->Add(
            mMainGLCanvas.get(),
            1,                  // Occupy all available horizontal space
            wxEXPAND,           // Expand also vertically
            0);                 // Border

        mMainPanelVSizer->Add(
            mMainPanelTopHSizer,
            1,                  // Occupy all available vertical space
            wxEXPAND,           // Expand also horizontally
            0);                 // Border
    }

    // Bottom
    {
        // Probe toolbar
        mProbeToolbar = new ProbeToolbar(mMainPanel);

        mMainPanelVSizer->Add(
            mProbeToolbar,
            0,                  // Own height
            wxEXPAND);          // Expand horizontally
    }

    //
    // Build menu
    //

    {
        wxMenuBar * mainMenuBar = new wxMenuBar();


        // File

        wxMenu * fileMenu = new wxMenu();

        wxMenuItem * loadObjectMenuItem = new wxMenuItem(fileMenu, ID_LOAD_OBJECT_MENUITEM, _("Load Object\tCtrl+O"), wxEmptyString, wxITEM_NORMAL);
        fileMenu->Append(loadObjectMenuItem);
        Connect(ID_LOAD_OBJECT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnLoadObjectMenuItemSelected);

        wxMenuItem * makeObjectMenuItem = new wxMenuItem(fileMenu, ID_MAKE_OBJECT_MENUITEM, _("Make Object"), wxEmptyString, wxITEM_NORMAL);
        fileMenu->Append(makeObjectMenuItem);
        Connect(ID_MAKE_OBJECT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnMakeObjectMenuItemSelected);

        wxMenuItem * resetMenuItem = new wxMenuItem(fileMenu, ID_RESET_MENUITEM, _("Reset\tCtrl+R"), wxEmptyString, wxITEM_NORMAL);
        fileMenu->Append(resetMenuItem);
        Connect(ID_RESET_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnResetMenuItemSelected);

        fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

        wxMenuItem * saveScreenshotMenuItem = new wxMenuItem(fileMenu, ID_SAVE_SCREENSHOT_MENUITEM, _("Save Screenshot\tCtrl+C"), wxEmptyString, wxITEM_NORMAL);
        fileMenu->Append(saveScreenshotMenuItem);
        Connect(ID_SAVE_SCREENSHOT_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnSaveScreenshotMenuItemSelected);

        fileMenu->Append(new wxMenuItem(fileMenu, wxID_SEPARATOR));

        wxMenuItem * quitMenuItem = new wxMenuItem(fileMenu, ID_QUIT_MENUITEM, _("Quit\tAlt-F4"), _("Quit the application"), wxITEM_NORMAL);
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


        // Options

        wxMenu * optionsMenu = new wxMenu();

        wxMenuItem * openSettingsWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_SETTINGS_WINDOW_MENUITEM, _("Simulation Settings...\tCtrl+S"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(openSettingsWindowMenuItem);
        Connect(ID_OPEN_SETTINGS_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenSettingsWindowMenuItemSelected);

        mReloadLastModifiedSettingsMenuItem = new wxMenuItem(optionsMenu, ID_RELOAD_LAST_MODIFIED_SETTINGS_MENUITEM, _("Reload Last-Modified Simulation Settings\tCtrl+D"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(mReloadLastModifiedSettingsMenuItem);
        Connect(ID_RELOAD_LAST_MODIFIED_SETTINGS_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnReloadLastModifiedSettingsMenuItem);

        optionsMenu->Append(new wxMenuItem(optionsMenu, wxID_SEPARATOR));

        wxMenuItem * openLogWindowMenuItem = new wxMenuItem(optionsMenu, ID_OPEN_LOG_WINDOW_MENUITEM, _("Open Log Window\tCtrl+L"), wxEmptyString, wxITEM_NORMAL);
        optionsMenu->Append(openLogWindowMenuItem);
        Connect(ID_OPEN_LOG_WINDOW_MENUITEM, wxEVT_COMMAND_MENU_SELECTED, (wxObjectEventFunction)&MainFrame::OnOpenLogWindowMenuItemSelected);

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
    }

    //
    // Finalize frame
    //

    {
        mMainPanel->SetSizer(mMainPanelVSizer);
        mMainPanel->Layout();

        auto * wholeSizer = new wxBoxSizer(wxVERTICAL);
        wholeSizer->Add(mMainPanel, 1, wxEXPAND, 0);
        this->SetSizer(wholeSizer);
    }


    //
    // Initialize tooltips
    //

    wxToolTip::Enable(true);
    wxToolTip::SetDelay(200);


    //
    // PostInitialize
    //

    Show(true);

    if (StartInFullScreenMode)
        ShowFullScreen(true, wxFULLSCREEN_NOBORDER);


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

void MainFrame::OnMainFrameClose(wxCloseEvent & /*event*/)
{
    if (!!mSimulationTimer)
        mSimulationTimer->Stop();

    if (!!mSettingsManager)
        mSettingsManager->SaveLastModifiedSettings();

    Destroy();
}

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
    else if (mControlToolbar->ProcessKeyDown(event.GetKeyCode(), event.GetModifiers()))
    {
        // Processed
    }
    else
    {
        // Keep processing it
        event.Skip();
    }
}

//
// Main canvas event handlers
//

void MainFrame::OnMainGLCanvasPaint(wxPaintEvent & event)
{
    if (mSimulationController)
    {
        mSimulationController->Render();

        assert(mMainGLCanvas);
        mMainGLCanvas->SwapBuffers();
    }

    event.Skip();
}

void MainFrame::OnMainGLCanvasResize(wxSizeEvent & event)
{
    LogMessage("OnMainGLCanvasResize: ", event.GetSize().GetX(), "x", event.GetSize().GetY());

    if (mSimulationController
        && event.GetSize().GetX() > 0
        && event.GetSize().GetY() > 0)
    {
        mSimulationController->SetCanvasSize(
            event.GetSize().GetX(),
            event.GetSize().GetY());
    }

    if (mProbeToolbar)
    {
        mProbeToolbar->Refresh();
    }

    event.Skip();
}

void MainFrame::OnMainGLCanvasLeftDown(wxMouseEvent & /*event*/)
{
    if (mToolController)
    {
        mToolController->OnLeftMouseDown();
    }

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->CaptureMouse();
        mIsMouseCapturedByGLCanvas = true;
    }
}

void MainFrame::OnMainGLCanvasLeftUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->ReleaseMouse();
        mIsMouseCapturedByGLCanvas = false;
    }

    if (mToolController)
    {
        mToolController->OnLeftMouseUp();
    }
}

void MainFrame::OnMainGLCanvasRightDown(wxMouseEvent & /*event*/)
{
    if (mToolController)
    {
        mToolController->OnRightMouseDown();
    }

    // Hang on to the mouse for as long as the button is pressed
    if (!mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->CaptureMouse();
        mIsMouseCapturedByGLCanvas = true;
    }
}

void MainFrame::OnMainGLCanvasRightUp(wxMouseEvent & /*event*/)
{
    // We can now release the mouse
    if (mIsMouseCapturedByGLCanvas)
    {
        mMainGLCanvas->ReleaseMouse();
        mIsMouseCapturedByGLCanvas = false;
    }

    if (mToolController)
    {
        mToolController->OnRightMouseUp();
    }
}

void MainFrame::OnMainGLCanvasMouseMove(wxMouseEvent & event)
{
    if (mToolController)
    {
        mToolController->OnMouseMove(event.GetX(), event.GetY());
    }
}

void MainFrame::OnMainGLCanvasMouseWheel(wxMouseEvent & event)
{
    if (mSimulationController)
    {
        mSimulationController->AdjustZoom(powf(1.002f, event.GetWheelRotation()));
    }
}

void MainFrame::OnMainGLCanvasCaptureMouseLost(wxMouseCaptureLostEvent & /*event*/)
{
    if (mToolController)
    {
        mToolController->UnsetTool();
    }
}

//
// Menu event handlers
//

void MainFrame::OnLoadObjectMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mFileOpenDialog)
    {
        mFileOpenDialog = std::make_unique<wxFileDialog>(
            this,
            L"Select Object",
            wxEmptyString,
            wxEmptyString,
            L"Object files (*.png)|*.png",
            wxFD_OPEN | wxFD_FILE_MUST_EXIST,
            wxDefaultPosition,
            wxDefaultSize,
            _T("File Open Dialog"));
    }

    assert(!!mFileOpenDialog);

    if (mFileOpenDialog->ShowModal() == wxID_OK)
    {
        std::string const filepath = mFileOpenDialog->GetPath().ToStdString();

        assert(!!mSimulationController);
        try
        {
            mSimulationController->LoadObject(filepath);
        }
        catch (std::exception const & ex)
        {
            OnError(ex.what(), false);
        }
    }
}

void MainFrame::OnMakeObjectMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mSyntheticObjectDialog)
    {
        mSyntheticObjectDialog = std::make_unique<SyntheticObjectDialog>(
            this);
    }

    auto numSprings = mSyntheticObjectDialog->AskNumSprings();
    if (numSprings)
    {
        try
        {
            mSimulationController->MakeObject(*numSprings);
        }
        catch (std::exception const & ex)
        {
            OnError(ex.what(), false);
        }
    }
}

void MainFrame::OnResetMenuItemSelected(wxCommandEvent & /*event*/)
{
    assert(!!mSimulationController);
    mSimulationController->Reset();
}

void MainFrame::OnSaveScreenshotMenuItemSelected(wxCommandEvent & /*event*/)
{
    //
    // Take screenshot
    //

    assert(!!mSimulationController);
    auto screenshotImage = mSimulationController->TakeScreenshot();

    //
    // Ensure pictures folder exists
    //

    auto const folderPath = StandardSystemPaths::GetInstance().GetUserPicturesSimulatorFolderPath();

    if (!std::filesystem::exists(folderPath))
    {
        try
        {
            std::filesystem::create_directories(folderPath);
        }
        catch (std::filesystem::filesystem_error const & fex)
        {
            OnError(
                std::string("Could not save screenshot to path \"") + folderPath.string() + "\": " + fex.what(),
                false);

            return;
        }
    }


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

        screenshotFilePath = folderPath / std::filesystem::path(ssFilename.str());

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

void MainFrame::OnOpenSettingsWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mSettingsDialog)
    {
        mSettingsDialog = std::make_unique<SettingsDialog>(
            this,
            mSettingsManager,
            mSimulationController);
    }

    mSettingsDialog->Open();
}

void MainFrame::OnReloadLastModifiedSettingsMenuItem(wxCommandEvent & /*event*/)
{
    // Load last-modified settings
    try
    {
        assert(!!mSettingsManager);
        mSettingsManager->EnforceDefaultsAndLastModifiedSettings();
    }
    catch (std::exception const & exc)
    {
        OnError("Could not load last-modified settings: " + std::string(exc.what()), false);

        // Disable menu item
        mReloadLastModifiedSettingsMenuItem->Enable(false);
    }
}

void MainFrame::OnOpenLogWindowMenuItemSelected(wxCommandEvent & /*event*/)
{
    if (!mLoggingDialog)
    {
        mLoggingDialog = std::make_unique<LoggingDialog>(this);
    }

    mLoggingDialog->Open();
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

void MainFrame::OnSimulationControlPlay(wxCommandEvent & /*event*/)
{
    mSimulationControlState = SimulationControlStateType::SlowPlay;
}

void MainFrame::OnSimulationControlFastPlay(wxCommandEvent & /*event*/)
{
    mSimulationControlState = SimulationControlStateType::FastPlay;
}

void MainFrame::OnSimulationControlPause(wxCommandEvent & /*event*/)
{
    mSimulationControlState = SimulationControlStateType::Paused;
}

void MainFrame::OnSimulationControlStep(wxCommandEvent & /*event*/)
{
    mSimulationControlImpulse = true;
}

void MainFrame::OnInitialConditionsGravity(wxCommandEvent & event)
{
    assert(!!mSimulationController);
    mSimulationController->SetCommonDoApplyGravity(event.GetInt() != 0);
}

void MainFrame::OnInitialConditionsMove(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::MoveSimple);
}

void MainFrame::OnInitialConditionsPin(wxCommandEvent & /*event*/)
{
    assert(!!mToolController);
    mToolController->SetTool(ToolType::Pin);
}

void MainFrame::OnInitialConditionsParticleForce(wxCommandEvent & /*event*/)
{
    LogMessage("TODO: OnInitialConditionsParticleForce");
}

void MainFrame::OnSimulatorTypeChanged(wxCommandEvent & event)
{
    assert(!!mSimulationController);
    mSimulationController->SetSimulator(event.GetString().ToStdString());
}

void MainFrame::OnViewControlGridToggled(wxCommandEvent & event)
{
    assert(!!mSimulationController);
    mSimulationController->SetViewGridEnabled(event.GetInt() != 0);
}

void MainFrame::OnSimulationTimer(wxTimerEvent & /*event*/)
{
    //
    // Complete initialization, if still not done
    //
    // We do it here to make sure we get the final canvas size
    //

    if (!mSimulationController)
    {
        try
        {
            FinishInitialization();
        }
        catch (SLabException const & e)
        {
            mSimulationTimer->Stop(); // Stop looping and allow Die() to finish

            OnError(std::string(e.what()), true);            
            return;
        }
    }

    //
    // Update tools
    //

    assert(!!mToolController);

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

    mToolController->Update();


    //
    // Update
    //

    assert(!!mSimulationController);

    auto constexpr SlowPlayInterval = std::chrono::milliseconds(500);

    if (auto const now = std::chrono::steady_clock::now();
        mSimulationControlState == SimulationControlStateType::FastPlay
        || (mSimulationControlState == SimulationControlStateType::SlowPlay && now >= mLastSimulationStepTimestamp + SlowPlayInterval)
        || mSimulationControlImpulse)
    {
        mSimulationController->UpdateSimulation();

        // Update state
        mSimulationControlImpulse = false;
        mLastSimulationStepTimestamp = now;
    }


    //
    // Render
    //

    assert(!!mMainGLCanvas);

    mMainGLCanvas->Refresh();


    //
    // Update probe toolbar
    //

    assert(!!mProbeToolbar);

    mProbeToolbar->UpdateSimulation();
}

/////////////////////////////////////////////////////////////////////////////////////////////////

void MainFrame::FinishInitialization()
{
    //
    // Create Simulation Controller
    //

    try
    {
        mSimulationController = SimulationController::Create(
            mMainGLCanvas->GetSize().x,
            mMainGLCanvas->GetSize().y);
    }
    catch (std::exception const & e)
    {
        throw SLabException("Error during initialization of simulation controller: " + std::string(e.what()));
    }

    //
    // Create Settings Manager
    //

    mSettingsManager = std::make_shared<SettingsManager>(
        mSimulationController,
        StandardSystemPaths::GetInstance().GetUserSimulatorSettingsRootFolderPath());

    // Enable "Reload Last Modified Settings" menu if we have last-modified settings
    mReloadLastModifiedSettingsMenuItem->Enable(mSettingsManager->HasLastModifiedSettingsPersisted());

    //
    // Create Tool Controller
    //

    try
    {
        mToolController = std::make_unique<ToolController>(
            ToolType::MoveSimple,
            mMainGLCanvas.get(),
            mSimulationController);
    }
    catch (std::exception const & e)
    {
        throw SLabException("Error during initialization of tool controller: " + std::string(e.what()));
    }

    //
    // Register event handlers
    //

    mSimulationController->RegisterEventHandler(mProbeToolbar);

    //
    // Load initial object
    //

    mSimulationController->LoadObject(ResourceLocator::GetDefaultObjectDefinitionFilePath());
}

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