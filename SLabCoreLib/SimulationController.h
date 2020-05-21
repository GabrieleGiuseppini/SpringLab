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
#include "RenderContext.h"
#include "SimulationParameters.h"
#include "SLabTypes.h"
#include "SLabWallClock.h"
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
 * This class is responsible for managing the simulation, from its lifetime to the user
 * interactions.
 */
class SimulationController
{
public:

    static std::unique_ptr<SimulationController> Create(
        int canvasWidth,
        int canvasHeight,
        std::function<void()> makeRenderContextCurrentFunction,
        std::function<void()> swapRenderBuffersFunction);

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
    void Reset();

    void UpdateSimulation();

    void Render();

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

    float GetClassicSimulatorSpringReductionFraction() const { return mSimulationParameters.ClassicSimulator.SpringReductionFraction; }
    void SetClassicSimulatorSpringReductionFraction(float value) { mSimulationParameters.ClassicSimulator.SpringReductionFraction = value; mAreSimulationParametersDirty = true; }
    float GetClassicSimulatorMinSpringReductionFraction() const { return ClassicSimulatorParameters::MinSpringReductionFraction; }
    float GetClassicSimulatorMaxSpringReductionFraction() const { return ClassicSimulatorParameters::MaxSpringReductionFraction; }

    float GetClassicSimulatorSpringDampingCoefficient() const { return mSimulationParameters.ClassicSimulator.SpringDampingCoefficient; }
    void SetClassicSimulatorSpringDampingCoefficient(float value) { mSimulationParameters.ClassicSimulator.SpringDampingCoefficient = value; mAreSimulationParametersDirty = true; }
    float GetClassicSimulatorMinSpringDampingCoefficient() const { return ClassicSimulatorParameters::MinSpringDampingCoefficient; }
    float GetClassicSimulatorMaxSpringDampingCoefficient() const { return ClassicSimulatorParameters::MaxSpringDampingCoefficient; }

private:

    SimulationController(
        std::unique_ptr<RenderContext> renderContext,
        StructuralMaterialDatabase structuralMaterialDatabase);

    void Reset(
        std::unique_ptr<Object> newObject,
        std::string objectName,
        std::filesystem::path objectDefinitionFilepath);

    void PublishStats(std::chrono::steady_clock::duration updateElapsed);

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
    size_t mTotalSimulationSteps;

    SimulationParameters mSimulationParameters;
    bool mAreSimulationParametersDirty;

    std::unique_ptr<Object> mObject;
    std::string mCurrentObjectName;
    std::filesystem::path mCurrentObjectDefinitionFilepath;

    //
    // Stats
    //

    SLabWallClock::time_point mOriginTimestamp;
};
