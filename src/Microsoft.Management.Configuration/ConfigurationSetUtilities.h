// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string_view>
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // The various configuration fields that are used in parsing/serialization.
    enum class ConfigurationField
    {
        // v0.1 and v0.2
        ConfigurationVersion,
        Properties,
        Resource,
        Directives,
        Settings,
        Assertions,
        Id,
        DependsOn,

        // Universal
        Resources,
        ModuleDirective,
        SecurityContextMetadata,

        // v0.3
        Schema,
        Metadata,
        Parameters,
        Variables,
        Type,
        Description,
        Name,
        IsGroupMetadata,
        DefaultValue,
        AllowedValues,
        MinimumLength,
        MaximumLength,
        MinimumValue,
        MaximumValue,
        WingetMetadataRoot,
        ProcessorMetadata,
        ProcessorIdentifierMetadata,
        ProcessorPropertiesMetadata,
    };

    // Gets the name value of the configuration field.
    std::string_view GetConfigurationFieldName(ConfigurationField fieldName);

    winrt::hstring GetConfigurationFieldNameHString(ConfigurationField fieldName);

    // Attempts to parse a security context from a string.
    // Returns true if successful; false otherwise.
    bool TryParseSecurityContext(const hstring& value, SecurityContext& result);

    // Parses a security context from a string.
    SecurityContext ParseSecurityContext(const hstring& value);

    // Gets the string representation of a security context.
    std::string_view ToString(SecurityContext value);

    // Gets the string representation of a security context.
    std::wstring_view ToWString(SecurityContext value);

    // Tries to get the field value from the given value set; only if it is a value set.
    Windows::Foundation::Collections::ValueSet TryLookupValueSet(const Windows::Foundation::Collections::ValueSet& valueSet, ConfigurationField field);

    // Tries to get the field value from the given value set; only if it is a value set.
    Windows::Foundation::IPropertyValue TryLookupProperty(const Windows::Foundation::Collections::ValueSet& valueSet, ConfigurationField field, Windows::Foundation::PropertyType type = Windows::Foundation::PropertyType::Empty);
}
