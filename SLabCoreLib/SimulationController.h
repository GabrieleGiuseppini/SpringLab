/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Colors.h"
#include "EventDispatcher.h"
#include "ImageData.h"
#include "Object.h"
#include "PerfStats.h"
#include "RenderContext.h"
#include "SimulationParameters.h"
#include "SLabTypes.h"
#include "StructuralMaterialDatabase.h"
#include "Vectors.h"

#include "Simulator/Common/ISimulator.h"

#include <cassert>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

/*
 * This class is responsible for managing the simulation - both its lifetime and the user
 * interactions.
 */
class SimulationController
{
public:

    static std::unique_ptr<SimulationController> Create(
        int initialCanvasWidth,
        int initialCanvasHeight);

public:

    void RegisterEventHandler(ISimulationEventHandler * handler)
    {
        mEventDispatcher.RegisterEventHandler(handler);
    }

    //
    // Simulation
    //

    void SetSimulator(std::string const & simulatorName);

    void LoadObject(std::filesystem::path const & objectDefinitionFilepath);

    void UpdateSimulation();

    void Render();

    void Reset();

    float GetCurrentSimulationTime() const
    {
        return mCurrentSimulationTime;
    }

    //
    // Simulation Interactions
    //

    void SetPointHighlight(ElementIndex pointElementIndex, float highlight);

    std::optional<ElementIndex> GetNearestPointAt(vec2f const & screenCoordinates) const;

    vec2f GetPointPosition(ElementIndex pointElementIndex) const;

    vec2f GetPointPositionInScreenCoordinates(ElementIndex pointElementIndex) const;

    bool IsPointFrozen(ElementIndex pointElementIndex) const;

    void MovePointBy(ElementIndex pointElementIndex, vec2f const & screenStride);

    void MovePointTo(ElementIndex pointElementIndex, vec2f const & screenCoordinates);

    void TogglePointFreeze(ElementIndex pointElementIndex);

    void QueryNearestPointAt(vec2f const & screenCoordinates) const;

    //
    // Render controls
    //

    void SetCanvasSize(int width, int height)
    {
        assert(!!mRenderContext);
        mRenderContext->SetCanvasSize(width, height);
    }

    void Pan(vec2f const & screenOffset)
    {
        assert(!!mRenderContext);
        vec2f const worldOffset = mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
        mRenderContext->SetCameraWorldPosition(mRenderContext->GetCameraWorldPosition() + worldOffset);
    }

    void ResetPan()
    {
        assert(!!mRenderContext);
        mRenderContext->SetCameraWorldPosition(vec2f::zero());
    }

    void AdjustZoom(float amount)
    {
        assert(!!mRenderContext);
        mRenderContext->SetZoom(mRenderContext->GetZoom() * amount);
    }

    void ResetZoom()
    {
        assert(!!mRenderContext);
        mRenderContext->SetZoom(1.0f);
    }

    vec2f ScreenToWorld(vec2f const & screenCoordinates) const
    {
        assert(!!mRenderContext);
        return mRenderContext->ScreenToWorld(screenCoordinates);
    }

    vec2f ScreenOffsetToWorldOffset(vec2f const & screenOffset) const
    {
        assert(!!mRenderContext);
        return mRenderContext->ScreenOffsetToWorldOffset(screenOffset);
    }

    vec2f WorldToScreen(vec2f const & worldCoordinates) const
    {
        assert(!!mRenderContext);
        return mRenderContext->WorldToScreen(worldCoordinates);
    }

    RgbImageData TakeScreenshot();


    //
    // Simmulation parameters
    //

    float GetCommonSimulationTimeStepDuration() const { return mSimulationParameters.Common.SimulationTimeStepDuration; }
    void SetCommonSimulationTimeStepDuration(float value) { mSimulationParameters.Common.SimulationTimeStepDuration = value; mIsSimulationStateDirty = true; }

    float GetCommonMassAdjustment() const { return mSimulationParameters.Common.MassAdjustment; }
    void SetCommonMassAdjustment(float value) { mSimulationParameters.Common.MassAdjustment = value; mIsSimulationStateDirty = true; }
    float GetCommonMinMassAdjustment() const { return CommonSimulatorParameters::MinMassAdjustment; }
    float GetCommonMaxMassAdjustment() const { return CommonSimulatorParameters::MaxMassAdjustment; }

    float GetCommonGravityAdjustment() const { return mSimulationParameters.Common.GravityAdjustment; }
    void SetCommonGravityAdjustment(float value) { mSimulationParameters.Common.GravityAdjustment = value; mIsSimulationStateDirty = true; }
    float GetCommonMinGravityAdjustment() const { return CommonSimulatorParameters::MinGravityAdjustment; }
    float GetCommonMaxGravityAdjustment() const { return CommonSimulatorParameters::MaxGravityAdjustment; }

    bool GetCommonDoApplyGravity() const { return mSimulationParameters.Common.AssignedGravity != vec2f::zero(); }
    void SetCommonDoApplyGravity(bool value);

