// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ApplyConfigurationUnitResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplyConfigurationUnitResult : ApplyConfigurationUnitResultT<ApplyConfigurationUnitResult>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using ConfigurationUnitResultInformation = Configuration::ConfigurationUnitResultInformation;

        ApplyConfigurationUnitResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const ConfigurationUnit& unit, bool previouslyInDesiredState, const ConfigurationUnitResultInformation& resultInformation);
#endif

        ConfigurationUnit Unit();
        bool PreviouslyInDesiredState() const;
        ConfigurationUnitResultInformation ResultInformation();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnit m_unit = nullptr;
        bool m_previouslyInDesiredState = false;
        ConfigurationUnitResultInformation m_resultInformation = nullptr;
#endif
    };
}
