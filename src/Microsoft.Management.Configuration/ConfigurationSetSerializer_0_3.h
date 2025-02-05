// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetSerializer.h"
#include "ConfigurationEnvironment.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Serializer for schema version 0.3
    struct ConfigurationSetSerializer_0_3 : public ConfigurationSetSerializer
    {
        ConfigurationSetSerializer_0_3() {}

        virtual ~ConfigurationSetSerializer_0_3() noexcept = default;

        ConfigurationSetSerializer_0_3(const ConfigurationSetSerializer_0_3&) = delete;
        ConfigurationSetSerializer_0_3& operator=(const ConfigurationSetSerializer_0_3&) = delete;
        ConfigurationSetSerializer_0_3(ConfigurationSetSerializer_0_3&&) = default;
        ConfigurationSetSerializer_0_3& operator=(ConfigurationSetSerializer_0_3&&) = default;

        hstring Serialize(ConfigurationSet* configurationSet) override;

    protected:
        void WriteYamlParameters(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::IVector<Configuration::ConfigurationParameter>& values);
        void WriteYamlConfigurationUnits(
            AppInstaller::YAML::Emitter& emitter,
            const Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>& values);
    };
}
