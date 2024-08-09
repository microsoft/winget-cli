// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetParser_0_1.h"

#include <winget/Yaml.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Parser for schema version 0.2
    struct ConfigurationSetParser_0_2 : public ConfigurationSetParser_0_1
    {
        ConfigurationSetParser_0_2() = default;

        virtual ~ConfigurationSetParser_0_2() noexcept = default;

        ConfigurationSetParser_0_2(const ConfigurationSetParser_0_2&) = delete;
        ConfigurationSetParser_0_2& operator=(const ConfigurationSetParser_0_2&) = delete;
        ConfigurationSetParser_0_2(ConfigurationSetParser_0_2&&) = default;
        ConfigurationSetParser_0_2& operator=(ConfigurationSetParser_0_2&&) = default;

        // Retrieves the schema version of the parser.
        hstring GetSchemaVersion() override;

    protected:
        // Sets (or resets) the document to parse.
        void SetDocument(AppInstaller::YAML::Node&& document) override;

        void ParseConfigurationUnit(ConfigurationUnit* unit, const AppInstaller::YAML::Node& unitNode, ConfigurationUnitIntent intent) override;
    };
}
