/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2020-05-16
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ResourceLocator.h"
#include "StructuralMaterial.h"

#include "Colors.h"
#include "SLabException.h"
#include "Utils.h"

#include <picojson.h>

#include <cassert>
#include <cstdint>
#include <map>

class StructuralMaterialDatabase
{
public:

    using ColorKey = rgbColor;

public:

    static StructuralMaterialDatabase Load()
    {
        picojson::value structuralMaterialsRoot = Utils::ParseJSONFile(ResourceLocator::GetStructuralMaterialDatabaseFilePath());

        if (!structuralMaterialsRoot.is<picojson::array>())
        {
            throw SLabException("Structural materials definition is not a JSON array");
        }

        //
        // Read into map
        //

        std::map<ColorKey, StructuralMaterial> structuralMaterialsMap;

        picojson::array const & structuralMaterialsRootArray = structuralMaterialsRoot.get<picojson::array>();
        for (auto const & materialElem : structuralMaterialsRootArray)
        {
            if (!materialElem.is<picojson::object>())
            {
                throw SLabException("Found a non-object in structural materials definition");
            }

            picojson::object const & materialObject = materialElem.get<picojson::object>();

            ColorKey colorKey = Utils::Hex2RgbColor(
                Utils::GetMandatoryJsonMember<std::string>(materialObject, "color_key"));

            StructuralMaterial material = StructuralMaterial::Create(materialObject);

            // Make sure there are no dupes
            if (structuralMaterialsMap.count(colorKey) != 0)
            {
                throw SLabException("Structural material \"" + material.Name + "\" has a duplicate color key");
            }

            // Store
            auto const storedEntry = structuralMaterialsMap.emplace(
                std::make_pair(
                    colorKey,
                    material));
        }

        return StructuralMaterialDatabase(std::move(structuralMaterialsMap));
    }

    StructuralMaterial const * FindStructuralMaterial(ColorKey const & colorKey) const
    {
        if (auto srchIt = mStructuralMaterialMap.find(colorKey);
            srchIt != mStructuralMaterialMap.end())
        {
            // Found color key verbatim!
            return &(srchIt->second);
        }

        // No luck
        return nullptr;
    }

private:

    explicit StructuralMaterialDatabase(std::map<ColorKey, StructuralMaterial> structuralMaterialMap)
        : mStructuralMaterialMap(std::move(structuralMaterialMap))
    {
    }

    std::map<ColorKey, StructuralMaterial> const mStructuralMaterialMap;
};
