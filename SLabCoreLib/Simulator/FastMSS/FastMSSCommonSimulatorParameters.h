/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-05-13
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

struct FastMSSCommonSimulatorParameters
{
    FastMSSCommonSimulatorParameters();

    // Pure and simple stiffness coefficient
    float SpringStiffnessCoefficient;
    static float constexpr MinSpringStiffnessCoefficient = 0.0f;
    static float constexpr MaxSpringStiffnessCoefficient = 500000.0f;
};