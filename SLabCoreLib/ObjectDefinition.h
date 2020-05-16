/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ImageData.h"

#include <filesystem>
#include <string>

/*
* The complete definition of an object.
*/
struct ObjectDefinition
{
public:

    RgbImageData StructuralLayerImage;
    std::string const ObjectName;

    static ObjectDefinition Load(std::filesystem::path const & filepath);

private:

    ObjectDefinition(
        RgbImageData structuralLayerImage,
        std::string const & objectName)
        : StructuralLayerImage(std::move(structuralLayerImage))
        , ObjectName(objectName)
    {
    }
};
