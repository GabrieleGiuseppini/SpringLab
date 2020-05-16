/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SLabCoreLib/ImageData.h"

#include <wx/cursor.h>
#include <wx/generic/statbmpg.h>
#include <wx/image.h>
#include <wx/wx.h>

#include <memory>

namespace WxHelpers
{
    wxCursor MakeCursor(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY);

    wxImage MakeCursorImage(
        std::string const & cursorName,
        int hotspotX,
        int hotspotY);
};
