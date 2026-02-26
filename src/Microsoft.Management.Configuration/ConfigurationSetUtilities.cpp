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
        case ConfigurationField::SecurityContextMetadata: return "securityContext"sv;

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
        case ConfigurationField::WingetMetadataRoot: return "winget"sv;
        case ConfigurationField::ProcessorMetadata: return "processor"sv;
        case ConfigurationField::ProcessorIdentifierMetadata: return "identifier"sv;
        case ConfigurationField::ProcessorPropertiesMetadata: return "properties"sv;
        }

        THROW_HR(E_UNEXPECTED);
    }

    hstring GetConfigurationFieldNameHString(ConfigurationField fieldName)
    {
        return hstring{ AppInstaller::Utility::ConvertToUTF16(GetConfigurationFieldName(fieldName)) };
    }

    bool TryParseSecurityContext(const hstring& value, SecurityContext& result)
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

    SecurityContext ParseSecurityContext(const hstring& value)
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

    Windows::Foundation::Collections::ValueSet TryLookupValueSet(const Windows::Foundation::Collections::ValueSet& valueSet, ConfigurationField field)
    {
        Windows::Foundation::IInspectable value = valueSet.TryLookup(GetConfigurationFieldNameHString(field));

        if (value)
        {
            return value.try_as<Windows::Foundation::Collections::ValueSet>();
        }

        return nullptr;
    }

    Windows::Foundation::IPropertyValue TryLookupProperty(const Windows::Foundation::Collections::ValueSet& valueSet, ConfigurationField field, Windows::Foundation::PropertyType type)
    {
        Windows::Foundation::IInspectable value = valueSet.TryLookup(GetConfigurationFieldNameHString(field));

        if (value)
        {
            Windows::Foundation::IPropertyValue property = value.try_as<Windows::Foundation::IPropertyValue>();
            if (property && (type == Windows::Foundation::PropertyType::Empty || property.Type() == type))
            {
                return property;
            }
        }

        return nullptr;
    }
}
