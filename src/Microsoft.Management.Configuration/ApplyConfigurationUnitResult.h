// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ApplyConfigurationUnitResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplyConfigurationUnitResult : ApplyConfigurationUnitResultT<ApplyConfigurationUnitResult>
    {
        ApplyConfigurationUnitResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const ConfigurationUnit& unit, const ConfigurationUnitResultInformation& resultInformation);
#endif

        ConfigurationUnit Unit();
        ConfigurationUnitResultInformation ResultInformation();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnit m_unit = nullptr;
        ConfigurationUnitResultInformation m_resultInformation = nullptr;
#endif
    };
}
