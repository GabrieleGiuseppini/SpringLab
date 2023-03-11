/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tools.h"

#include "WxHelpers.h"

MoveSimpleTool::MoveSimpleTool(
    wxWindow * cursorWindow,
    std::shared_ptr<SimulationController> simulationController)
    : Tool(
        ToolType::MoveSimple,
        cursorWindow,
        std::move(simulationController))
    , mCurrentEngagementState(std::nullopt)
    , mUpCursor(WxHelpers::MakeCursor("move_cursor_up", 13, 5))
    , mDownCursor(WxHelpers::MakeCursor("move_cursor_down", 13, 5))
{
}

MoveSmoothTool::MoveSmoothTool(
    wxWindow * cursorWindow,
    std::shared_ptr<SimulationController> simulationController)
    : Tool(
        ToolType::MoveSmooth,
        cursorWindow,
        std::move(simulationController))
    , mCurrentEngagementState(std::nullopt)
    , mUpCursor(WxHelpers::MakeCursor("move_cursor_up", 13, 5))
    , mDownCursor(WxHelpers::MakeCursor("move_cursor_down", 13, 5))
{
}

PinTool::PinTool(
    wxWindow * cursorWindow,
    std::shared_ptr<SimulationController> simulationController)
    : Tool(
        ToolType::Pin,
        cursorWindow,
        std::move(simulationController))
    , mCurrentlyEngagedElement(false)
    , mCursor(WxHelpers::MakeCursor("pin_cursor", 4, 27))
{
}