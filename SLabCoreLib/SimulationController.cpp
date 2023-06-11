/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SimulationController.h"

#include "Chronometer.h"
#include "ObjectBuilder.h"
#include "ResourceLocator.h"

#include "Simulator/Common/SimulatorRegistry.h"

#include <sstream>

std::unique_ptr<SimulationController> SimulationController::Create(
    int initialCanvasWidth,
    int initialCanvasHeight)
{
    LogMessage("InitialCanvasSize: ", initialCanvasWidth, "x", initialCanvasHeight);

    // Load materials
    StructuralMaterialDatabase structuralMaterialDatabase = StructuralMaterialDatabase::Load();

    // Create render context
    std::unique_ptr<RenderContext> renderContext = std::make_unique<RenderContext>(
        initialCanvasWidth,
        initialCanvasHeight);

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
    , mThreadManager(false, 1) // Initial parallelism=1, we allow user to change later
    , mStructuralMaterialDatabase(std::move(structuralMaterialDatabase))
    // Simulation state
    , mSimulator()
    , mCurrentSimulatorTypeName(SimulatorRegistry::GetDefaultSimulatorTypeName())
    , mCurrentSimulationTime(0.0f)
    , mSimulationParameters()
    , mObject()
    , mCurrentObjectName()
    , mCurrentObjectDefinitionSource()
    , mIsSimulationStateDirty(false)
    // Parameters
    , mDoRenderAssignedParticleForces(false)
    // Stats
    , mPerfStats()
{    
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
            mStructuralMaterialDatabase,
            SimulatorRegistry::GetLayoutOptimizer(mCurrentSimulatorTypeName)));

    //
    // No errors, so we may continue
    //

    Reset(
        std::move(newObject),
        objectName,
        ObjectDefinitionSource(
            ObjectDefinitionSource::SourceType::File,
            objectDefinitionFilepath,
            0));
}

void SimulationController::MakeObject(size_t numSprings)
{
    // Create a new object
    auto newObject = std::make_unique<Object>(
        ObjectBuilder::MakeSynthetic(
            numSprings,
            mStructuralMaterialDatabase,
            SimulatorRegistry::GetLayoutOptimizer(mCurrentSimulatorTypeName)));

    std::stringstream ss;
    ss << "SynthObject (" << numSprings << ")";

    //
    // No errors, so we may continue
    //

    Reset(
        std::move(newObject),
        ss.str(),
        ObjectDefinitionSource(
            ObjectDefinitionSource::SourceType::Synthetic,
            "",
            numSprings));
}

void SimulationController::UpdateSimulation()
{
    assert(!!mSimulator);
    assert(!!mObject);

    PerfStats const lastPerfStats = mPerfStats;

    ////////////////////////////////////////////////////////
    // Update parameters
    ////////////////////////////////////////////////////////

    if (mIsSimulationStateDirty)
    {
        mSimulator->OnStateChanged(*mObject, mSimulationParameters, mThreadManager);

        mIsSimulationStateDirty = false;
    }

    ////////////////////////////////////////////////////////
    // Update
    ////////////////////////////////////////////////////////

    auto const updateStartTimestamp = Chronometer::now();

    // Update simulation
    mSimulator->Update(
        *mObject,
        mCurrentSimulationTime,
        mSimulationParameters,
        mThreadManager);

    mPerfStats.SimulationDuration.Update(std::chrono::duration_cast<std::chrono::nanoseconds>(Chronometer::now() - updateStartTimestamp));

    // Update simulation time    
    mCurrentSimulationTime += mSimulationParameters.Common.SimulationTimeStepDuration;

    ////////////////////////////////////////////////////////
    // Observe
    ////////////////////////////////////////////////////////

    ObserveObject(lastPerfStats);
}

void SimulationController::Render()
{
    assert(!!mRenderContext);

    mRenderContext->RenderStart();

    if (mObject)
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

void SimulationController::Reset()
{
    assert(mCurrentObjectDefinitionSource);

    switch (mCurrentObjectDefinitionSource->Type)
    {
        case ObjectDefinitionSource::SourceType::File:
        {
            LoadObject(mCurrentObjectDefinitionSource->DefinitionFilePath);
            break;
        }

        case ObjectDefinitionSource::SourceType::Synthetic:
        {
            MakeObject(mCurrentObjectDefinitionSource->NumSprings);
            break;
        }
    }
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
    ObjectDefinitionSource && currentObjectDefinitionSource)
{
    //
    // Take object in
    //

    mObject = std::move(newObject);
    mCurrentObjectName = objectName;
    mCurrentObjectDefinitionSource = std::move(currentObjectDefinitionSource);

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
    mSimulator = SimulatorRegistry::MakeSimulator(mCurrentSimulatorTypeName, *mObject, mSimulationParameters, mThreadManager);

    // Reset simulation state
    mCurrentSimulationTime = 0.0f;
    mIsSimulationStateDirty = false;

    // Publish reset
    mEventDispatcher.OnSimulationReset(mObject->GetSprings().GetElementCount());

    //
    // Reset stats
    //

    mPerfStats.Reset();
}

void SimulationController::ObserveObject(PerfStats const & lastPerfStats)
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

        // TODOHERE: we can only do this ourselves if MaterialStiffness is all that there is
        totalPotentialEnergy +=
            mSimulationParameters.ClassicSimulator.SpringStiffnessCoefficient
            * springs.GetMaterialStiffness(s)
            * abs(displacementLength - springs.GetRestLength(s));
    }

    totalKineticEnergy *= 0.5f;
    totalPotentialEnergy *= 0.5f;

    //
    // Bending
    //

    std::optional<float> bending;

    auto const & bendingProbe = mObject->GetPoints().GetBendingProbe();
    if (bendingProbe)
    {
        vec2f const & currentProbePosition = mObject->GetPoints().GetPosition(bendingProbe->PointIndex);

        // TODOHERE: arc?
        bending = -(currentProbePosition.y - bendingProbe->OriginalWorldCoordinates.y);
    }

    //
    // Update perf
    //

    auto const deltaStats = mPerfStats - lastPerfStats;

    //
    // Publish observations
    //

    mEventDispatcher.OnMeasurement(
        totalKineticEnergy,
        totalPotentialEnergy,
        bending,
        deltaStats.SimulationDuration.Finalize<std::chrono::nanoseconds>(),
        mPerfStats.SimulationDuration.Finalize<std::chrono::nanoseconds>());
}
