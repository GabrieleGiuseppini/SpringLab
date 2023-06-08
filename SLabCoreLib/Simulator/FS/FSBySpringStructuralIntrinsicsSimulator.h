/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-22
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include "ILayoutOptimizer.h"

#include <memory>
#include <string>

/*
 * Simulator implementing the same spring relaxation algorithm
 * as Floating Sandbox 1.17.5, optimized with intrinsics, and
 * taking advantage of structural regularities in the object.
 */

class FSBySpringStructuralIntrinsicsLayoutOptimizer;


class FSBySpringStructuralIntrinsicsSimulator : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "FS 12 - By Spring - Structural Instrinsics";
    }

    using layout_optimizer = FSBySpringStructuralIntrinsicsLayoutOptimizer;

public:

    FSBySpringStructuralIntrinsicsSimulator(
        Object const & object,
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager);

    //////////////////////////////////////////////////////////
    // ISimulator
    //////////////////////////////////////////////////////////

    void OnStateChanged(
        Object const & object,
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager) override;

    void Update(
        Object & object,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters,
        ThreadManager & threadManager) override;

protected:

    virtual void CreateState(
        Object const & object,
        SimulationParameters const & simulationParameters,
        ThreadManager const & threadManager);

    virtual void ApplySpringsForces(
        Object const & object,
        ThreadManager & threadManager);

    void ApplySpringsForcesVectorized(
        Object const & object,
        vec2f * restrict pointSpringForceBuffer,
        ElementIndex startSpringIndex,
        ElementCount endSpringIndex);  // Excluded

    virtual void IntegrateAndResetSpringForces(
        Object & object,
        SimulationParameters const & simulationParameters);

protected:

    //
    // Point buffers
    //

    Buffer<vec2f> mPointSpringForceBuffer;
    Buffer<vec2f> mPointExternalForceBuffer;
    Buffer<vec2f> mPointIntegrationFactorBuffer; // dt^2/Mass or zero when the point is frozen; identical elements, one for x and one for y


    //
    // Spring buffers
    //

    Buffer<float> mSpringStiffnessCoefficientBuffer;
    Buffer<float> mSpringDampingCoefficientBuffer;

    // Structure
    ElementCount mSpringPerfectSquareCount;
};

class FSBySpringStructuralIntrinsicsLayoutOptimizer : public ILayoutOptimizer
{
public:

    LayoutRemap Remap(
        ObjectBuildPointIndexMatrix const & pointMatrix,
        std::vector<ObjectBuildPoint> const & points,
        std::vector<ObjectBuildSpring> const & springs) const override;

private:
};