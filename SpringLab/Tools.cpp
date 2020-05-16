/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "Tools.h"

#include "WxHelpers.h"

MoveTool::MoveTool(
    wxWindow * cursorWindow,
    std::shared_ptr<SimulationController> simulationController)
    : Tool(
        ToolType::Move,
        cursorWindow,
        std::move(simulationController))
    , mUpCursor(WxHelpers::MakeCursor("move_cursor_up", 13, 5))
    , mDownCursor(WxHelpers::MakeCursor("move_cursor_down", 13, 5))
{
}