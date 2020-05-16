/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SimulationController.h"

#include "ObjectBuilder.h"
#include "ResourceLocator.h"

#include <iomanip> // TODO
#include <sstream>

std::unique_ptr<SimulationController> SimulationController::Create(
    std::function<void()> makeRenderContextCurrentFunction,
    std::function<void()> swapRenderBuffersFunction)
{
    // Load materials
    StructuralMaterialDatabase structuralMaterialDatabase = StructuralMaterialDatabase::Load();

    // Create render context
    std::unique_ptr<RenderContext> renderContext = std::make_unique<RenderContext>(
        std::move(makeRenderContextCurrentFunction),
        std::move(swapRenderBuffersFunction));

    //
    // Create controller
    //

    return std::unique_ptr<SimulationController>(
        new SimulationController(
            std::move(renderContext),
            std::move(structuralMaterialDatabase)));
}

SimulationController::SimulationController(
    std::unique_ptr<RenderContext> renderContext,
    StructuralMaterialDatabase structuralMaterialDatabase)
    : mEventDispatcher()
    , mRenderContext(std::move(renderContext))
    , mStructuralMaterialDatabase(std::move(structuralMaterialDatabase))
    // Simulation state
    , mCurrentSimulationTime(0.0f)
    , mTotalSimulationSteps(0)
    , mSimulationParameters()
    , mObject()
    , mCurrentObjectName()
    , mCurrentObjectDefinitionFilepath()
    // Stats
    , mOriginTimestamp(SLabWallClock::time_point::max())
{
    LoadObject(ResourceLocator::GetDefaultObjectDefinitionFilePath());
}

void SimulationController::LoadObject(std::filesystem::path const & objectDefinitionFilepath)
{
    // Load object definition
    auto objectDefinition = ObjectDefinition::Load(objectDefinitionFilepath);

    // Save object metadata
    std::string const objectName = objectDefinition.ObjectName;

    // Create a new object
    auto newObject = std::make_unique<Object>(
        ObjectBuilder::Create(
            std::move(objectDefinition),
            mStructuralMaterialDatabase));

    //
    // No errors, so we may continue
    //

    Reset(
        std::move(newObject),
        objectName,
        objectDefinitionFilepath);
}

void SimulationController::Reset()
{
    assert(!mCurrentObjectDefinitionFilepath.empty());

    LoadObject(mCurrentObjectDefinitionFilepath);
}

void SimulationController::RunSimulationIteration()
{
    ////////////////////////////////////////////////////////
    // Update
    ////////////////////////////////////////////////////////

    auto const updateStartTimestamp = std::chrono::steady_clock::now();

    // TODO

    auto const updateEndTimestamp = std::chrono::steady_clock::now();

    ////////////////////////////////////////////////////////
    // Book-Keeping
    ////////////////////////////////////////////////////////

    // Update stats
    mCurrentSimulationTime += SimulationParameters::SimulationStepTimeDuration<float>;
    ++mTotalSimulationSteps;

    // publish stats
    PublishStats(updateEndTimestamp - updateStartTimestamp);
}

void SimulationController::Render()
{
    // TODO
}

/////////////////////////////////////////////////////////////////////////////////
// Render controls
/////////////////////////////////////////////////////////////////////////////////

RgbImageData SimulationController::TakeScreenshot()
{
    assert(!!mRenderContext);
    return mRenderContext->TakeScreenshot();
}

/////////////////////////////////////////////////////////////////////////////////
// Simulation Parameters
/////////////////////////////////////////////////////////////////////////////////

void SimulationController::SetSpringStiffnessAdjustment(float value)
{
    mSimulationParameters.SpringStiffnessAdjustment = value;

    // TODO: communicate to simulator/s
}

void SimulationController::SetSpringDampingAdjustment(float value)
{
    mSimulationParameters.SpringDampingAdjustment = value;

    // TODO: communicate to simulator/s
}

/////////////////////////////////////////////////////////////////////////////////
// Helpers
/////////////////////////////////////////////////////////////////////////////////

void SimulationController::Reset(
    std::unique_ptr<Object> newObject,
    std::string objectName,
    std::filesystem::path objectDefinitionFilepath)
{
    //
    // Take object in
    //

    mObject = std::move(newObject);
    mCurrentObjectName = objectName;
    mCurrentObjectDefinitionFilepath = objectDefinitionFilepath;

    //
    // Auto-zoom
    //

    {
        // Calculate zoom to fit width and height (plus a nicely-looking margin)
        vec2f const objectSize = mObject->GetPoints().GetAABB().GetSize();
        float const newZoom = std::min(
            mRenderContext->CalculateZoomForWorldWidth(objectSize.x + 5.0f),
            mRenderContext->CalculateZoomForWorldHeight(objectSize.y + 3.0f));

        // Set zoom
        mRenderContext->SetZoom(newZoom);
    }

    //
    // Reset simulation
    //

    mCurrentSimulationTime = 0.0f;
    mTotalSimulationSteps = 0;

    //
    // Reset stats
    //

    mOriginTimestamp = SLabWallClock::GetInstance().Now();
}

void SimulationController::PublishStats(std::chrono::steady_clock::duration /*updateElapsed*/)
{
    // TODO: calc

    // TODO: publish via event
}