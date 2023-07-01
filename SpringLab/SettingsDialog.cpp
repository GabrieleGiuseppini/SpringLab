/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-05-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "SettingsDialog.h"

#include "WxHelpers.h"

#include "UIControls/ExponentialSliderCore.h"
#include "UIControls/FixedTickSliderCore.h"
#include "UIControls/IntegralLinearSliderCore.h"
#include "UIControls/LinearSliderCore.h"

#include <SLabCoreLib/Log.h>

#include <wx/gbsizer.h>
#include <wx/intl.h>
#include <wx/notebook.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/string.h>

#include <algorithm>
#include <stdexcept>

#ifdef _MSC_VER
 // Nothing to do here - we use RC files
#else
#include "Resources/SLabBBB.xpm"
#endif

static int constexpr SliderWidth = 40;
static int constexpr SliderHeight = 140;

static int constexpr StaticBoxTopMargin = 7;
static int constexpr StaticBoxInsetMargin = 10;
static int constexpr CellBorder = 8;

SettingsDialog::SettingsDialog(
    wxWindow * parent,
    std::shared_ptr<SettingsManager> settingsManager,
    std::shared_ptr<SimulationController> simulationController)
    : mParent(parent)
    , mSettingsManager(std::move(settingsManager))
	, mSimulationController(std::move(simulationController))
    // State
    , mLiveSettings(mSettingsManager->MakeSettings())
    , mCheckpointSettings(mSettingsManager->MakeSettings())
{
    Create(
        mParent,
        wxID_ANY,
        _("Simulation Settings"),
        wxDefaultPosition,
        wxSize(400, 200),
        wxCAPTION | wxCLOSE_BOX | wxMINIMIZE_BOX | wxFRAME_NO_TASKBAR
			| /* wxFRAME_FLOAT_ON_PARENT */ wxSTAY_ON_TOP, // See https://trac.wxwidgets.org/ticket/18535
        _T("Settings Window"));

    this->Bind(wxEVT_CLOSE_WINDOW, &SettingsDialog::OnCloseButton, this);

    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));

    SetIcon(wxICON(BBB_SLAB_ICON));


    //
    // Lay the dialog out
    //

    wxBoxSizer * dialogVSizer = new wxBoxSizer(wxVERTICAL);


    wxNotebook * notebook = new wxNotebook(
        this,
        wxID_ANY,
        wxPoint(-1, -1),
        wxSize(-1, -1),
        wxNB_TOP);


    //
    // Common
    //

    wxPanel * commonSimulatorPanel = new wxPanel(notebook);

    PopulateCommonSimulatorPanel(commonSimulatorPanel);

    notebook->AddPage(commonSimulatorPanel, "All Simulators");


    //
    // Classic
    //

    wxPanel * classicSimulatorPanel = new wxPanel(notebook);

    PopulateClassicSimulatorPanel(classicSimulatorPanel);

    notebook->AddPage(classicSimulatorPanel, "Classic Simulator");


    //
    // FS
    //

    wxPanel * fsSimulatorPanel = new wxPanel(notebook);

    PopulateFSSimulatorPanel(fsSimulatorPanel);

    notebook->AddPage(fsSimulatorPanel, "FS Simulator");


    //
    // Fast MSS
    //

    wxPanel * fastMSSSimulatorPanel = new wxPanel(notebook);

    PopulateFastMSSSimulatorPanel(fastMSSSimulatorPanel);

    notebook->AddPage(fastMSSSimulatorPanel, "Fast MSS");


    //
    // Gauss-Seidel
    //

    wxPanel * gaussSeidelSimulatorPanel = new wxPanel(notebook);

    PopulateGaussSeidelSimulatorPanel(gaussSeidelSimulatorPanel);

    notebook->AddPage(gaussSeidelSimulatorPanel, "Gauss-Seidel");


    //
    // Position-Based
    //

    wxPanel * positionBasedSimulatorPanel = new wxPanel(notebook);

    PopulatePositionBasedSimulatorPanel(positionBasedSimulatorPanel);

    notebook->AddPage(positionBasedSimulatorPanel, "PB Simulator");


    //
    // Rendering
    //

    wxPanel * renderingPanel = new wxPanel(notebook);

    PopulateRenderingPanel(renderingPanel);

    notebook->AddPage(renderingPanel, "Rendering");

    dialogVSizer->Add(notebook, 0, wxEXPAND);



    dialogVSizer->AddSpacer(20);



    // Buttons
    {
        wxBoxSizer * buttonsSizer = new wxBoxSizer(wxHORIZONTAL);

        buttonsSizer->AddSpacer(20);

		mRevertToDefaultsButton = new wxButton(this, wxID_ANY, "Revert to Defaults");
		mRevertToDefaultsButton->SetToolTip("Resets all settings to their default values.");
		mRevertToDefaultsButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnRevertToDefaultsButton, this);
		buttonsSizer->Add(mRevertToDefaultsButton, 0, 0, 0);

		buttonsSizer->AddStretchSpacer(1);

        mOkButton = new wxButton(this, wxID_ANY, "OK");
		mOkButton->SetToolTip("Closes the window keeping all changes.");
        mOkButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnOkButton, this);
        buttonsSizer->Add(mOkButton, 0, 0, 0);

        buttonsSizer->AddSpacer(20);

        mCancelButton = new wxButton(this, wxID_ANY, "Cancel");
		mCancelButton->SetToolTip("Reverts all changes effected since the window was last opened, and closes the window.");
        mCancelButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnCancelButton, this);
        buttonsSizer->Add(mCancelButton, 0, 0, 0);

        buttonsSizer->AddSpacer(20);

        mUndoButton = new wxButton(this, wxID_ANY, "Undo");
		mUndoButton->SetToolTip("Reverts all changes effected since the window was last opened.");
        mUndoButton->Bind(wxEVT_BUTTON, &SettingsDialog::OnUndoButton, this);
        buttonsSizer->Add(mUndoButton, 0, 0, 0);

        buttonsSizer->AddSpacer(20);

        dialogVSizer->Add(buttonsSizer, 0, wxEXPAND, 0);
    }

    dialogVSizer->AddSpacer(20);


    //
    // Finalize dialog
    //

    SetSizerAndFit(dialogVSizer);

    Centre(wxCENTER_ON_SCREEN | wxBOTH);
}

