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

#include <cassert>
#include <chrono> // TODO
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

    void LoadObject(std::filesystem::path const & objectDefinitionFilepath);
    void Reset();

    void RunSimulationIteration();

    void Render();

    float GetCurrentSimulationTime() const
    {
        return mCurrentSimulationTime;
    }

    //
    // Simulation Interactions
    //

    std::optional<ElementIndex> GetNearestPointAt(vec2f const & screenCoordinates) const;

    void MovePoint(ElementIndex pointElementIndex, vec2f const & screenOffset);

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

    RgbImageData TakeScreenshot();


    //
    // Simmulation parameters
    //

    float GetSpringStiffnessAdjustment() const { return mSimulationParameters.SpringStiffnessAdjustment; }
    void SetSpringStiffnessAdjustment(float value);
    float GetMinSpringStiffnessAdjustment() const { return SimulationParameters::MinSpringStiffnessAdjustment; }
    float GetMaxSpringStiffnessAdjustment() const { return SimulationParameters::MaxSpringStiffnessAdjustment; }

    float GetSpringDampingAdjustment() const { return mSimulationParameters.SpringDampingAdjustment; }
    void SetSpringDampingAdjustment(float value);
    float GetMinSpringDampingAdjustment() const { return SimulationParameters::MinSpringDampingAdjustment; }
    float GetMaxSpringDampingAdjustment() const { return SimulationParameters::MaxSpringDampingAdjustment; }

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

    float mCurrentSimulationTime;
    size_t mTotalSimulationSteps;

    SimulationParameters mSimulationParameters;

    std::unique_ptr<Object> mObject;
    std::string mCurrentObjectName;
    std::filesystem::path mCurrentObjectDefinitionFilepath;

    //
    // Stats
    //

    SLabWallClock::time_point mOriginTimestamp;
};
