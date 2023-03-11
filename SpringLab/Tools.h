/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include <SLabCoreLib/SimulationController.h>

#include <wx/image.h>
#include <wx/window.h>

#include <cassert>
#include <chrono>
#include <memory>
#include <optional>
#include <vector>

enum class ToolType
{
    MoveSimple = 0,
    MoveSmooth = 1,
    Pin = 2
};

struct InputState
{
    bool IsLeftMouseDown;
    bool IsRightMouseDown;
    bool IsShiftKeyDown;
    vec2f MousePosition;
    vec2f PreviousMousePosition;

    InputState()
        : IsLeftMouseDown(false)
        , IsRightMouseDown(false)
        , IsShiftKeyDown(false)
        , MousePosition()
        , PreviousMousePosition()
    {
    }
};

/*
 * Base abstract class of all tools.
 */
class Tool
{
public:

    virtual ~Tool() {}

    ToolType GetToolType() const { return mToolType; }

    virtual void Initialize(InputState const & inputState) = 0;
    virtual void Deinitialize(InputState const & inputState) = 0;
    virtual void SetCurrentCursor() = 0;

    virtual void Update(InputState const & inputState) = 0;

protected:

    Tool(
        ToolType toolType,
        wxWindow * cursorWindow,
        std::shared_ptr<SimulationController> simulationController)
        : mCursorWindow(cursorWindow)
        , mSimulationController(std::move(simulationController))
        , mToolType(toolType)
    {}

    wxWindow * const mCursorWindow;
    std::shared_ptr<SimulationController> const mSimulationController;

private:

    ToolType const mToolType;
};


//////////////////////////////////////////////////////////////////////////////////////////
// Tools
//////////////////////////////////////////////////////////////////////////////////////////

class MoveSimpleTool final : public Tool
{
public:

    MoveSimpleTool(
        wxWindow * cursorWindow,
        std::shared_ptr<SimulationController> simulationController);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        mCurrentEngagementState.reset();

        // Set cursor
        SetCurrentCursor();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        if (mCurrentEngagementState.has_value())
        {
            mSimulationController->SetPointHighlight(mCurrentEngagementState->PointIndex, 0.0f);
        }
    }

    virtual void SetCurrentCursor() override
    {
        mCursorWindow->SetCursor(!!mCurrentEngagementState ? mDownCursor : mUpCursor);
    }

    virtual void Update(InputState const & inputState) override
    {
        bool const wasEngaged = !!mCurrentEngagementState;

        if (inputState.IsLeftMouseDown)
        {
            if (!mCurrentEngagementState)
            {
                //
                // Not engaged...
                // ...see if we're able to pick a point and thus start engagement
                //

                vec2f const mousePosition = inputState.MousePosition;

                auto const elementId = mSimulationController->GetNearestPointAt(mousePosition);
                if (elementId.has_value())
                {
                    //
                    // Engage!
                    //

                    bool const isPointAlreadyFrozen = mSimulationController->IsPointFrozen(*elementId);

                    mCurrentEngagementState.emplace(
                        *elementId,
                        isPointAlreadyFrozen,
                        inputState.MousePosition);

                    if (!isPointAlreadyFrozen)
                    {
                        mSimulationController->TogglePointFreeze(*elementId);
                    }

                    mSimulationController->SetPointHighlight(mCurrentEngagementState->PointIndex, 1.0f);
                }
            }
            else
            {
                //
                // Engaged
                //

                vec2f const screenStride = inputState.MousePosition - mCurrentEngagementState->LastScreenPosition;
                mSimulationController->MovePointBy(
                    mCurrentEngagementState->PointIndex,
                    screenStride);

                mCurrentEngagementState->LastScreenPosition = inputState.MousePosition;
            }
        }
        else
        {
            if (mCurrentEngagementState)
            {
                mSimulationController->SetPointHighlight(mCurrentEngagementState->PointIndex, 0.0f);

                if (!mCurrentEngagementState->WasPointAlreadyFrozen)
                {
                    mSimulationController->TogglePointFreeze(mCurrentEngagementState->PointIndex);
                }

                // Disengage
                mCurrentEngagementState.reset();
            }
        }

        if (!!mCurrentEngagementState != wasEngaged)
        {
            // State change

            // Update cursor
            SetCurrentCursor();
        }
    }

private:

    // Our state

    struct EngagementState
    {
        ElementIndex PointIndex;
        bool const WasPointAlreadyFrozen;
        vec2f LastScreenPosition;

        EngagementState(
            ElementIndex pointIndex,
            bool wasPointAlreadyFrozen,
            vec2f startScreenPosition)
            : PointIndex(pointIndex)
            , WasPointAlreadyFrozen(wasPointAlreadyFrozen)
            , LastScreenPosition(startScreenPosition)
        {}
    };

    std::optional<EngagementState> mCurrentEngagementState; // When set, indicates it's engaged

    // The cursors
    wxCursor const mUpCursor;
    wxCursor const mDownCursor;
};

class MoveSmoothTool final : public Tool
{
public:

    MoveSmoothTool(
        wxWindow * cursorWindow,
        std::shared_ptr<SimulationController> simulationController);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        mCurrentEngagementState.reset();