    size_t GetCommonNumberOfThreads() const { return mSimulationParameters.Common.NumberOfThreads; }
    void SetCommonNumberOfThreads(size_t value) { mSimulationParameters.Common.NumberOfThreads = value; mIsSimulationStateDirty = true; }
    size_t GetCommonMinNumberOfThreads() const { return CommonSimulatorParameters::MinNumberOfThreads; }
    size_t GetCommonMaxNumberOfThreads() const { return mSimulationParameters.Common.MaxNumberOfThreads; }

    float GetClassicSimulatorSpringStiffnessCoefficient() const { return mSimulationParameters.ClassicSimulator.SpringStiffnessCoefficient; }
    void SetClassicSimulatorSpringStiffnessCoefficient(float value) { mSimulationParameters.ClassicSimulator.SpringStiffnessCoefficient = value; mIsSimulationStateDirty = true; }
    float GetClassicSimulatorMinSpringStiffnessCoefficient() const { return ClassicSimulatorParameters::MinSpringStiffnessCoefficient; }
    float GetClassicSimulatorMaxSpringStiffnessCoefficient() const { return ClassicSimulatorParameters::MaxSpringStiffnessCoefficient; }

    float GetClassicSimulatorSpringDampingCoefficient() const { return mSimulationParameters.ClassicSimulator.SpringDampingCoefficient; }
    void SetClassicSimulatorSpringDampingCoefficient(float value) { mSimulationParameters.ClassicSimulator.SpringDampingCoefficient = value; mIsSimulationStateDirty = true; }
    float GetClassicSimulatorMinSpringDampingCoefficient() const { return ClassicSimulatorParameters::MinSpringDampingCoefficient; }
    float GetClassicSimulatorMaxSpringDampingCoefficient() const { return ClassicSimulatorParameters::MaxSpringDampingCoefficient; }

    float GetClassicSimulatorGlobalDamping() const { return mSimulationParameters.ClassicSimulator.GlobalDamping; }
    void SetClassicSimulatorGlobalDamping(float value) { mSimulationParameters.ClassicSimulator.GlobalDamping = value; mIsSimulationStateDirty = true; }
    float GetClassicSimulatorMinGlobalDamping() const { return ClassicSimulatorParameters::MinGlobalDamping; }
    float GetClassicSimulatorMaxGlobalDamping() const { return ClassicSimulatorParameters::MaxGlobalDamping; }

    size_t GetFSSimulatorNumMechanicalDynamicsIterations() const { return mSimulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations; }
    void SetFSSimulatorNumMechanicalDynamicsIterations(size_t value) { mSimulationParameters.FSCommonSimulator.NumMechanicalDynamicsIterations = value; mIsSimulationStateDirty = true; }
    size_t GetFSSimulatorMinNumMechanicalDynamicsIterations() const { return FSCommonSimulatorParameters::MinNumMechanicalDynamicsIterations; }
    size_t GetFSSimulatorMaxNumMechanicalDynamicsIterations() const { return FSCommonSimulatorParameters::MaxNumMechanicalDynamicsIterations; }

    float GetFSSimulatorSpringReductionFraction() const { return mSimulationParameters.FSCommonSimulator.SpringReductionFraction; }
    void SetFSSimulatorSpringReductionFraction(float value) { mSimulationParameters.FSCommonSimulator.SpringReductionFraction = value; mIsSimulationStateDirty = true; }
    float GetFSSimulatorMinSpringReductionFraction() const { return FSCommonSimulatorParameters::MinSpringReductionFraction; }
    float GetFSSimulatorMaxSpringReductionFraction() const { return FSCommonSimulatorParameters::MaxSpringReductionFraction; }

    float GetFSSimulatorSpringDampingCoefficient() const { return mSimulationParameters.FSCommonSimulator.SpringDampingCoefficient; }
    void SetFSSimulatorSpringDampingCoefficient(float value) { mSimulationParameters.FSCommonSimulator.SpringDampingCoefficient = value; mIsSimulationStateDirty = true; }
    float GetFSSimulatorMinSpringDampingCoefficient() const { return FSCommonSimulatorParameters::MinSpringDampingCoefficient; }
    float GetFSSimulatorMaxSpringDampingCoefficient() const { return FSCommonSimulatorParameters::MaxSpringDampingCoefficient; }

    float GetFSSimulatorGlobalDamping() const { return mSimulationParameters.FSCommonSimulator.GlobalDamping; }
    void SetFSSimulatorGlobalDamping(float value) { mSimulationParameters.FSCommonSimulator.GlobalDamping = value; mIsSimulationStateDirty = true; }
    float GetFSSimulatorMinGlobalDamping() const { return FSCommonSimulatorParameters::MinGlobalDamping; }
    float GetFSSimulatorMaxGlobalDamping() const { return FSCommonSimulatorParameters::MaxGlobalDamping; }

