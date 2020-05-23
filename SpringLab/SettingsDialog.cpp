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
#include "UIControls/SimulationTimeStepSliderCore.h"

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

    SetIcon(wxICON(BBB_SHIP_ICON));


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
                    std::make_unique<SimulationTimeStepSliderCore>());

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

            // Global Damping
            {
                mCommonGlobalDampingSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Global Damping",
                    "The global velocity damp factor.",
                    [this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::CommonGlobalDamping, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetCommonMinGlobalDamping(),
                        mSimulationController->GetCommonMaxGlobalDamping()));

                mechanicsSizer->Add(
                    mCommonGlobalDampingSlider,
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
            wxEXPAND | wxALL,
            CellBorder);
    }


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
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

            // Spring Reduction Fraction
            {
                mClassicSimulatorSpringReductionFractionSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Reduction Fraction",
                    "Adjusts the fraction of the spring displacement that a spring force attempts to cause a counter-displacement for.",
					[this](float value)
                    {
                        this->mLiveSettings.SetValue(SLabSettings::ClassicSimulatorSpringReductionFraction, value);
                        this->OnLiveSettingsChanged();
                    },
                    std::make_unique<LinearSliderCore>(
                        mSimulationController->GetClassicSimulatorMinSpringReductionFraction(),
                        mSimulationController->GetClassicSimulatorMaxSpringReductionFraction()));

                mechanicsSizer->Add(
                    mClassicSimulatorSpringReductionFractionSlider,
                    wxGBPosition(0, 0),
                    wxGBSpan(1, 1),
                    wxEXPAND | wxALL,
                    CellBorder);
            }

            // Spring Damping Coefficient
            {
                mClassicSimulatorSpringDampingCoefficientSlider = new SliderControl<float>(
                    mechanicsBox,
                    SliderWidth,
                    SliderHeight,
                    "Spring Damping Coefficient",
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
                    mClassicSimulatorSpringDampingCoefficientSlider,
                    wxGBPosition(0, 1),
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
            wxGBSpan(1, 2),
            wxEXPAND | wxALL,
            CellBorder);
    }


    // Finalize panel

    panel->SetSizerAndFit(gridSizer);
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

    panel->SetSizerAndFit(gridSizer);
}

void SettingsDialog::SyncControlsWithSettings(Settings<SLabSettings> const & settings)
{
    // Common
    mCommonSimulationTimeStepDurationSlider->SetValue(settings.GetValue<float>(SLabSettings::CommonSimulationTimeStepDuration));
    mCommonMassAdjustmentSlider->SetValue(settings.GetValue<float>(SLabSettings::CommonMassAdjustment));
    mCommonGravityAdjustmentSlider->SetValue(settings.GetValue<float>(SLabSettings::CommonGravityAdjustment));
    mCommonGlobalDampingSlider->SetValue(settings.GetValue<float>(SLabSettings::CommonGlobalDamping));

    // Classic
    mClassicSimulatorSpringReductionFractionSlider->SetValue(settings.GetValue<float>(SLabSettings::ClassicSimulatorSpringReductionFraction));
    mClassicSimulatorSpringDampingCoefficientSlider->SetValue(settings.GetValue<float>(SLabSettings::ClassicSimulatorSpringDampingCoefficient));

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