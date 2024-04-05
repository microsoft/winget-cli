// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetSerializer_0_2.h"
#include "ConfigurationSetUtilities.h"

#include <AppInstallerStrings.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    using namespace AppInstaller::YAML;

    hstring ConfigurationSetSerializer_0_2::Serialize(ConfigurationSet* configurationSet)
    {
        Emitter emitter;

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::Properties);

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::ConfigurationVersion) << Value << AppInstaller::Utility::ConvertToUTF8(configurationSet->SchemaVersion());

        emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::Resources);
        emitter << BeginSeq;

        for (auto unit : configurationSet->Units())
        {
            emitter << BeginMap;
            emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::Resource) << Value << AppInstaller::Utility::ConvertToUTF8(unit.Type());

            // Directives
            const auto& metadata = unit.Metadata();
            emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::Directives);
            WriteYamlValueSet(emitter, metadata);

            // Settings
            auto settings = unit.Settings();
            emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::Settings);
            WriteYamlValueSet(emitter, settings);

            emitter << EndMap;
        }

        emitter << EndSeq;

        emitter << EndMap;
        emitter << EndMap;

        return winrt::to_hstring(emitter.str());
    }
}