    size_t GetPositionBasedSimulatorNumMechanicalDynamicsIterations() const { return mSimulationParameters.PositionBasedCommonSimulator.NumMechanicalDynamicsIterations; }
    void SetPositionBasedSimulatorNumMechanicalDynamicsIterations(size_t value) { mSimulationParameters.PositionBasedCommonSimulator.NumMechanicalDynamicsIterations = value; mIsSimulationStateDirty = true; }
    size_t GetPositionBasedSimulatorMinNumMechanicalDynamicsIterations() const { return PositionBasedCommonSimulatorParameters::MinNumMechanicalDynamicsIterations; }
    size_t GetPositionBasedSimulatorMaxNumMechanicalDynamicsIterations() const { return PositionBasedCommonSimulatorParameters::MaxNumMechanicalDynamicsIterations; }

    size_t GetPositionBasedSimulatorNumSolverIterations() const { return mSimulationParameters.PositionBasedCommonSimulator.NumSolverIterations; }
    void SetPositionBasedSimulatorNumSolverIterations(size_t value) { mSimulationParameters.PositionBasedCommonSimulator.NumSolverIterations = value; mIsSimulationStateDirty = true; }
    size_t GetPositionBasedSimulatorMinNumSolverIterations() const { return PositionBasedCommonSimulatorParameters::MinNumSolverIterations; }
    size_t GetPositionBasedSimulatorMaxNumSolverIterations() const { return PositionBasedCommonSimulatorParameters::MaxNumSolverIterations; }

    float GetPositionBasedSimulatorSpringReductionFraction() const { return mSimulationParameters.PositionBasedCommonSimulator.SpringReductionFraction; }
    void SetPositionBasedSimulatorSpringReductionFraction(float value) { mSimulationParameters.PositionBasedCommonSimulator.SpringReductionFraction = value; mIsSimulationStateDirty = true; }
    float GetPositionBasedSimulatorMinSpringReductionFraction() const { return PositionBasedCommonSimulatorParameters::MinSpringReductionFraction; }
    float GetPositionBasedSimulatorMaxSpringReductionFraction() const { return PositionBasedCommonSimulatorParameters::MaxSpringReductionFraction; }

    float GetPositionBasedSimulatorSpringDampingCoefficient() const { return mSimulationParameters.PositionBasedCommonSimulator.SpringDampingCoefficient; }
    void SetPositionBasedSimulatorSpringDampingCoefficient(float value) { mSimulationParameters.PositionBasedCommonSimulator.SpringDampingCoefficient = value; mIsSimulationStateDirty = true; }
    float GetPositionBasedSimulatorMinSpringDampingCoefficient() const { return PositionBasedCommonSimulatorParameters::MinSpringDampingCoefficient; }
    float GetPositionBasedSimulatorMaxSpringDampingCoefficient() const { return PositionBasedCommonSimulatorParameters::MaxSpringDampingCoefficient; }

    float GetPositionBasedSimulatorGlobalDamping() const { return mSimulationParameters.PositionBasedCommonSimulator.GlobalDamping; }
    void SetPositionBasedSimulatorGlobalDamping(float value) { mSimulationParameters.PositionBasedCommonSimulator.GlobalDamping = value; mIsSimulationStateDirty = true; }
    float GetPositionBasedSimulatorMinGlobalDamping() const { return PositionBasedCommonSimulatorParameters::MinGlobalDamping; }
    float GetPositionBasedSimulatorMaxGlobalDamping() const { return PositionBasedCommonSimulatorParameters::MaxGlobalDamping; }

    //
    // Own parameters
    //

    bool GetDoRenderAssignedParticleForces() const { return mDoRenderAssignedParticleForces; }
    void SetDoRenderAssignedParticleForces(bool value) { mDoRenderAssignedParticleForces = value; }

private:

    SimulationController(
        std::unique_ptr<RenderContext> renderContext,
        StructuralMaterialDatabase structuralMaterialDatabase);

    void Reset(
        std::unique_ptr<Object> newObject,
        std::string objectName,
        std::filesystem::path objectDefinitionFilepath);

    void ObserveObject(PerfStats const & lastPerfStats);

private:

    EventDispatcher mEventDispatcher;
    std::unique_ptr<RenderContext> mRenderContext;
    StructuralMaterialDatabase mStructuralMaterialDatabase;

    //
    // Current simulation state
    //

    std::unique_ptr<ISimulator> mSimulator;
    std::string mCurrentSimulatorTypeName;

    float mCurrentSimulationTime;

    SimulationParameters mSimulationParameters;

    std::unique_ptr<Object> mObject;
    std::string mCurrentObjectName;
    std::filesystem::path mCurrentObjectDefinitionFilepath;

    bool mIsSimulationStateDirty;

    //
    // Own parameters
    //

    bool mDoRenderAssignedParticleForces;

    //
    // Stats
    //

    PerfStats mPerfStats;
};
