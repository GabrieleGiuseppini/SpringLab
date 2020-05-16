/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "WxHelpers.h"

#include <SLabCoreLib/ResourceLocator.h>
#include <SLabCoreLib/SLabException.h>

#include <wx/rawbmp.h>

#include <memory>
#include <stdexcept>

wxCursor WxHelpers::LoadCursor(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY)
{
    wxImage img = LoadCursorImage(
        cursorName,
        hotspotX,
        hotspotY);

    return wxCursor(img);
}

wxImage WxHelpers::LoadCursorImage(
    std::string const & cursorName,
    int hotspotX,
    int hotspotY)
{
    auto filepath = ResourceLocator::GetResourcesFolderPath() / (cursorName + ".png");
    auto bmp = std::make_unique<wxBitmap>(filepath.string(), wxBITMAP_TYPE_PNG);

    wxImage img = bmp->ConvertToImage();

    // Set hotspots
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_X, hotspotX);
    img.SetOption(wxIMAGE_OPTION_CUR_HOTSPOT_Y, hotspotY);

    return img;
}