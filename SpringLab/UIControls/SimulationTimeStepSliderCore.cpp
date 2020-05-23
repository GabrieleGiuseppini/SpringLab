/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#include "SimulationTimeStepSliderCore.h"

#include <cassert>

float constexpr BaseTimeStep = 0.02f;
int constexpr NumberOfTicks = 65;

SimulationTimeStepSliderCore::SimulationTimeStepSliderCore()
{
}

int SimulationTimeStepSliderCore::GetNumberOfTicks() const
{
    return NumberOfTicks;
}

float SimulationTimeStepSliderCore::TickToValue(int tick) const
{
    if (tick >= (NumberOfTicks / 2))
    {
        return BaseTimeStep * static_cast<float>(tick - NumberOfTicks / 2 + 1);
    }
    else
    {
        return BaseTimeStep / static_cast<float>(NumberOfTicks / 2 - tick + 1);
    }
}

int SimulationTimeStepSliderCore::ValueToTick(float value) const
{
    if (value >= BaseTimeStep)
    {
        return
            NumberOfTicks / 2
            + static_cast<int>(value / BaseTimeStep) - 1;
    }
    else
    {
        return
            NumberOfTicks / 2 + 1
            - static_cast<int>(BaseTimeStep / value);
    }
}

float const & SimulationTimeStepSliderCore::GetMinValue() const
{
    static float const MinValue = BaseTimeStep / static_cast<float>(NumberOfTicks / 2);
    return MinValue;
}

float const & SimulationTimeStepSliderCore::GetMaxValue() const
{
    static float const MaxValue = BaseTimeStep * static_cast<float>(NumberOfTicks / 2 + 1);
    return MaxValue;
}