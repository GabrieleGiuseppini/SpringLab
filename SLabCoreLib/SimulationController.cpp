/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SimulationController.h"

#include "ObjectBuilder.h"
#include "ResourceLocator.h"

#include "Simulator/Common/SimulatorRegistry.h"

std::unique_ptr<SimulationController> SimulationController::Create(
    int canvasWidth,
    int canvasHeight,
    std::function<void()> makeRenderContextCurrentFunction,
    std::function<void()> swapRenderBuffersFunction)
{
    // Load materials
    StructuralMaterialDatabase structuralMaterialDatabase = StructuralMaterialDatabase::Load();

    // Create render context
    std::unique_ptr<RenderContext> renderContext = std::make_unique<RenderContext>(
        canvasWidth,
        canvasHeight,
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
    , mSimulator()
    , mCurrentSimulatorTypeName(SimulatorRegistry::GetDefaultSimulatorTypeName())
    , mCurrentSimulationTime(0.0f)
    , mTotalSimulationSteps(0)
    , mSimulationParameters()
    , mObject()
    , mCurrentObjectName()
    , mCurrentObjectDefinitionFilepath()
    , mIsSimulationStateDirty(false)
    // Parameters
    , mDoRenderAssignedParticleForces(false)
    // Stats
    , mOriginTimestamp(SLabWallClock::time_point::max())
{
    LoadObject(ResourceLocator::GetDefaultObjectDefinitionFilePath());
}

void SimulationController::SetSimulator(std::string const & simulatorName)
{
    LogMessage("SimulationController::SetSimulator(", simulatorName, ")");

    mCurrentSimulatorTypeName = simulatorName;

    Reset();
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

void SimulationController::UpdateSimulation()
{
    assert(!!mSimulator);
    assert(!!mObject);

    ////////////////////////////////////////////////////////
    // Update parameters
    ////////////////////////////////////////////////////////

    if (mIsSimulationStateDirty)
    {
        mSimulator->OnStateChanged(*mObject, mSimulationParameters);

        mIsSimulationStateDirty = false;
    }

    ////////////////////////////////////////////////////////
    // Update
    ////////////////////////////////////////////////////////

    auto const updateStartTimestamp = std::chrono::steady_clock::now();

    // Update simulation
    mSimulator->Update(
        *mObject,
        mCurrentSimulationTime,
        mSimulationParameters);

    auto const updateEndTimestamp = std::chrono::steady_clock::now();

    // Update simulation time
    mCurrentSimulationTime += mSimulationParameters.Common.SimulationTimeStepDuration;

    ////////////////////////////////////////////////////////
    // Observe
    ////////////////////////////////////////////////////////

    ObserveObject();

    ////////////////////////////////////////////////////////
    // Book-Keeping
    ////////////////////////////////////////////////////////

    // Update stats
    mCurrentSimulationTime += mSimulationParameters.Common.SimulationTimeStepDuration;
    ++mTotalSimulationSteps;

    // publish stats
    PublishStats(updateEndTimestamp - updateStartTimestamp);
}

void SimulationController::Render()
{
    assert(!!mRenderContext);

    mRenderContext->RenderStart();

    if (!!mObject)
    {
        mRenderContext->UploadPoints(
            mObject->GetPoints().GetElementCount(),
            mObject->GetPoints().GetPositionBuffer(),
            mObject->GetPoints().GetRenderColorBuffer(),
            mObject->GetPoints().GetRenderNormRadiusBuffer(),
            mObject->GetPoints().GetRenderHighlightBuffer(),
            mObject->GetPoints().GetFrozenCoefficientBuffer());

        mRenderContext->UploadSpringsStart(mObject->GetSprings().GetElementCount());

        for (auto s : mObject->GetSprings())
        {
            mRenderContext->UploadSpring(
                mObject->GetPoints().GetPosition(mObject->GetSprings().GetEndpointAIndex(s)),
                mObject->GetPoints().GetPosition(mObject->GetSprings().GetEndpointBIndex(s)),
                mObject->GetSprings().GetRenderColor(s),
                mObject->GetSprings().GetRenderNormThickness(s),
                mObject->GetSprings().GetRenderHighlight(s));
        }

        mRenderContext->UploadSpringsEnd();
    }

    mRenderContext->RenderEnd();
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
// Simulation parameters
/////////////////////////////////////////////////////////////////////////////////

void SimulationController::SetCommonDoApplyGravity(bool value)
{
    if (value)
    {
        mSimulationParameters.Common.AssignedGravity = SimulationParameters::Gravity;
    }
    else
    {
        mSimulationParameters.Common.AssignedGravity = vec2f::zero();
    }

    mIsSimulationStateDirty = true;
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
    // Auto-zoom & center
    //

    {
        AABB const objectAABB = mObject->GetPoints().GetAABB();

        vec2f const objectSize = objectAABB.GetSize();

        // Zoom to fit width and height (plus a nicely-looking margin)
        float const newZoom = std::min(
            mRenderContext->CalculateZoomForWorldWidth(objectSize.x + 5.0f),
            mRenderContext->CalculateZoomForWorldHeight(objectSize.y + 3.0f));
        mRenderContext->SetZoom(newZoom);

        // Center
        vec2f const objectCenter(
            (objectAABB.BottomLeft.x + objectAABB.TopRight.x) / 2.0f,
            (objectAABB.BottomLeft.y + objectAABB.TopRight.y) / 2.0f);
        mRenderContext->SetCameraWorldPosition(objectCenter);

    }

    //
    // Reset simulation
    //

    // Make new simulator
    mSimulator = SimulatorRegistry::MakeSimulator(mCurrentSimulatorTypeName, *mObject, mSimulationParameters);

    // Reset simulation state
    mCurrentSimulationTime = 0.0f;
    mTotalSimulationSteps = 0;
    mIsSimulationStateDirty = false;

    // Publish reset
    mEventDispatcher.OnSimulationReset();

    //
    // Reset stats
    //

    mOriginTimestamp = SLabWallClock::GetInstance().Now();
}

void SimulationController::ObserveObject()
{
    //
    // Calculate:
    // - Total kinetic energy
    // - Total potential energy
    //

    float totalKineticEnergy = 0;
    float totalPotentialEnergy = 0;

    auto const & points = mObject->GetPoints();
    for (auto p : points)
    {
        totalKineticEnergy +=
            points.GetMass(p)
            * points.GetVelocity(p).squareLength();
    }

    auto const & springs = mObject->GetSprings();
    for (auto s : springs)
    {
        auto const endpointAIndex = springs.GetEndpointAIndex(s);
        auto const endpointBIndex = springs.GetEndpointBIndex(s);

        float const displacementLength = (points.GetPosition(endpointBIndex) - points.GetPosition(endpointAIndex)).length();

        totalPotentialEnergy +=
            mSimulationParameters.ClassicSimulator.SpringStiffness
            * springs.GetMaterialStiffness(s)
            * abs(displacementLength - springs.GetRestLength(s));
    }

    totalKineticEnergy *= 0.5f;
    totalPotentialEnergy *= 0.5f;

    //
    // Publish observations
    //

    mEventDispatcher.OnObjectProbe(totalKineticEnergy, totalPotentialEnergy);
}

void SimulationController::PublishStats(std::chrono::steady_clock::duration /*updateElapsed*/)
{
    // TODO: calc

    // TODO: publish via event
}