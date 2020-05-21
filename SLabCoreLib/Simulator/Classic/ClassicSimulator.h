/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-21
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include <string>

class ClassicSimulator final : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "Classic";
    }

public:

    //////////////////////////////////////////////////////////
    // ISimulator
    //////////////////////////////////////////////////////////

private:

};