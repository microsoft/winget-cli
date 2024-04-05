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
        std::vector<ConfigurationUnit> assertions;
        std::vector<ConfigurationUnit> resources;

        for (auto unit : configurationSet->Units())
        {
            if (unit.Intent() == ConfigurationUnitIntent::Assert)
            {
                assertions.emplace_back(unit);
            }
            else if (unit.Intent() == ConfigurationUnitIntent::Apply)
            {
                resources.emplace_back(unit);
            }
        }

        Emitter emitter;

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::Properties);

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::ConfigurationVersion) << Value << AppInstaller::Utility::ConvertToUTF8(configurationSet->SchemaVersion());

        if (!assertions.empty())
        {
            emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::Assertions);
            WriteYamlConfigurationUnits(emitter, assertions);
        }

        if (!resources.empty())
        {
            emitter << Key << GetConfigurationFieldName(ConfigurationFieldName::Resources);
            WriteYamlConfigurationUnits(emitter, resources);
        }

        emitter << EndMap;
        emitter << EndMap;

        return winrt::to_hstring(emitter.str());
    }
}
