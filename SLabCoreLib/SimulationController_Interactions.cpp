/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-16
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SimulationController.h"

#include <limits>

// Interaction constants
float constexpr PointSearchRadius = 0.5f;

void SimulationController::SetPointHighlight(ElementIndex pointElementIndex, float highlight)
{
    assert(!!mObject);

    mObject->GetPoints().SetRenderHighlight(pointElementIndex, highlight);
}

std::optional<ElementIndex> SimulationController::GetNearestPointAt(vec2f const & screenCoordinates) const
{
    assert(!!mObject);

    //
    // Find closest point within the radius
    //

    vec2f const worldCoordinates = ScreenToWorld(screenCoordinates);

    float constexpr SquareSearchRadius = PointSearchRadius * PointSearchRadius;

    float bestSquareDistance = std::numeric_limits<float>::max();
    ElementIndex bestPoint = NoneElementIndex;

    auto const & points = mObject->GetPoints();
    for (auto p : points)
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

vec2f SimulationController::GetPointPosition(ElementIndex pointElementIndex) const
{
    assert(!!mObject);

    return mObject->GetPoints().GetPosition(pointElementIndex);
}

vec2f SimulationController::GetPointPositionInScreenCoordinates(ElementIndex pointElementIndex) const
{
    assert(!!mRenderContext);

    return mRenderContext->WorldToScreen(GetPointPosition(pointElementIndex));
}

void SimulationController::MovePointTo(ElementIndex pointElementIndex, vec2f const & screenCoordinates)
{
    assert(!!mObject);

    vec2f const worldCoordinates = ScreenToWorld(screenCoordinates);

    mObject->GetPoints().SetPosition(pointElementIndex, worldCoordinates);
}

void SimulationController::QueryNearestPointAt(vec2f const & screenCoordinates) const
{
    assert(!!mObject);

    auto const nearestPoint = GetNearestPointAt(screenCoordinates);
    if (nearestPoint.has_value())
    {
        mObject->GetPoints().Query(*nearestPoint);
    }
}