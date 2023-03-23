/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2023-03-23
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "SLabTypes.h"

#include <vector>

struct ObjectSimulatorSpecificStructure
{
    std::vector<ElementCount> PointProcessingBlockSizes;
    std::vector<ElementCount> SpringProcessingBlockSizes;
};
