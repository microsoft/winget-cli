// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetParser.h"
#include "ConfigurationParameter.h"
#include <winrt/Microsoft.Management.Configuration.h>

#include <winget/Yaml.h>
#include <optional>
#include <utility>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Parser for schema version 0.3
    struct ConfigurationSetParser_0_3 : public ConfigurationSetParser
    {
        ConfigurationSetParser_0_3() = default;

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
        // Sets (or resets) the document to parse.
        void SetDocument(AppInstaller::YAML::Node&& document) override;

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

        // Extracts the environment configuration from the given metadata.
        // This only examines the winget subnode.
        void ExtractEnvironmentFromMetadata(const Windows::Foundation::Collections::ValueSet& metadata, ConfigurationEnvironment& targetEnvironment);

        // Extracts the environment for a unit.
        void ExtractEnvironmentForUnit(ConfigurationUnit* unit);

        AppInstaller::YAML::Node m_document;
    };

    std::optional<std::pair<Windows::Foundation::PropertyType, bool>> ParseWindowsFoundationPropertyType(std::string_view value);
    std::string_view ToString(Windows::Foundation::PropertyType value, bool isSecure);
}
