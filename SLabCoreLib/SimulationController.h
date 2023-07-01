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
#include "ThreadManager.h"
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

    void MakeObject(size_t numSprings);

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

    void SetViewGridEnabled(bool value)
    {
        assert(!!mRenderContext);
        mRenderContext->SetGridEnabled(value);
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

    size_t GetPositionBasedSimulatorNumUpdateIterations() const { return mSimulationParameters.PositionBasedCommonSimulator.NumUpdateIterations; }
    void SetPositionBasedSimulatorNumUpdateIterations(size_t value) { mSimulationParameters.PositionBasedCommonSimulator.NumUpdateIterations = value; mIsSimulationStateDirty = true; }
    size_t GetPositionBasedSimulatorMinNumUpdateIterations() const { return PositionBasedCommonSimulatorParameters::MinNumUpdateIterations; }
    size_t GetPositionBasedSimulatorMaxNumUpdateIterations() const { return PositionBasedCommonSimulatorParameters::MaxNumUpdateIterations; }

    size_t GetPositionBasedSimulatorNumSolverIterations() const { return mSimulationParameters.PositionBasedCommonSimulator.NumSolverIterations; }
    void SetPositionBasedSimulatorNumSolverIterations(size_t value) { mSimulationParameters.PositionBasedCommonSimulator.NumSolverIterations = value; mIsSimulationStateDirty = true; }
    size_t GetPositionBasedSimulatorMinNumSolverIterations() const { return PositionBasedCommonSimulatorParameters::MinNumSolverIterations; }
    size_t GetPositionBasedSimulatorMaxNumSolverIterations() const { return PositionBasedCommonSimulatorParameters::MaxNumSolverIterations; }

    float GetPositionBasedSimulatorSpringStiffness() const { return mSimulationParameters.PositionBasedCommonSimulator.SpringStiffness; }
    void SetPositionBasedSimulatorSpringStiffness(float value) { mSimulationParameters.PositionBasedCommonSimulator.SpringStiffness = value; mIsSimulationStateDirty = true; }
    float GetPositionBasedSimulatorMinSpringStiffness() const { return PositionBasedCommonSimulatorParameters::MinSpringStiffness; }
    float GetPositionBasedSimulatorMaxSpringStiffness() const { return PositionBasedCommonSimulatorParameters::MaxSpringStiffness; }

    float GetPositionBasedSimulatorGlobalDamping() const { return mSimulationParameters.PositionBasedCommonSimulator.GlobalDamping; }
    void SetPositionBasedSimulatorGlobalDamping(float value) { mSimulationParameters.PositionBasedCommonSimulator.GlobalDamping = value; mIsSimulationStateDirty = true; }
    float GetPositionBasedSimulatorMinGlobalDamping() const { return PositionBasedCommonSimulatorParameters::MinGlobalDamping; }
    float GetPositionBasedSimulatorMaxGlobalDamping() const { return PositionBasedCommonSimulatorParameters::MaxGlobalDamping; }

    size_t GetFastMSSSimulatorNumLocalGlobalStepIterations() const { return mSimulationParameters.FastMSSCommonSimulator.NumLocalGlobalStepIterations; }
    void SetFastMSSSimulatorNumLocalGlobalStepIterations(size_t value) { mSimulationParameters.FastMSSCommonSimulator.NumLocalGlobalStepIterations = value; mIsSimulationStateDirty = true; }
    size_t GetFastMSSSimulatorMinNumLocalGlobalStepIterations() const { return FastMSSCommonSimulatorParameters::MinNumLocalGlobalStepIterations; }
    size_t GetFastMSSSimulatorMaxNumLocalGlobalStepIterations() const { return FastMSSCommonSimulatorParameters::MaxNumLocalGlobalStepIterations; }

    float GetFastMSSSimulatorSpringStiffnessCoefficient() const { return mSimulationParameters.FastMSSCommonSimulator.SpringStiffnessCoefficient; }
    void SetFastMSSSimulatorSpringStiffnessCoefficient(float value) { mSimulationParameters.FastMSSCommonSimulator.SpringStiffnessCoefficient = value; mIsSimulationStateDirty = true; }
    float GetFastMSSSimulatorMinSpringStiffnessCoefficient() const { return FastMSSCommonSimulatorParameters::MinSpringStiffnessCoefficient; }
    float GetFastMSSSimulatorMaxSpringStiffnessCoefficient() const { return FastMSSCommonSimulatorParameters::MaxSpringStiffnessCoefficient; }

    float GetFastMSSSimulatorGlobalDamping() const { return mSimulationParameters.FastMSSCommonSimulator.GlobalDamping; }
    void SetFastMSSSimulatorGlobalDamping(float value) { mSimulationParameters.FastMSSCommonSimulator.GlobalDamping = value; mIsSimulationStateDirty = true; }
    float GetFastMSSSimulatorMinGlobalDamping() const { return FastMSSCommonSimulatorParameters::MinGlobalDamping; }
    float GetFastMSSSimulatorMaxGlobalDamping() const { return FastMSSCommonSimulatorParameters::MaxGlobalDamping; }

    size_t GetGaussSeidelSimulatorNumMechanicalDynamicsIterations() const { return mSimulationParameters.GaussSeidelCommonSimulator.NumMechanicalDynamicsIterations; }
    void SetGaussSeidelSimulatorNumMechanicalDynamicsIterations(size_t value) { mSimulationParameters.GaussSeidelCommonSimulator.NumMechanicalDynamicsIterations = value; mIsSimulationStateDirty = true; }
    size_t GetGaussSeidelSimulatorMinNumMechanicalDynamicsIterations() const { return GaussSeidelCommonSimulatorParameters::MinNumMechanicalDynamicsIterations; }
    size_t GetGaussSeidelSimulatorMaxNumMechanicalDynamicsIterations() const { return GaussSeidelCommonSimulatorParameters::MaxNumMechanicalDynamicsIterations; }

    float GetGaussSeidelSimulatorSpringReductionFraction() const { return mSimulationParameters.GaussSeidelCommonSimulator.SpringReductionFraction; }
    void SetGaussSeidelSimulatorSpringReductionFraction(float value) { mSimulationParameters.GaussSeidelCommonSimulator.SpringReductionFraction = value; mIsSimulationStateDirty = true; }
    float GetGaussSeidelSimulatorMinSpringReductionFraction() const { return GaussSeidelCommonSimulatorParameters::MinSpringReductionFraction; }
    float GetGaussSeidelSimulatorMaxSpringReductionFraction() const { return GaussSeidelCommonSimulatorParameters::MaxSpringReductionFraction; }

    float GetGaussSeidelSimulatorSpringDampingCoefficient() const { return mSimulationParameters.GaussSeidelCommonSimulator.SpringDampingCoefficient; }
    void SetGaussSeidelSimulatorSpringDampingCoefficient(float value) { mSimulationParameters.GaussSeidelCommonSimulator.SpringDampingCoefficient = value; mIsSimulationStateDirty = true; }
    float GetGaussSeidelSimulatorMinSpringDampingCoefficient() const { return GaussSeidelCommonSimulatorParameters::MinSpringDampingCoefficient; }
    float GetGaussSeidelSimulatorMaxSpringDampingCoefficient() const { return GaussSeidelCommonSimulatorParameters::MaxSpringDampingCoefficient; }

    float GetGaussSeidelSimulatorGlobalDamping() const { return mSimulationParameters.GaussSeidelCommonSimulator.GlobalDamping; }
    void SetGaussSeidelSimulatorGlobalDamping(float value) { mSimulationParameters.GaussSeidelCommonSimulator.GlobalDamping = value; mIsSimulationStateDirty = true; }
    float GetGaussSeidelSimulatorMinGlobalDamping() const { return GaussSeidelCommonSimulatorParameters::MinGlobalDamping; }
    float GetGaussSeidelSimulatorMaxGlobalDamping() const { return GaussSeidelCommonSimulatorParameters::MaxGlobalDamping; }

    //
    // Parallelism
    //

    size_t GetNumberOfSimulationThreads() const { return mThreadManager.GetSimulationParallelism(); }
    void SetNumberOfSimulationThreads(size_t value) { mThreadManager.SetSimulationParallelism(value); mIsSimulationStateDirty = true; }
    size_t GetMinNumberOfSimulationThreads() const { return mThreadManager.GetMinSimulationParallelism(); }
    size_t GetMaxNumberOfSimulationThreads() const { return mThreadManager.GetMaxSimulationParallelism(); }


    //
    // Own parameters
    //

    bool GetDoRenderAssignedParticleForces() const { return mDoRenderAssignedParticleForces; }
    void SetDoRenderAssignedParticleForces(bool value) { mDoRenderAssignedParticleForces = value; }

private:

        struct ObjectDefinitionSource
        {
            enum class SourceType
            {
                File,
                Synthetic
            };

            SourceType Type;
            std::filesystem::path DefinitionFilePath;
            size_t NumSprings;

            ObjectDefinitionSource(
                SourceType type,
                std::filesystem::path const & definitionFilePath,
                size_t numSprings)
                : Type(type)
                , DefinitionFilePath(definitionFilePath)
                , NumSprings(numSprings)
            {}
        };

private:

    SimulationController(
        std::unique_ptr<RenderContext> renderContext,
        StructuralMaterialDatabase structuralMaterialDatabase);

    void Reset(
        std::unique_ptr<Object> newObject,
        std::string objectName,
        ObjectDefinitionSource && currentObjectDefinitionSource);

    void ObserveObject(PerfStats const & lastPerfStats);

private:

    EventDispatcher mEventDispatcher;
    std::unique_ptr<RenderContext> mRenderContext;
    ThreadManager mThreadManager;
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
    std::optional<ObjectDefinitionSource> mCurrentObjectDefinitionSource;

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
