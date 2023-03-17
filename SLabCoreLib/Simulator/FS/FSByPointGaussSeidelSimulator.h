/***************************************************************************************
* Original Author:      Gabriele Giuseppini
* Created:              2023-03-12
* Copyright:            Gabriele Giuseppini  (https://github.com/GabrieleGiuseppini)
***************************************************************************************/
#pragma once

#include "Simulator/Common/ISimulator.h"

#include <cstdint>
#include <string>

/*
 * Simulator implementing the same spring relaxation algorithm
 * as Floating Sandbox 1.17.5, but pivoted on a by-point visit,
 * and integrating for each point - effectively implementing Gauss-Seidel.
 */
class FSByPointGaussSeidelSimulator final : public ISimulator
{
public:

    static std::string GetSimulatorName()
    {
        return "FS X - By Point, Gauss-Seidel";
    }

public:

    FSByPointGaussSeidelSimulator(
        Object const & object,
        SimulationParameters const & simulationParameters);

    //////////////////////////////////////////////////////////
    // ISimulator
    //////////////////////////////////////////////////////////

    void OnStateChanged(
        Object const & object,
        SimulationParameters const & simulationParameters) override;

    void Update(
        Object & object,
        float currentSimulationTime,
        SimulationParameters const & simulationParameters) override;

private:

    void CreateState(
        Object const & object,
        SimulationParameters const & simulationParameters);

    void ApplySpringsForcesAndIntegrate(
        Object & object,
        SimulationParameters const & simulationParameters);

private:

    //
    // Point buffers
    //

    Buffer<vec2f> mPointSpringForceBuffer;
    Buffer<vec2f> mPointExternalForceBuffer;
    Buffer<float> mPointIntegrationFactorBuffer; // dt^2/Mass or zero when the point is frozen

    struct ConnectedSpring
    {
        float StiffnessCoefficient;
        float DampingCoefficient;
        float RestLength;
        ElementIndex OtherEndpointIndex;

        ConnectedSpring()
            : StiffnessCoefficient(0.0f)
            , DampingCoefficient(0.0f)
            , RestLength(0.0f)
            , OtherEndpointIndex(NoneElementIndex)
        {}

        ConnectedSpring(
            float stiffnessCoefficient,
            float dampingCoefficient,
            float restLength,
            ElementIndex otherEndpointIndex)
            : StiffnessCoefficient(stiffnessCoefficient)
            , DampingCoefficient(dampingCoefficient)
            , RestLength(restLength)
            , OtherEndpointIndex(otherEndpointIndex)
        {}
    };

    // Connected springs:
    // - NumSprings
    // - ConnectedSpring x [0,..,MaxSpringsPerPoint]

    Buffer<std::uint8_t> mConnectedSpringsBuffer;
};