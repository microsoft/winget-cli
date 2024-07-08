// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetSerializer_0_2.h"
#include "ConfigurationSetUtilities.h"

#include <AppInstallerStrings.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    using namespace AppInstaller::YAML;
    using namespace winrt::Windows::Foundation;

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
        emitter << Key << GetConfigurationFieldName(ConfigurationField::Properties);

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(ConfigurationField::ConfigurationVersion) << Value << AppInstaller::Utility::ConvertToUTF8(configurationSet->SchemaVersion());

        if (!assertions.empty())
        {
            emitter << Key << GetConfigurationFieldName(ConfigurationField::Assertions);
            WriteYamlConfigurationUnits(emitter, assertions);
        }

        if (!resources.empty())
        {
            emitter << Key << GetConfigurationFieldName(ConfigurationField::Resources);
            WriteYamlConfigurationUnits(emitter, resources);
        }

        emitter << EndMap;
        emitter << EndMap;

        return GetSchemaVersionComment(configurationSet->SchemaVersion()) + winrt::to_hstring(L"\n") + winrt::to_hstring(emitter.str());
    }

    winrt::hstring ConfigurationSetSerializer_0_2::GetResourceName(const ConfigurationUnit& unit)
    {
        const auto& metadata = unit.Metadata();
        const auto moduleKey = GetConfigurationFieldNameHString(ConfigurationField::ModuleDirective);
        if (metadata.HasKey(moduleKey))
        {
            auto object = metadata.Lookup(moduleKey);
            auto property = object.try_as<IPropertyValue>();
            if (property && property.Type() == PropertyType::String)
            {
                return property.GetString() + '/' + unit.Type();
            }
        }

        return unit.Type();
    }

    void ConfigurationSetSerializer_0_2::WriteResourceDirectives(AppInstaller::YAML::Emitter& emitter, const ConfigurationUnit& unit)
    {
        emitter << Key << GetConfigurationFieldName(ConfigurationField::Directives);
        WriteYamlValueSet(emitter, unit.Metadata(), { ConfigurationField::ModuleDirective });
    }
}
