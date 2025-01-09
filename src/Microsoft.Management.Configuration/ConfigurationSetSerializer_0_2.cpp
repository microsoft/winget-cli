// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetSerializer_0_2.h"
#include "ConfigurationSetUtilities.h"

#include <AppInstallerStrings.h>

using namespace AppInstaller::Utility;
using namespace AppInstaller::YAML;
using namespace winrt::Windows::Foundation;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    hstring ConfigurationSetSerializer_0_2::Serialize(ConfigurationSet* configurationSet)
    {
        std::vector<ConfigurationUnit> assertions;
        std::vector<ConfigurationUnit> resources;

        for (auto unit : configurationSet->Units())
        {
            ConfigurationUnitIntent unitIntent = unit.Intent();

            if (unitIntent == ConfigurationUnitIntent::Assert)
            {
                assertions.emplace_back(unit);
            }
            else if (unitIntent == ConfigurationUnitIntent::Apply)
            {
                resources.emplace_back(unit);
            }
        }

        Emitter emitter;

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(ConfigurationField::Properties);

        emitter << BeginMap;
        emitter << Key << GetConfigurationFieldName(ConfigurationField::ConfigurationVersion) << Value << ConvertToUTF8(configurationSet->SchemaVersion());

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

        std::wostringstream result;
        result << GetSchemaVersionCommentPrefix() << static_cast<std::wstring_view>(configurationSet->SchemaVersion()) << L"\n" << ConvertToUTF16(emitter.str());
        return hstring{ std::move(result).str() };
    }

    void ConfigurationSetSerializer_0_2::WriteYamlConfigurationUnits(AppInstaller::YAML::Emitter& emitter, const std::vector<ConfigurationUnit>& units)
    {
        emitter << BeginSeq;

        for (const auto& unit : units)
        {
            // Resource
            emitter << BeginMap;
            emitter << Key << GetConfigurationFieldName(ConfigurationField::Resource) << Value << AppInstaller::Utility::ConvertToUTF8(GetResourceName(unit));

            // Id
            if (!unit.Identifier().empty())
            {
                emitter << Key << GetConfigurationFieldName(ConfigurationField::Id) << Value << AppInstaller::Utility::ConvertToUTF8(unit.Identifier());
            }

            // Dependencies
            if (unit.Dependencies().Size() > 0)
            {
                emitter << Key << GetConfigurationFieldName(ConfigurationField::DependsOn);
                emitter << BeginSeq;

                for (const auto& dependency : unit.Dependencies())
                {
                    emitter << AppInstaller::Utility::ConvertToUTF8(dependency);
                }

                emitter << EndSeq;
            }

            // Directives
            WriteResourceDirectives(emitter, unit);

            // Settings
            const auto& settings = unit.Settings();
            emitter << Key << GetConfigurationFieldName(ConfigurationField::Settings);
            WriteYamlValueSet(emitter, settings);

            emitter << EndMap;
        }

        emitter << EndSeq;
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
        SecurityContext securityContext = unit.Environment().Context();

        WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Directives, unit.Metadata(),
            { { ConfigurationField::ModuleDirective, nullptr },
            { ConfigurationField::SecurityContextMetadata, (securityContext != SecurityContext::Current ? PropertyValue::CreateString(ToWString(securityContext)) : nullptr)} });
    }
}
