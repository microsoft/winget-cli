// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetSerializer.h"
#include "ConfigurationSet.h"

#include <winget/Yaml.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationSetSerializer
    {
        static std::unique_ptr<ConfigurationSetSerializer> CreateSerializer(hstring version);

        virtual ~ConfigurationSetSerializer() noexcept = default;

        ConfigurationSetSerializer(const ConfigurationSetSerializer&) = delete;
        ConfigurationSetSerializer& operator=(const ConfigurationSetSerializer&) = delete;
        ConfigurationSetSerializer(ConfigurationSetSerializer&&) = default;
        ConfigurationSetSerializer& operator=(ConfigurationSetSerializer&&) = default;

        // Serializes a configuration set to the original yaml string.
        virtual hstring Serialize(ConfigurationSet*) = 0;

    protected:
        ConfigurationSetSerializer() = default;

        void WriteYamlConfigurationUnits(AppInstaller::YAML::Emitter& emitter, const std::vector<ConfigurationUnit>& units);

        void WriteYamlValueSet(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSet);
        void WriteYamlValue(AppInstaller::YAML::Emitter& emitter, const winrt::Windows::Foundation::IInspectable& value);
        void WriteYamlValueSetAsArray(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSetArray);
    };
}