        // Set cursor
        SetCurrentCursor();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
        if (mCurrentEngagementState.has_value())
        {
            mSimulationController->SetPointHighlight(mCurrentEngagementState->PointIndex, 0.0f);
        }
    }

    virtual void SetCurrentCursor() override
    {
        mCursorWindow->SetCursor(!!mCurrentEngagementState ? mDownCursor : mUpCursor);
    }

    virtual void Update(InputState const & inputState) override
    {
        bool const wasEngaged = !!mCurrentEngagementState;

        if (inputState.IsLeftMouseDown)
        {
            if (!mCurrentEngagementState)
            {
                //
                // Not engaged...
                // ...see if we're able to pick a point and thus start engagement
                //

                vec2f const mousePosition = inputState.MousePosition;

                auto const elementId = mSimulationController->GetNearestPointAt(mousePosition);
                if (elementId.has_value())
                {
                    //
                    // Engage!
                    //

                    vec2f const pointScreenPosition = mSimulationController->GetPointPositionInScreenCoordinates(*elementId);

                    mCurrentEngagementState.emplace(
                        *elementId,
                        inputState.MousePosition,
                        pointScreenPosition - mousePosition);

                    mSimulationController->SetPointHighlight(mCurrentEngagementState->PointIndex, 1.0f);
                }
            }
            else
            {
                //
                // Engaged
                //

                // 1. Update target position
                mCurrentEngagementState->TargetScreenPosition = inputState.MousePosition;

                // 2. Calculate convergence speed based on speed of mouse move

                vec2f const mouseMovementStride = inputState.MousePosition - mCurrentEngagementState->LastScreenPosition;
                float const worldStride = mSimulationController->ScreenOffsetToWorldOffset(mouseMovementStride).length();

                // New convergence rate:
                // - Stride < 2.0: 0.03 (76 steps to 0.9 of target)
                // - Stride > 20.0: 0.09 (<20 steps to 0.9 of target)
                float constexpr MinConvRate = 0.03f;
                float constexpr MaxConvRate = 0.1f;
                float const newConvergenceSpeed =
                    MinConvRate
                    + (MaxConvRate - MinConvRate) * SmoothStep(2.0f, 20.0f, worldStride);

                // Change current convergence rate depending on how much mouse has moved
                // - Small mouse movement: old speed
                // - Large mouse movement: new speed
                float newSpeedAlpha = SmoothStep(0.0f, 3.0, mouseMovementStride.length());
                mCurrentEngagementState->CurrentConvergenceSpeed = Mix(
                    mCurrentEngagementState->CurrentConvergenceSpeed,
                    newConvergenceSpeed,
                    newSpeedAlpha);

                // Update last mouse position
                mCurrentEngagementState->LastScreenPosition = inputState.MousePosition;

                // 3. Converge towards target position
                mCurrentEngagementState->CurrentScreenPosition +=
                    (mCurrentEngagementState->TargetScreenPosition - mCurrentEngagementState->CurrentScreenPosition)
                    * mCurrentEngagementState->CurrentConvergenceSpeed;

                // 4. Move point to current position
                mSimulationController->MovePointTo(
                    mCurrentEngagementState->PointIndex,
                    mCurrentEngagementState->CurrentScreenPosition + mCurrentEngagementState->PickScreenOffset);
            }
        }
        else
        {
            if (mCurrentEngagementState)
            {
                mSimulationController->SetPointHighlight(mCurrentEngagementState->PointIndex, 0.0f);

                // Disengage
                mCurrentEngagementState.reset();
            }
        }

        if (!!mCurrentEngagementState != wasEngaged)
        {
            // State change

            // Update cursor
            SetCurrentCursor();
        }
    }

private:

    // Our state

    struct EngagementState
    {
        ElementIndex PointIndex;
        vec2f CurrentScreenPosition;
        vec2f TargetScreenPosition;
        vec2f LastScreenPosition;
        float CurrentConvergenceSpeed;

        vec2f PickScreenOffset;

        EngagementState(
            ElementIndex pointIndex,
            vec2f startScreenPosition,
            vec2f pickScreenOffset)
            : PointIndex(pointIndex)
            , CurrentScreenPosition(startScreenPosition)
            , TargetScreenPosition(startScreenPosition)
            , LastScreenPosition(startScreenPosition)
            , CurrentConvergenceSpeed(0.03f)
            , PickScreenOffset(pickScreenOffset)
        {}
    };

    std::optional<EngagementState> mCurrentEngagementState; // When set, indicates it's engaged

    // The cursors
    wxCursor const mUpCursor;
    wxCursor const mDownCursor;
};

class PinTool final : public Tool
{
public:

    PinTool(
        wxWindow * cursorWindow,
        std::shared_ptr<SimulationController> simulationController);

public:

    virtual void Initialize(InputState const & /*inputState*/) override
    {
        mCurrentlyEngagedElement.reset();

        // Set cursor
        SetCurrentCursor();
    }

    virtual void Deinitialize(InputState const & /*inputState*/) override
    {
    }

    virtual void SetCurrentCursor() override
    {
        mCursorWindow->SetCursor(mCursor);
    }

    virtual void Update(InputState const & inputState) override
    {
        if (inputState.IsLeftMouseDown)
        {
            vec2f const mousePosition = inputState.MousePosition;
            auto const elementId = mSimulationController->GetNearestPointAt(mousePosition);
            if (elementId.has_value())
            {
                // Check if different than previous
                if (!mCurrentlyEngagedElement || *mCurrentlyEngagedElement != *elementId)
                {
                    //
                    // New engagement !
                    //

                    mSimulationController->TogglePointFreeze(*elementId);
                    mCurrentlyEngagedElement = *elementId;
                }
            }
        }
        else
        {
            mCurrentlyEngagedElement.reset();
        }
    }

private:

    // Our state
    std::optional<ElementIndex> mCurrentlyEngagedElement; // When set, indicates it's engaged

    // The cursor
    wxCursor const mCursor;
};