// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetParser.h"
#include <string_view>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Parser object that only indicates an error occurred.
    struct ConfigurationSetParserError : public ConfigurationSetParser
    {
        ConfigurationSetParserError(hresult result, std::string_view field = {})
        {
            SetError(result, field);
        }

        std::vector<Configuration::ConfigurationUnit> GetConfigurationUnits() override { return {}; }
    };
}
