// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetUtilities.h"
#include <AppInstallerStrings.h>

using namespace std::string_view_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    std::string_view GetConfigurationFieldName(ConfigurationFieldName fieldName)
    {
        switch (fieldName)
        {
        case ConfigurationFieldName::ConfigurationVersion: return "configurationVersion"sv;
        case ConfigurationFieldName::Properties: return "properties"sv;
        case ConfigurationFieldName::Resource: return "resource"sv;
        case ConfigurationFieldName::Directives: return "directives"sv;
        case ConfigurationFieldName::Settings: return "settings"sv;
        case ConfigurationFieldName::Assertions: return "assertions"sv;
        case ConfigurationFieldName::Id: return "id"sv;
        case ConfigurationFieldName::DependsOn: return "dependsOn"sv;

        case ConfigurationFieldName::Resources: return "resources"sv;
        case ConfigurationFieldName::ModuleDirective: return "module"sv;

        case ConfigurationFieldName::Schema: return "$schema"sv;
        case ConfigurationFieldName::Metadata: return "metadata"sv;
        case ConfigurationFieldName::Parameters: return "parameters"sv;
        case ConfigurationFieldName::Variables: return "variables"sv;
        case ConfigurationFieldName::Type: return "type"sv;
        case ConfigurationFieldName::Description: return "description"sv;
        case ConfigurationFieldName::Name: return "name"sv;
        case ConfigurationFieldName::IsGroupMetadata: return "isGroup"sv;
        case ConfigurationFieldName::DefaultValue: return "defaultValue"sv;
        case ConfigurationFieldName::AllowedValues: return "allowedValues"sv;
        case ConfigurationFieldName::MinimumLength: return "minLength"sv;
        case ConfigurationFieldName::MaximumLength: return "maxLength"sv;
        case ConfigurationFieldName::MinimumValue: return "minValue"sv;
        case ConfigurationFieldName::MaximumValue: return "maxValue"sv;
        }

        THROW_HR(E_UNEXPECTED);
    }

    hstring GetConfigurationFieldNameHString(ConfigurationFieldName fieldName)
    {
        return hstring{ AppInstaller::Utility::ConvertToUTF16(GetConfigurationFieldName(fieldName)) };
    }
}
