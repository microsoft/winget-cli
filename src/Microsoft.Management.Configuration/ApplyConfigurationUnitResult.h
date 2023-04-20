// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ApplyConfigurationUnitResult.g.h"
#include <atomic>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplyConfigurationUnitResult : ApplyConfigurationUnitResultT<ApplyConfigurationUnitResult>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using ConfigurationUnitResultInformation = Configuration::ConfigurationUnitResultInformation;

        ApplyConfigurationUnitResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Unit(ConfigurationUnit value);
        void State(ConfigurationUnitState value);
        void PreviouslyInDesiredState(bool value);
        void RebootRequired(bool value);
        void ResultInformation(ConfigurationUnitResultInformation value);
#endif

        ConfigurationUnit Unit();
        ConfigurationUnitState State() const;
        bool PreviouslyInDesiredState() const;
        bool RebootRequired() const;
        ConfigurationUnitResultInformation ResultInformation();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnit m_unit = nullptr;
        std::atomic<ConfigurationUnitState> m_state = ConfigurationUnitState::Pending;
        bool m_previouslyInDesiredState = false;
        bool m_rebootRequired = false;
        ConfigurationUnitResultInformation m_resultInformation = nullptr;
#endif
    };
}