SettingsDialog::~SettingsDialog()
{
}

void SettingsDialog::Open()
{
    if (IsShown())
        return; // Handle Ctrl^S while minimized

    assert(!!mSettingsManager);

    //
    // Initialize state
    //

    // Pull currently-enforced settings
    mSettingsManager->Pull(mLiveSettings);
    mLiveSettings.ClearAllDirty();

    // Save checkpoint for undo
    mCheckpointSettings = mLiveSettings;

    // Populate controls with live settings
	SyncControlsWithSettings(mLiveSettings);

    // Remember that the user hasn't changed anything yet in this session
    mHasBeenDirtyInCurrentSession = false;

	// Enable Revert to Defaults button only if settings are different than defaults
	mAreSettingsDirtyWrtDefaults = (mLiveSettings != mSettingsManager->GetDefaults());

	// Reconcile controls wrt dirty state
	ReconcileDirtyState();


    //
    // Open dialog
    //

    this->Raise();
    this->Show();
}

////////////////////////////////////////////////////////////

void SettingsDialog::OnDoRenderAssignedParticleForcesCheckBoxClick(wxCommandEvent & event)
{
    mLiveSettings.SetValue(SLabSettings::DoRenderAssignedParticleForces, event.IsChecked());
    OnLiveSettingsChanged();
}

