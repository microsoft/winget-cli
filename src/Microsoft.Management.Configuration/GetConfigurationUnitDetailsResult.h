// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "GetConfigurationUnitDetailsResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct GetConfigurationUnitDetailsResult : GetConfigurationUnitDetailsResultT<GetConfigurationUnitDetailsResult>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using ConfigurationUnitResultInformation = Configuration::ConfigurationUnitResultInformation;

        GetConfigurationUnitDetailsResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Unit(ConfigurationUnit value);
        void ResultInformation(ConfigurationUnitResultInformation value);
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
