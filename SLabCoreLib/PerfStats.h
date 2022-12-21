/***************************************************************************************
* Original Author:		Gabriele Giuseppini
* Created:				2019-12-21
* Copyright:			Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Chronometer.h"

struct PerfStats
{
    struct Ratio
    {
    public:

        Ratio()
        {
            Reset();
        }

        inline void Update(Chronometer::duration duration)
        {
            mDuration += duration;
            mDenominator += 1;
        }

        template<typename TDuration>
        inline TDuration Finalize() const
        {
            if (mDenominator == 0)
                return TDuration(0);

            Chronometer::duration avgDuration(mDuration.count() / mDenominator);
            return std::chrono::duration_cast<TDuration>(avgDuration);
        }

        inline void Reset()
        {
            mDuration = Chronometer::duration::zero();
            mDenominator = 0;
        }

        friend Ratio operator-(Ratio const & lhs, Ratio const & rhs)
        {
            return Ratio(
                lhs.mDuration - rhs.mDuration,
                lhs.mDenominator - rhs.mDenominator);
        }

    private:

        Ratio(
            Chronometer::duration duration,
            size_t denominator)
            : mDuration(duration)
            , mDenominator(denominator)
        {}

        Chronometer::duration mDuration;
        size_t mDenominator;
    };

    Ratio SimulationDuration;

    PerfStats()
    {
        Reset();
    }

    void Reset()
    {
        SimulationDuration.Reset();
    }

    PerfStats & operator=(PerfStats const & other) = default;
};

inline PerfStats operator-(PerfStats const & lhs, PerfStats const & rhs)
{
    PerfStats perfStats;

    perfStats.SimulationDuration = lhs.SimulationDuration - rhs.SimulationDuration;

    return perfStats;
}