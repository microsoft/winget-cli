// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetParser.h"
#include <ConfigurationParameter.h>

#include <winget/Yaml.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Parser for schema version 0.3
    struct ConfigurationSetParser_0_3 : public ConfigurationSetParser
    {
        ConfigurationSetParser_0_3(AppInstaller::YAML::Node&& document) : m_document(std::move(document)) {}

        virtual ~ConfigurationSetParser_0_3() noexcept = default;

        ConfigurationSetParser_0_3(const ConfigurationSetParser_0_3&) = delete;
        ConfigurationSetParser_0_3& operator=(const ConfigurationSetParser_0_3&) = delete;
        ConfigurationSetParser_0_3(ConfigurationSetParser_0_3&&) = default;
        ConfigurationSetParser_0_3& operator=(ConfigurationSetParser_0_3&&) = default;

        // Retrieve the configuration units from the parser.
        void Parse() override;

        // Retrieves the schema version of the parser.
        hstring GetSchemaVersion() override;

    protected:
        void ParseParameters(ConfigurationSetParser::ConfigurationSetPtr& set);
        void ParseParameter(ConfigurationParameter* parameter, const AppInstaller::YAML::Node& node);
        void ParseParameterType(ConfigurationParameter* parameter, const AppInstaller::YAML::Node& node);
        void GetStringValueForParameter(
            const AppInstaller::YAML::Node& node,
            ConfigurationField field,
            ConfigurationParameter* parameter,
            void(ConfigurationParameter::* propertyFunction)(const hstring& value));
        void GetUInt32ValueForParameter(
            const AppInstaller::YAML::Node& node,
            ConfigurationField field,
            ConfigurationParameter* parameter,
            void(ConfigurationParameter::* propertyFunction)(uint32_t value));
        void ParseObjectValueForParameter(
            const AppInstaller::YAML::Node& node,
            ConfigurationField field,
            Windows::Foundation::PropertyType type,
            ConfigurationParameter* parameter,
            void(ConfigurationParameter::* propertyFunction)(const Windows::Foundation::IInspectable& value));

        void ParseConfigurationUnitsFromField(const AppInstaller::YAML::Node& document, ConfigurationField field, std::vector<Configuration::ConfigurationUnit>& result);
        virtual void ParseConfigurationUnit(ConfigurationUnit* unit, const AppInstaller::YAML::Node& unitNode);
        // Determines if the given unit should be converted to a group.
        bool ShouldConvertToGroup(ConfigurationUnit* unit);

        AppInstaller::YAML::Node m_document;
    };
}
