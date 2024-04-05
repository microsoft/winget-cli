// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <string_view>
#include <winrt/base.h>

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
    };

    // Gets the name value of the configuration field.
    std::string_view GetConfigurationFieldName(ConfigurationField fieldName);

    winrt::hstring GetConfigurationFieldNameHString(ConfigurationField fieldName);
}
