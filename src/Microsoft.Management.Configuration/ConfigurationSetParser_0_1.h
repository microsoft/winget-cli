// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetParser.h"

#include <winget/Yaml.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Interface for parsing a configuration set stream.
    struct ConfigurationSetParser_0_1 : public ConfigurationSetParser
    {
        ConfigurationSetParser_0_1(AppInstaller::YAML::Node&& document) : m_document(std::move(document)) {}

        virtual ~ConfigurationSetParser_0_1() noexcept = default;

        ConfigurationSetParser_0_1(const ConfigurationSetParser_0_1&) = delete;
        ConfigurationSetParser_0_1& operator=(const ConfigurationSetParser_0_1&) = delete;
        ConfigurationSetParser_0_1(ConfigurationSetParser_0_1&&) = default;
        ConfigurationSetParser_0_1& operator=(ConfigurationSetParser_0_1&&) = default;

        // Retrieve the configuration units from the parser.
        std::vector<Configuration::ConfigurationUnit> GetConfigurationUnits() override;

    protected:
        AppInstaller::YAML::Node m_document;
    };
}
