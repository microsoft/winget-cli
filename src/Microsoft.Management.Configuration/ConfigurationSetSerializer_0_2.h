// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetSerializer.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Serializer for schema version 0.2
    struct ConfigurationSetSerializer_0_2 : public ConfigurationSetSerializer
    {
        ConfigurationSetSerializer_0_2() {}

        virtual ~ConfigurationSetSerializer_0_2() noexcept = default;

        ConfigurationSetSerializer_0_2(const ConfigurationSetSerializer_0_2&) = delete;
        ConfigurationSetSerializer_0_2& operator=(const ConfigurationSetSerializer_0_2&) = delete;
        ConfigurationSetSerializer_0_2(ConfigurationSetSerializer_0_2&&) = default;
        ConfigurationSetSerializer_0_2& operator=(ConfigurationSetSerializer_0_2&&) = default;

        hstring Serialize(ConfigurationSet* configurationSet) override;

    protected:
        void WriteYamlConfigurationUnits(AppInstaller::YAML::Emitter& emitter, const std::vector<ConfigurationUnit>& units);

        virtual winrt::hstring GetResourceName(const ConfigurationUnit& unit);
        virtual void WriteResourceDirectives(AppInstaller::YAML::Emitter& emitter, const ConfigurationUnit& unit);
    };
}
