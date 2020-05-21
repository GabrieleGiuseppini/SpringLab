/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "ClassicSimulator.h"

void ClassicSimulator::Update(
    Object & object,
    float currentSimulationTime,
    SimulationParameters const & simulationParameters)
{
    // TODOTEST
    Points & points = object.GetPoints();
    for (auto pointIndex : points)
    {
        vec2f newPosition = points.GetPosition(pointIndex);
        newPosition += vec2f(0.0f, 0.1f).rotate(currentSimulationTime + static_cast<float>(pointIndex));
        points.SetPosition(
            pointIndex,
            newPosition);
    }
}