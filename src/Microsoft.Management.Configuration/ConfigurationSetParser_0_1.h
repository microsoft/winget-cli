// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetParser.h"

#include <winget/Yaml.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Parser for schema version 0.1
    struct ConfigurationSetParser_0_1 : public ConfigurationSetParser
    {
        ConfigurationSetParser_0_1() = default;

        virtual ~ConfigurationSetParser_0_1() noexcept = default;

        ConfigurationSetParser_0_1(const ConfigurationSetParser_0_1&) = delete;
        ConfigurationSetParser_0_1& operator=(const ConfigurationSetParser_0_1&) = delete;
        ConfigurationSetParser_0_1(ConfigurationSetParser_0_1&&) = default;
        ConfigurationSetParser_0_1& operator=(ConfigurationSetParser_0_1&&) = default;

        void Parse() override;

        // Retrieves the schema version of the parser.
        hstring GetSchemaVersion() override;

    protected:
        // Sets (or resets) the document to parse.
        void SetDocument(AppInstaller::YAML::Node&& document) override;

        void ParseConfigurationUnitsFromField(const AppInstaller::YAML::Node& document, ConfigurationField field, ConfigurationUnitIntent intent, std::vector<Configuration::ConfigurationUnit>& result);
        virtual void ParseConfigurationUnit(ConfigurationUnit* unit, const AppInstaller::YAML::Node& unitNode, ConfigurationUnitIntent intent);

        AppInstaller::YAML::Node m_document;
    };
}
