/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SimulationController.h"

#include <limits>

// Interaction constants
float constexpr PointSearchRadius = 0.5f;

void SimulationController::SetPointHighlightState(ElementIndex pointElementIndex, bool highlightState)
{
    // TODOHERE
}

std::optional<ElementIndex> SimulationController::GetNearestPointAt(vec2f const & screenCoordinates) const
{
    //
    // Find closest point within the radius
    //

    vec2f const worldCoordinates = ScreenToWorld(screenCoordinates);

    float constexpr SquareSearchRadius = PointSearchRadius * PointSearchRadius;

    float bestSquareDistance = std::numeric_limits<float>::max();
    ElementIndex bestPoint = NoneElementIndex;

    auto const & points = mObject->GetPoints();
    for (auto p : points.RawPoints())
    {
        float const squareDistance = (points.GetPosition(p) - worldCoordinates).squareLength();
        if (squareDistance < SquareSearchRadius
            && squareDistance < bestSquareDistance)
        {
            bestSquareDistance = squareDistance;
            bestPoint = p;
        }
    }

    if (bestPoint != NoneElementIndex)
        return bestPoint;
    else
        return std::nullopt;
}

void SimulationController::MovePoint(ElementIndex pointElementIndex, vec2f const & screenCoordinates)
{
    vec2f const worldCoordinates = ScreenToWorld(screenCoordinates);

    mObject->GetPoints().GetPosition(pointElementIndex) = worldCoordinates;
}

void SimulationController::QueryNearestPointAt(vec2f const & screenCoordinates) const
{
    auto const nearestPoint = GetNearestPointAt(screenCoordinates);
    if (nearestPoint.has_value())
    {
        mObject->GetPoints().Query(*nearestPoint);
    }
}