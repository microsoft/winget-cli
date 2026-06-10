// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "GetConfigurationUnitDetailsResult.g.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct GetConfigurationUnitDetailsResult : GetConfigurationUnitDetailsResultT<GetConfigurationUnitDetailsResult>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;

        GetConfigurationUnitDetailsResult() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Unit(ConfigurationUnit value);
        void Details(IConfigurationUnitProcessorDetails value);
        void ResultInformation(IConfigurationUnitResultInformation value);
#endif

        ConfigurationUnit Unit();
        IConfigurationUnitProcessorDetails Details();
        IConfigurationUnitResultInformation ResultInformation();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnit m_unit = nullptr;
        IConfigurationUnitProcessorDetails m_details;
        IConfigurationUnitResultInformation m_resultInformation;
#endif
    };
}