void SettingsDialog::OnRevertToDefaultsButton(wxCommandEvent& /*event*/)
{
	//
	// Enforce default settings
	//

	mLiveSettings = mSettingsManager->GetDefaults();

	// Do not update checkpoint, allow user to revert to it

	// Enforce everything as a safety net, immediately
	mLiveSettings.MarkAllAsDirty();
	mSettingsManager->EnforceDirtySettingsImmediate(mLiveSettings);

	// We are back in sync
	mLiveSettings.ClearAllDirty();

	assert(mSettingsManager->Pull() == mLiveSettings);

	// Re-populate controls with new values
	SyncControlsWithSettings(mLiveSettings);

	// Remember user has made changes wrt checkpoint
	mHasBeenDirtyInCurrentSession = true;

	// Remember we are clean now wrt defaults
	mAreSettingsDirtyWrtDefaults = false;

	ReconcileDirtyState();
}

void SettingsDialog::OnOkButton(wxCommandEvent & /*event*/)
{
    // Just close the dialog
    DoClose();
}

void SettingsDialog::OnCancelButton(wxCommandEvent & /*event*/)
{
    DoCancel();
}

void SettingsDialog::OnUndoButton(wxCommandEvent & /*event*/)
{
    assert(!!mSettingsManager);

    //
    // Undo changes done since last open, including eventual loads
    //

    mLiveSettings = mCheckpointSettings;

    // Just enforce anything in the checkpoint that is different than the current settings,
	// immediately
    mLiveSettings.SetDirtyWithDiff(mSettingsManager->Pull());
    mSettingsManager->EnforceDirtySettingsImmediate(mLiveSettings);

    mLiveSettings.ClearAllDirty();

    assert(mSettingsManager->Pull() == mCheckpointSettings);

    // Re-populate controls with new values
	SyncControlsWithSettings(mLiveSettings);

    // Remember we are clean now
    mHasBeenDirtyInCurrentSession = false;
    ReconcileDirtyState();
}

void SettingsDialog::OnCloseButton(wxCloseEvent & /*event*/)
{
    DoCancel();
}

/////////////////////////////////////////////////////////////////////////////

void SettingsDialog::DoCancel()
{
    assert(!!mSettingsManager);

    if (mHasBeenDirtyInCurrentSession)
    {
        //
        // Undo changes done since last open, including eventual loads
        //

        mLiveSettings = mCheckpointSettings;

        // Just enforce anything in the checkpoint that is different than the current settings,
		// immediately
        mLiveSettings.SetDirtyWithDiff(mSettingsManager->Pull());
        mSettingsManager->EnforceDirtySettingsImmediate(mLiveSettings);
    }

    //
    // Close the dialog
    //

    DoClose();
}

void SettingsDialog::DoClose()
{
    this->Hide();
}

