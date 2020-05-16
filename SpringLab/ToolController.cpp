/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ToolController.h"

#include "WxHelpers.h"

#include <SLabCoreLib/Vectors.h>

ToolController::ToolController(
    ToolType initialToolType,
    wxWindow * cursorWindow,
    std::shared_ptr<SimulationController> simulationController)
    : mInputState()
    , mCurrentTool(nullptr)
    , mAllTools()
    , mCursorWindow(cursorWindow)
    , mPanCursor()
    , mSimulationController(std::move(simulationController))
{
    //
    // Initialize all tools
    //

    mAllTools.emplace_back(
        std::make_unique<MoveTool>(
            mCursorWindow,
            mSimulationController));

    // Prepare own cursor(s)
    mPanCursor = WxHelpers::MakeCursor("pan_cursor", 15, 15);

    // Set current tool
    this->SetTool(initialToolType);
}

void ToolController::OnMouseMove(
    int x,
    int y,
    bool isShiftDown)
{
    // Update input state
    mInputState.PreviousMousePosition = mInputState.MousePosition;
    mInputState.MousePosition = vec2f(x, y);
    ProcessShiftState(isShiftDown);

    // Perform action
    if (mInputState.IsRightMouseDown)
    {
        // Perform our pan tool

        // Pan (opposite direction)
        vec2f screenOffset = mInputState.PreviousMousePosition - mInputState.MousePosition;
        mSimulationController->Pan(screenOffset);
    }
    else
    {
        // Perform current tool's action
        if (nullptr != mCurrentTool)
        {
            mCurrentTool->Update(mInputState);
        }
    }
}

void ToolController::OnLeftMouseDown(bool isShiftDown)
{
    // Update input state
    mInputState.IsLeftMouseDown = true;
    ProcessShiftState(isShiftDown);

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->Update(mInputState);
    }
}

void ToolController::OnLeftMouseUp(bool isShiftDown)
{
    // Update input state
    mInputState.IsLeftMouseDown = false;
    ProcessShiftState(isShiftDown);

    // Perform current tool's action
    if (nullptr != mCurrentTool)
    {
        mCurrentTool->Update(mInputState);
    }
}

void ToolController::OnRightMouseDown()
{
    // Update input state
    mInputState.IsRightMouseDown = true;

    // Show our pan cursor
    mCursorWindow->SetCursor(mPanCursor);
}

void ToolController::OnRightMouseUp()
{
    // Update input state
    mInputState.IsRightMouseDown = false;

    if (nullptr != mCurrentTool)
    {
        // Show tool's cursor again, since we moved out of Pan
        mCurrentTool->SetCurrentCursor();
    }
}

void ToolController::ProcessShiftState(bool isDown)
{
    auto oldState = mInputState.IsShiftKeyDown;

    // Update input state
    mInputState.IsShiftKeyDown = isDown;

    // Perform current tool's action
    if (oldState != isDown
        && nullptr != mCurrentTool)
    {
        mCurrentTool->Update(mInputState);
    }
}