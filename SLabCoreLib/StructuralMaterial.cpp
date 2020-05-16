/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-05-16
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#include "StructuralMaterial.h"

#include "Utils.h"

StructuralMaterial StructuralMaterial::Create(picojson::object const & structuralMaterialJson)
{
    std::string const name = Utils::GetMandatoryJsonMember<std::string>(structuralMaterialJson, "name");

    try
    {
        picojson::object massJson = Utils::GetMandatoryJsonObject(structuralMaterialJson, "mass");
        float const nominalMass = Utils::GetMandatoryJsonMember<float>(massJson, "nominal_mass");
        float const density = Utils::GetMandatoryJsonMember<float>(massJson, "density");
        float const stiffness = Utils::GetOptionalJsonMember<float>(structuralMaterialJson, "stiffness", 1.0);

        return StructuralMaterial(
            name,
            nominalMass,
            density,
            stiffness);
    }
    catch (SLabException const & ex)
    {
        throw SLabException(std::string("Error parsing structural material \"") + name + "\": " + ex.what());
    }
}