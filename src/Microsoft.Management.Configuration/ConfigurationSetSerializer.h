// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSetSerializer.h"
#include "ConfigurationSetUtilities.h"
#include "ConfigurationSet.h"

#include <winget/Yaml.h>
#include <initializer_list>
#include <string_view>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ConfigurationSetSerializer
    {
        static std::unique_ptr<ConfigurationSetSerializer> CreateSerializer(hstring version, bool strictVersionMatching = false);

        virtual ~ConfigurationSetSerializer() noexcept = default;

        ConfigurationSetSerializer(const ConfigurationSetSerializer&) = delete;
        ConfigurationSetSerializer& operator=(const ConfigurationSetSerializer&) = delete;
        ConfigurationSetSerializer(ConfigurationSetSerializer&&) = default;
        ConfigurationSetSerializer& operator=(ConfigurationSetSerializer&&) = default;

        // Serializes a configuration set to the original yaml string.
        virtual hstring Serialize(ConfigurationSet*) = 0;

        // Serializes a value set only.
        std::string SerializeValueSet(const Windows::Foundation::Collections::ValueSet& valueSet);

        // Serializes a value set only.
        std::string SerializeStringArray(const Windows::Foundation::Collections::IVector<hstring>& stringArray);

    protected:
        ConfigurationSetSerializer() = default;

        void WriteYamlValueSet(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSet, std::initializer_list<ConfigurationField> exclusions = {});
        void WriteYamlValueSetIfNotEmpty(AppInstaller::YAML::Emitter& emitter, ConfigurationField key, const Windows::Foundation::Collections::ValueSet& valueSet);
        void WriteYamlValueSetAsArray(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSetArray);

        void WriteYamlStringArray(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::IVector<hstring>& values);

        void WriteYamlValue(AppInstaller::YAML::Emitter& emitter, const winrt::Windows::Foundation::IInspectable& value);
        void WriteYamlValueIfNotEmpty(AppInstaller::YAML::Emitter& emitter, ConfigurationField key, const winrt::Windows::Foundation::IInspectable& value);
        void WriteYamlStringValueIfNotEmpty(AppInstaller::YAML::Emitter& emitter, ConfigurationField key, hstring value);

        std::wstring_view GetSchemaVersionCommentPrefix();
    };
}
