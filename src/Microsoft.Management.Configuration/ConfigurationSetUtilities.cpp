// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetUtilities.h"
#include <AppInstallerStrings.h>

using namespace std::string_view_literals;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    std::string_view GetConfigurationFieldName(ConfigurationField fieldName)
    {
        switch (fieldName)
        {
        case ConfigurationField::ConfigurationVersion: return "configurationVersion"sv;
        case ConfigurationField::Properties: return "properties"sv;
        case ConfigurationField::Resource: return "resource"sv;
        case ConfigurationField::Directives: return "directives"sv;
        case ConfigurationField::Settings: return "settings"sv;
        case ConfigurationField::Assertions: return "assertions"sv;
        case ConfigurationField::Id: return "id"sv;
        case ConfigurationField::DependsOn: return "dependsOn"sv;

        case ConfigurationField::Resources: return "resources"sv;
        case ConfigurationField::ModuleDirective: return "module"sv;
        case ConfigurationField::SecurityContextDirective: return "securityContext"sv;

        case ConfigurationField::Schema: return "$schema"sv;
        case ConfigurationField::Metadata: return "metadata"sv;
        case ConfigurationField::Parameters: return "parameters"sv;
        case ConfigurationField::Variables: return "variables"sv;
        case ConfigurationField::Type: return "type"sv;
        case ConfigurationField::Description: return "description"sv;
        case ConfigurationField::Name: return "name"sv;
        case ConfigurationField::IsGroupMetadata: return "isGroup"sv;
        case ConfigurationField::DefaultValue: return "defaultValue"sv;
        case ConfigurationField::AllowedValues: return "allowedValues"sv;
        case ConfigurationField::MinimumLength: return "minLength"sv;
        case ConfigurationField::MaximumLength: return "maxLength"sv;
        case ConfigurationField::MinimumValue: return "minValue"sv;
        case ConfigurationField::MaximumValue: return "maxValue"sv;
        }

        THROW_HR(E_UNEXPECTED);
    }

    hstring GetConfigurationFieldNameHString(ConfigurationField fieldName)
    {
        return hstring{ AppInstaller::Utility::ConvertToUTF16(GetConfigurationFieldName(fieldName)) };
    }

    bool TryParseSecurityContext(winrt::hstring value, SecurityContext& result)
    {
        std::wstring securityContextLower = AppInstaller::Utility::ToLower(value);

        if (securityContextLower == L"elevated")
        {
            result = SecurityContext::Elevated;
        }
        else if (securityContextLower == L"restricted")
        {
            result = SecurityContext::Restricted;
        }
        else if (securityContextLower == L"current")
        {
            result = SecurityContext::Current;
        }
        else
        {
            return false;
        }

        return true;
    }

    SecurityContext ParseSecurityContext(hstring value)
    {
        SecurityContext result = SecurityContext::Current;
        THROW_HR_IF(E_INVALIDARG, !TryParseSecurityContext(value, result));
        return result;
    }

    std::string_view ToString(SecurityContext value)
    {
        switch (value)
        {
        case SecurityContext::Current: return "current";
        case SecurityContext::Restricted: return "restricted";
        case SecurityContext::Elevated: return "elevated";
        }

        THROW_HR(E_INVALIDARG);
    }

    std::wstring_view ToWString(SecurityContext value)
    {
        switch (value)
        {
        case SecurityContext::Current: return L"current";
        case SecurityContext::Restricted: return L"restricted";
        case SecurityContext::Elevated: return L"elevated";
        }

        THROW_HR(E_INVALIDARG);
    }
}
