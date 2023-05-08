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
        ConfigurationSetParser_0_1(AppInstaller::YAML::Node&& document) : m_document(std::move(document)) {}

        virtual ~ConfigurationSetParser_0_1() noexcept = default;

        ConfigurationSetParser_0_1(const ConfigurationSetParser_0_1&) = delete;
        ConfigurationSetParser_0_1& operator=(const ConfigurationSetParser_0_1&) = delete;
        ConfigurationSetParser_0_1(ConfigurationSetParser_0_1&&) = default;
        ConfigurationSetParser_0_1& operator=(ConfigurationSetParser_0_1&&) = default;

        // Retrieve the configuration units from the parser.
        std::vector<Configuration::ConfigurationUnit> GetConfigurationUnits() override;

        // Retrieves the schema version of the parser.
        hstring GetSchemaVersion() override;

    protected:
        void ParseConfigurationUnitsFromSubsection(const AppInstaller::YAML::Node& document, std::string_view subsection, ConfigurationUnitIntent intent, std::vector<Configuration::ConfigurationUnit>& result);
        virtual void ParseConfigurationUnit(ConfigurationUnit* unit, const AppInstaller::YAML::Node& unitNode, ConfigurationUnitIntent intent);
        void GetStringValueForUnit(const AppInstaller::YAML::Node& item, std::string_view valueName, bool required, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(const hstring& value));
        void GetStringArrayForUnit(const AppInstaller::YAML::Node& item, std::string_view arrayName, ConfigurationUnit* unit, void(ConfigurationUnit::* propertyFunction)(std::vector<hstring>&& value));
        void GetValueSet(const AppInstaller::YAML::Node& item, std::string_view mapName, bool required, const Windows::Foundation::Collections::ValueSet& valueSet);

        AppInstaller::YAML::Node m_document;
    };
}
