/***************************************************************************************
 * Original Author:     Gabriele Giuseppini
 * Created:             2020-05-23
 * Copyright:           Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include "SettingsManager.h"

#include "UIControls/SliderControl.h"

#include <SLabCoreLib/Settings.h>
#include <SLabCoreLib/SimulationController.h>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/radiobox.h>

#include <memory>
#include <vector>

class SettingsDialog : public wxFrame
{
public:

    SettingsDialog(
        wxWindow * parent,
        std::shared_ptr<SettingsManager> settingsManager,
		std::shared_ptr<SimulationController> simulationController);

    virtual ~SettingsDialog();

    void Open();

private:

    void OnDoRenderAssignedParticleForcesCheckBoxClick(wxCommandEvent & event);

	void OnRevertToDefaultsButton(wxCommandEvent& event);
    void OnOkButton(wxCommandEvent & event);
    void OnCancelButton(wxCommandEvent & event);
    void OnUndoButton(wxCommandEvent & event);

    void OnCloseButton(wxCloseEvent & event);

private:

    //////////////////////////////////////////////////////
    // Control tabs
    //////////////////////////////////////////////////////

    // Common
    SliderControl<float> * mCommonSimulationTimeStepDurationSlider;
    SliderControl<float> * mCommonMassAdjustmentSlider;
    SliderControl<float> * mCommonGravityAdjustmentSlider;
    SliderControl<size_t> * mNumberOfSimulationThreadsSlider;

    // Classic
    SliderControl<float> * mClassicSimulatorSpringStiffnessSlider;
    SliderControl<float> * mClassicSimulatorSpringDampingSlider;
    SliderControl<float> * mClassicSimulatorGlobalDampingSlider;

    // FS
    SliderControl<size_t> * mFSSimulatorNumMechanicalDynamicsIterationsSlider;
    SliderControl<float> * mFSSimulatorSpringReductionFraction;
    SliderControl<float> * mFSSimulatorSpringDampingSlider;
    SliderControl<float> * mFSSimulatorGlobalDampingSlider;

    // PositionBased
    SliderControl<size_t> * mPositionBasedSimulatorNumUpdateIterationsSlider;
    SliderControl<size_t> * mPositionBasedSimulatorNumSolverIterationsSlider;
    SliderControl<float> * mPositionBasedSimulatorSpringStiffnessSlider;
    SliderControl<float> * mPositionBasedSimulatorGlobalDampingSlider;

    // FastMSS
    SliderControl<size_t> * mFastMSSSimulatorNumLocalGlobalStepIterationsSlider;
    SliderControl<float> * mFastMSSSimulatorSpringStiffnessSlider;
    SliderControl<float> * mFastMSSSimulatorGlobalDampingSlider;

    // GaussSeidel
    SliderControl<size_t> * mGaussSeidelSimulatorNumMechanicalDynamicsIterationsSlider;
    SliderControl<float> * mGaussSeidelSimulatorSpringReductionFraction;
    SliderControl<float> * mGaussSeidelSimulatorSpringDampingSlider;
    SliderControl<float> * mGaussSeidelSimulatorGlobalDampingSlider;

    // Rendering
    wxCheckBox * mDoRenderAssignedParticleForcesCheckBox;

    //////////////////////////////////////////////////////

    // Buttons
	wxButton * mRevertToDefaultsButton;
    wxButton * mOkButton;
    wxButton * mCancelButton;
    wxButton * mUndoButton;

private:

    void DoCancel();
    void DoClose();

    void PopulateCommonSimulatorPanel(wxPanel * panel);
    void PopulateClassicSimulatorPanel(wxPanel * panel);
    void PopulateFSSimulatorPanel(wxPanel * panel);
    void PopulatePositionBasedSimulatorPanel(wxPanel * panel);
    void PopulateFastMSSSimulatorPanel(wxPanel * panel);
    void PopulateGaussSeidelSimulatorPanel(wxPanel * panel);
    void PopulateRenderingPanel(wxPanel * panel);

    void SyncControlsWithSettings(Settings<SLabSettings> const & settings);

    void OnLiveSettingsChanged();
    void ReconcileDirtyState();

private:

    wxWindow * const mParent;
    std::shared_ptr<SettingsManager> mSettingsManager;
	std::shared_ptr<SimulationController> mSimulationController;

    //
    // State
    //

    // The current settings, always enforced
    Settings<SLabSettings> mLiveSettings;

    // The settings when the dialog was last opened
    Settings<SLabSettings> mCheckpointSettings;

    // Tracks whether the user has changed any settings since the dialog
    // was last opened. When false there's a guarantee that the current live
    // settings have not been modified.
    bool mHasBeenDirtyInCurrentSession;

	// Tracks whether the current settings are (possibly) dirty wrt the defaults.
	// Best effort, we assume all changes deviate from the default.
	bool mAreSettingsDirtyWrtDefaults;
};
