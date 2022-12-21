/***************************************************************************************
 * Original Author:		Gabriele Giuseppini
 * Created:				2020-05-16
 * Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
 ***************************************************************************************/
#pragma once

#include <picojson.h>

#include <string>

struct StructuralMaterial
{
public:

    std::string Name;
    float NominalMass;
    float Density;
    float Stiffness;

    bool IsFixed;
    bool IsBendingProbe;

public:

    static StructuralMaterial Create(picojson::object const & structuralMaterialJson);

    /*
     * Returns the mass of this particle, calculated assuming that the particle is a cubic meter
     * full of a quantity of material equal to the density; for example, an iron truss has a lower
     * density than solid iron.
     */
    float GetMass() const
    {
        return NominalMass * Density;
    }

    StructuralMaterial(
        std::string name,
        float nominalMass,
        float density,
        float stiffness,
        bool isFixed,
        bool isBendingProbe)
        : Name(name)
        , NominalMass(nominalMass)
        , Density(density)
        , Stiffness(stiffness)
        , IsFixed(isFixed)
        , IsBendingProbe(isBendingProbe)
    {}
};
