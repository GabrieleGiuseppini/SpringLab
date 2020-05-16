/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ObjectDefinition.h"

#include "ImageFileTools.h"

#include <cassert>

ObjectDefinition ObjectDefinition::Load(std::filesystem::path const & filepath)
{
    ImageData structuralImage = ImageFileTools::LoadImageRgb(filepath);

    return ObjectDefinition(
        std::move(structuralImage),
        filepath.stem().string());
}