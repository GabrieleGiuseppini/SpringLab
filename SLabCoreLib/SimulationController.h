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

    float GetCommonGlobalDamping() const { return mSimulationParameters.Common.GlobalDamping; }
    void SetCommonGlobalDamping(float value) { mSimulationParameters.Common.GlobalDamping = value; mIsSimulationStateDirty = true; }
    float GetCommonMinGlobalDamping() const { return CommonSimulatorParameters::MinGlobalDamping; }
    float GetCommonMaxGlobalDamping() const { return CommonSimulatorParameters::MaxGlobalDamping; }

    float GetClassicSimulatorSpringStiffnessCoefficient() const { return mSimulationParameters.ClassicSimulator.SpringStiffnessCoefficient; }
    void SetClassicSimulatorSpringStiffnessCoefficient(float value) { mSimulationParameters.ClassicSimulator.SpringStiffnessCoefficient = value; mIsSimulationStateDirty = true; }
    float GetClassicSimulatorMinSpringStiffnessCoefficient() const { return ClassicSimulatorParameters::MinSpringStiffnessCoefficient; }
    float GetClassicSimulatorMaxSpringStiffnessCoefficient() const { return ClassicSimulatorParameters::MaxSpringStiffnessCoefficient; }

    float GetClassicSimulatorSpringDampingCoefficient() const { return mSimulationParameters.ClassicSimulator.SpringDampingCoefficient; }
    void SetClassicSimulatorSpringDampingCoefficient(float value) { mSimulationParameters.ClassicSimulator.SpringDampingCoefficient = value; mIsSimulationStateDirty = true; }
    float GetClassicSimulatorMinSpringDampingCoefficient() const { return ClassicSimulatorParameters::MinSpringDampingCoefficient; }
    float GetClassicSimulatorMaxSpringDampingCoefficient() const { return ClassicSimulatorParameters::MaxSpringDampingCoefficient; }

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
