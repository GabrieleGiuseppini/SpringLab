/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISliderCore.h"

class SimulationTimeStepSliderCore final : public ISliderCore<float>
{
public:

    SimulationTimeStepSliderCore();

    virtual int GetNumberOfTicks() const override;

    virtual float TickToValue(int tick) const override;

    virtual int ValueToTick(float value) const override;

    virtual float const & GetMinValue() const override;

    virtual float const & GetMaxValue() const override;
};
