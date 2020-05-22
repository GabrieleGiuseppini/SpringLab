/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2020-05-23
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "ISliderCore.h"

class ExponentialSliderCore final : public ISliderCore<float>
{
public:

    ExponentialSliderCore(
        float minValue,
        float zeroValue,
        float maxValue);

    virtual int GetNumberOfTicks() const override;

    virtual float TickToValue(int tick) const override;

    virtual int ValueToTick(float value) const override;

    virtual float const & GetMinValue() const override
    {
        return mMinValue;
    }

    virtual float const & GetMaxValue() const override
    {
        return mMaxValue;
    }

private:

    float const mMinValue;
    float const mZeroValue;
    float const mMaxValue;

    float const mLowerA;
    float const mLowerB;
    float const mUpperA;
    float const mUpperB;
};