void SettingsDialog::PopulateCommonSimulatorPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Mechanics
    {
        wxStaticBox * mechanicsBox = new wxStaticBox(panel, wxID_ANY, _("Mechanics"));

        wxBoxSizer * mechanicsBoxSizer = new wxBoxSizer(wxVERTICAL);
        mechanicsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * mechanicsSizer = new wxGridBagSizer(0, 0);

            // Time step
            {
                mCommonSimulationTimeStepDurationSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Time Step",
                    "The time step of the simulation.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::CommonSimulationTimeStepDuration, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        0.000002f,
                        0.02f,
                        0.4f));

                mechanicsSizer->Add(
                    mCommonSimulationTimeStepDurationSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Mass Adjustment
            {
                mCommonMassAdjustmentSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Mass Adjust",
                    "Adjusts the mass of all particles.",
					[this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::CommonMassAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mSimulationController->GetCommonMinMassAdjustment(),
                        1.0f,
                        mSimulationController->GetCommonMaxMassAdjustment()));

                mechanicsSizer->Add(
                    mCommonMassAdjustmentSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Gravity Adjustment
            {
                mCommonGravityAdjustmentSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Gravity Adjust",
                    "Adjusts the magnitude of gravity.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::CommonGravityAdjustment, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<ExponentialSliderCore>(
                        mSimulationController->GetCommonMinGravityAdjustment(),
                        1.0f,
                        mSimulationController->GetCommonMaxGravityAdjustment()));

                mechanicsSizer->Add(
                    mCommonGravityAdjustmentSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            mechanicsBoxSizer->Add(mechanicsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        mechanicsBox->SetSizerAndFit(mechanicsBoxSizer);

        gridSizer->Add(
            mechanicsBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 3),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorder);
    }

    // Computation
    {
        wxStaticBox * computationBox = new wxStaticBox(panel, wxID_ANY, _("Computation"));

        wxBoxSizer * computationBoxSizer = new wxBoxSizer(wxVERTICAL);
        computationBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * computationSizer = new wxGridBagSizer(0, 0);

            // NumberOfThreads
            {
                mNumberOfSimulationThreadsSlider = new SliderControl<size_t>(
                    computationBox,
                    SliderWidth,
                    SliderHeight,
                    "Number of Threads",
                    "The number of threads that multi-threaded simulators may use.",
                    [this](size_t value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::NumberOfSimulationThreads, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<size_t>>(
                        mSimulationController->GetMinNumberOfSimulationThreads(),
                        mSimulationController->GetMaxNumberOfSimulationThreads()));

                computationSizer->Add(
                    mNumberOfSimulationThreadsSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            computationBoxSizer->Add(computationSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        computationBox->SetSizerAndFit(computationBoxSizer);

        gridSizer->Add(
            computationBox,
            wxGBPosition(0, 4),
            wxGBSpan(1, 1),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorder);
    }


    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateClassicSimulatorPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Mechanics
    {
        wxStaticBox * mechanicsBox = new wxStaticBox(panel, wxID_ANY, _("Mechanics"));

        wxBoxSizer * mechanicsBoxSizer = new wxBoxSizer(wxVERTICAL);
        mechanicsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * mechanicsSizer = new wxGridBagSizer(0, 0);

            // Spring Stiffness
            {
                mClassicSimulatorSpringStiffnessSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Stiffness",
                    "Adjusts the stiffness of the spring's Hookean force.",
					[this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::ClassicSimulatorSpringStiffnessCoefficient, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetClassicSimulatorMinSpringStiffnessCoefficient(),
                        mSimulationController->GetClassicSimulatorMaxSpringStiffnessCoefficient()));

                mechanicsSizer->Add(
                    mClassicSimulatorSpringStiffnessSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Spring Damping
            {
                mClassicSimulatorSpringDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Damping",
                    "Adjusts the magnitude of the spring damping.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::ClassicSimulatorSpringDampingCoefficient, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetClassicSimulatorMinSpringDampingCoefficient(),
                        mSimulationController->GetClassicSimulatorMaxSpringDampingCoefficient()));

                mechanicsSizer->Add(
                    mClassicSimulatorSpringDampingSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Global Damping
            {
                mClassicSimulatorGlobalDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Global Damping",
                    "The global velocity damp factor.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::ClassicSimulatorGlobalDamping, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetClassicSimulatorMinGlobalDamping(),
                        mSimulationController->GetClassicSimulatorMaxGlobalDamping()));

                mechanicsSizer->Add(
                    mClassicSimulatorGlobalDampingSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            mechanicsBoxSizer->Add(mechanicsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        mechanicsBox->SetSizerAndFit(mechanicsBoxSizer);

        gridSizer->Add(
            mechanicsBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 3),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorder);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateFSSimulatorPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Mechanics
    {
        wxStaticBox * mechanicsBox = new wxStaticBox(panel, wxID_ANY, _("Mechanics"));

        wxBoxSizer * mechanicsBoxSizer = new wxBoxSizer(wxVERTICAL);
        mechanicsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * mechanicsSizer = new wxGridBagSizer(0, 0);

            // Num Iterations
            {
                mFSSimulatorNumMechanicalDynamicsIterationsSlider = new SliderControl<size_t>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Num Iterations",
                    "Adjusts the number of iterations of the spring relaxation algorithm.",
                    [this](size_t value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::FSSimulatorNumMechanicalDynamicsIterations, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<size_t>>(
                        mSimulationController->GetFSSimulatorMinNumMechanicalDynamicsIterations(),
                        mSimulationController->GetFSSimulatorMaxNumMechanicalDynamicsIterations()));

                mechanicsSizer->Add(
                    mFSSimulatorNumMechanicalDynamicsIterationsSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Spring Reduction Fraction
            {
                mFSSimulatorSpringReductionFraction = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Reduction Fraction",
                    "Adjusts the fraction of the over-length that gets reduced by the spring.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::FSSimulatorSpringReductionFraction, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetFSSimulatorMinSpringReductionFraction(),
                        mSimulationController->GetFSSimulatorMaxSpringReductionFraction()));

                mechanicsSizer->Add(
                    mFSSimulatorSpringReductionFraction,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Spring Damping
            {
                mFSSimulatorSpringDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Damping",
                    "Adjusts the magnitude of the spring damping.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::FSSimulatorSpringDampingCoefficient, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetFSSimulatorMinSpringDampingCoefficient(),
                        mSimulationController->GetFSSimulatorMaxSpringDampingCoefficient()));

                mechanicsSizer->Add(
                    mFSSimulatorSpringDampingSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Global Damping
            {
                mFSSimulatorGlobalDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Global Damping",
                    "The global velocity damp factor.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::FSSimulatorGlobalDamping, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetFSSimulatorMinGlobalDamping(),
                        mSimulationController->GetFSSimulatorMaxGlobalDamping()));

                mechanicsSizer->Add(
                    mFSSimulatorGlobalDampingSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            mechanicsBoxSizer->Add(mechanicsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        mechanicsBox->SetSizerAndFit(mechanicsBoxSizer);

        gridSizer->Add(
            mechanicsBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 4),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorder);
    }


    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateFastMSSSimulatorPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Mechanics
    {
        wxStaticBox * mechanicsBox = new wxStaticBox(panel, wxID_ANY, _("Mechanics"));

        wxBoxSizer * mechanicsBoxSizer = new wxBoxSizer(wxVERTICAL);
        mechanicsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * mechanicsSizer = new wxGridBagSizer(0, 0);

            // Local-Global Num Iterations
            {
                mFastMSSSimulatorNumLocalGlobalStepIterationsSlider = new SliderControl<size_t>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Local-Global Solver Iterations",
                    "Adjusts the number of local-global solver iterations per step.",
                    [this](size_t value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::FastMSSSimulatorNumLocalGlobalStepIterations, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<size_t>>(
                        mSimulationController->GetFastMSSSimulatorMinNumLocalGlobalStepIterations(),
                        mSimulationController->GetFastMSSSimulatorMaxNumLocalGlobalStepIterations()));

                mechanicsSizer->Add(
                    mFastMSSSimulatorNumLocalGlobalStepIterationsSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }
            
            // Spring Stiffness
            {
                mFastMSSSimulatorSpringStiffnessSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,                    
                    "Spring Stiffness",
                    "Adjusts the stiffness of springs.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::FastMSSSimulatorSpringStiffnessCoefficient, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetFastMSSSimulatorMinSpringStiffnessCoefficient(),
                        mSimulationController->GetFastMSSSimulatorMaxSpringStiffnessCoefficient()));

                mechanicsSizer->Add(
                    mFastMSSSimulatorSpringStiffnessSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Global Damping
            {
                mFastMSSSimulatorGlobalDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Global Damping",
                    "The global velocity damp factor.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::FastMSSSimulatorGlobalDamping, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetFastMSSSimulatorMinGlobalDamping(),
                        mSimulationController->GetFastMSSSimulatorMaxGlobalDamping()));

                mechanicsSizer->Add(
                    mFastMSSSimulatorGlobalDampingSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            mechanicsBoxSizer->Add(mechanicsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        mechanicsBox->SetSizerAndFit(mechanicsBoxSizer);

        gridSizer->Add(
            mechanicsBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 3),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorder);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateGaussSeidelSimulatorPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Mechanics
    {
        wxStaticBox * mechanicsBox = new wxStaticBox(panel, wxID_ANY, _("Mechanics"));

        wxBoxSizer * mechanicsBoxSizer = new wxBoxSizer(wxVERTICAL);
        mechanicsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * mechanicsSizer = new wxGridBagSizer(0, 0);

            // Num Iterations
            {
                mGaussSeidelSimulatorNumMechanicalDynamicsIterationsSlider = new SliderControl<size_t>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Num Iterations",
                    "Adjusts the number of iterations of the spring relaxation algorithm.",
                    [this](size_t value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::GaussSeidelSimulatorNumMechanicalDynamicsIterations, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<size_t>>(
                        mSimulationController->GetGaussSeidelSimulatorMinNumMechanicalDynamicsIterations(),
                        mSimulationController->GetGaussSeidelSimulatorMaxNumMechanicalDynamicsIterations()));

                mechanicsSizer->Add(
                    mGaussSeidelSimulatorNumMechanicalDynamicsIterationsSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Spring Reduction Fraction
            {
                mGaussSeidelSimulatorSpringReductionFraction = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Reduction Fraction",
                    "Adjusts the fraction of the over-length that gets reduced by the spring.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::GaussSeidelSimulatorSpringReductionFraction, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetGaussSeidelSimulatorMinSpringReductionFraction(),
                        mSimulationController->GetGaussSeidelSimulatorMaxSpringReductionFraction()));

                mechanicsSizer->Add(
                    mGaussSeidelSimulatorSpringReductionFraction,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Spring Damping
            {
                mGaussSeidelSimulatorSpringDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Damping",
                    "Adjusts the magnitude of the spring damping.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::GaussSeidelSimulatorSpringDampingCoefficient, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetGaussSeidelSimulatorMinSpringDampingCoefficient(),
                        mSimulationController->GetGaussSeidelSimulatorMaxSpringDampingCoefficient()));

                mechanicsSizer->Add(
                    mGaussSeidelSimulatorSpringDampingSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Global Damping
            {
                mGaussSeidelSimulatorGlobalDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Global Damping",
                    "The global velocity damp factor.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::GaussSeidelSimulatorGlobalDamping, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetGaussSeidelSimulatorMinGlobalDamping(),
                        mSimulationController->GetGaussSeidelSimulatorMaxGlobalDamping()));

                mechanicsSizer->Add(
                    mGaussSeidelSimulatorGlobalDampingSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            mechanicsBoxSizer->Add(mechanicsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        mechanicsBox->SetSizerAndFit(mechanicsBoxSizer);

        gridSizer->Add(
            mechanicsBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 4),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorder);
    }


    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulatePositionBasedSimulatorPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Mechanics
    {
        wxStaticBox * mechanicsBox = new wxStaticBox(panel, wxID_ANY, _("Mechanics"));

        wxBoxSizer * mechanicsBoxSizer = new wxBoxSizer(wxVERTICAL);
        mechanicsBoxSizer->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * mechanicsSizer = new wxGridBagSizer(0, 0);

            // Num Update Iterations
            {
                mPositionBasedSimulatorNumUpdateIterationsSlider = new SliderControl<size_t>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Num Update Iterations",
                    "Adjusts the number of iterations per step.",
                    [this](size_t value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::PositionBasedSimulatorNumUpdateIterations, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<size_t>>(
                        mSimulationController->GetPositionBasedSimulatorMinNumUpdateIterations(),
                        mSimulationController->GetPositionBasedSimulatorMaxNumUpdateIterations()));

                mechanicsSizer->Add(
                    mPositionBasedSimulatorNumUpdateIterationsSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Num Solver Iterations
            {
                mPositionBasedSimulatorNumSolverIterationsSlider = new SliderControl<size_t>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Num Solver Iterations",
                    "Adjusts the number of solver iterations.",
                    [this](size_t value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::PositionBasedSimulatorNumSolverIterations, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<IntegralLinearSliderCore<size_t>>(
                        mSimulationController->GetPositionBasedSimulatorMinNumSolverIterations(),
                        mSimulationController->GetPositionBasedSimulatorMaxNumSolverIterations()));

                mechanicsSizer->Add(
                    mPositionBasedSimulatorNumSolverIterationsSlider,
                    wxGBPosition(0, 1),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Spring Stiffness
            {
                mPositionBasedSimulatorSpringStiffnessSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Stiffness",
                    "Adjusts the stiffness of the spring constraints.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::PositionBasedSimulatorSpringStiffness, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetPositionBasedSimulatorMinSpringStiffness(),
                        mSimulationController->GetPositionBasedSimulatorMaxSpringStiffness()));

                mechanicsSizer->Add(
                    mPositionBasedSimulatorSpringStiffnessSlider,
                    wxGBPosition(0, 2),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Global Damping
            {
                mPositionBasedSimulatorGlobalDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Global Damping",
                    "The global velocity damp factor.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::PositionBasedSimulatorGlobalDamping, value);
                this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetPositionBasedSimulatorMinGlobalDamping(),
                        mSimulationController->GetPositionBasedSimulatorMaxGlobalDamping()));

                mechanicsSizer->Add(
                    mPositionBasedSimulatorGlobalDampingSlider,
                    wxGBPosition(0, 3),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            mechanicsBoxSizer->Add(mechanicsSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        mechanicsBox->SetSizerAndFit(mechanicsBoxSizer);

        gridSizer->Add(
            mechanicsBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 4),
            wxEXPAND | wxALL | wxALIGN_CENTER_HORIZONTAL,
            CellBorder);
    }

    // Finalize panel

    for (int c = 0; c < gridSizer->GetCols(); ++c)
        gridSizer->AddGrowableCol(c);

    panel->SetSizer(gridSizer);
}

void SettingsDialog::PopulateRenderingPanel(wxPanel * panel)
{
    wxGridBagSizer * gridSizer = new wxGridBagSizer(0, 0);

    // Forces
    {
        wxStaticBox * forcesBox = new wxStaticBox(panel, wxID_ANY, _("Forces"));

        wxBoxSizer * forcesBoxSizer1 = new wxBoxSizer(wxVERTICAL);
        forcesBoxSizer1->AddSpacer(StaticBoxTopMargin);

        {
            wxGridBagSizer * forcesSizer = new wxGridBagSizer(0, 0);

            // Do Render assigned particle forces
            {
                mDoRenderAssignedParticleForcesCheckBox = new wxCheckBox(forcesBox, wxID_ANY,
                    _("Render Assigned Particle Forces"), wxDefaultPosition, wxDefaultSize);
                mDoRenderAssignedParticleForcesCheckBox->SetToolTip("Renders the forces that have been manually assigned to individual particles.");
                mDoRenderAssignedParticleForcesCheckBox->Bind(wxEVT_COMMAND_CHECKBOX_CLICKED, &SettingsDialog::OnDoRenderAssignedParticleForcesCheckBoxClick, this);

                forcesSizer->Add(
                    mDoRenderAssignedParticleForcesCheckBox,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxALL,
                    CellBorder);
            }

            forcesBoxSizer1->Add(forcesSizer, 0, wxALL, StaticBoxInsetMargin);
        }

        forcesBox->SetSizerAndFit(forcesBoxSizer1);

        gridSizer->Add(
            forcesBox,
            wxGBPosition(0, 0),
            wxGBSpan(1, 1),
            wxALL,
            CellBorder);
    }

    // Finalize panel

    panel->SetSizer(gridSizer);
}

void SettingsDialog::SyncControlsWithSettings(Settings<SLabSettings> const & settings)
{
    // Common
    mCommonSimulationTimeStepDurationSlider->SetValue(settings.GetValue<float>(SLabSettings::CommonSimulationTimeStepDuration));
    mCommonMassAdjustmentSlider->SetValue(settings.GetValue<float>(SLabSettings::CommonMassAdjustment));
    mCommonGravityAdjustmentSlider->SetValue(settings.GetValue<float>(SLabSettings::CommonGravityAdjustment));    
    mNumberOfSimulationThreadsSlider->SetValue(settings.GetValue<size_t>(SLabSettings::NumberOfSimulationThreads));

    // Classic
    mClassicSimulatorSpringStiffnessSlider->SetValue(settings.GetValue<float>(SLabSettings::ClassicSimulatorSpringStiffnessCoefficient));
    mClassicSimulatorSpringDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::ClassicSimulatorSpringDampingCoefficient));
    mClassicSimulatorGlobalDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::ClassicSimulatorGlobalDamping));

    // FS
    mFSSimulatorNumMechanicalDynamicsIterationsSlider->SetValue(settings.GetValue<size_t>(SLabSettings::FSSimulatorNumMechanicalDynamicsIterations));
    mFSSimulatorSpringReductionFraction->SetValue(settings.GetValue<float>(SLabSettings::FSSimulatorSpringReductionFraction));
    mFSSimulatorSpringDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::FSSimulatorSpringDampingCoefficient));
    mFSSimulatorGlobalDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::FSSimulatorGlobalDamping));

    // Position-Based
    mPositionBasedSimulatorNumUpdateIterationsSlider->SetValue(settings.GetValue<size_t>(SLabSettings::PositionBasedSimulatorNumUpdateIterations));
    mPositionBasedSimulatorNumSolverIterationsSlider->SetValue(settings.GetValue<size_t>(SLabSettings::PositionBasedSimulatorNumSolverIterations));
    mPositionBasedSimulatorSpringStiffnessSlider->SetValue(settings.GetValue<float>(SLabSettings::PositionBasedSimulatorSpringStiffness));
    mPositionBasedSimulatorGlobalDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::PositionBasedSimulatorGlobalDamping));

    // FastMSS
    mFastMSSSimulatorNumLocalGlobalStepIterationsSlider->SetValue(settings.GetValue<size_t>(SLabSettings::FastMSSSimulatorNumLocalGlobalStepIterations));
    mFastMSSSimulatorSpringStiffnessSlider->SetValue(settings.GetValue<float>(SLabSettings::FastMSSSimulatorSpringStiffnessCoefficient));
    mFastMSSSimulatorGlobalDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::FastMSSSimulatorGlobalDamping));

    // Gauss-Seidel
    mGaussSeidelSimulatorNumMechanicalDynamicsIterationsSlider->SetValue(settings.GetValue<size_t>(SLabSettings::GaussSeidelSimulatorNumMechanicalDynamicsIterations));
    mGaussSeidelSimulatorSpringReductionFraction->SetValue(settings.GetValue<float>(SLabSettings::GaussSeidelSimulatorSpringReductionFraction));
    mGaussSeidelSimulatorSpringDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::GaussSeidelSimulatorSpringDampingCoefficient));
    mGaussSeidelSimulatorGlobalDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::GaussSeidelSimulatorGlobalDamping));

    // Render
    mDoRenderAssignedParticleForcesCheckBox->SetValue(settings.GetValue<bool>(SLabSettings::DoRenderAssignedParticleForces));
}

void SettingsDialog::OnLiveSettingsChanged()
{
    assert(!!mSettingsManager);

    // Enforce settings that have just changed
    mSettingsManager->EnforceDirtySettings(mLiveSettings);

    // We're back in sync
    mLiveSettings.ClearAllDirty();

	// Remember that we have changed since we were opened
	mHasBeenDirtyInCurrentSession = true;
	mAreSettingsDirtyWrtDefaults = true; // Best effort, assume each change deviates from defaults
	ReconcileDirtyState();
}

void SettingsDialog::ReconcileDirtyState()
{
    //
    // Update buttons' state based on dirty state
    //

	mRevertToDefaultsButton->Enable(mAreSettingsDirtyWrtDefaults);
    mUndoButton->Enable(mHasBeenDirtyInCurrentSession);
}