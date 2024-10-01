// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetSerializer_0_3.h"
#include "ArgumentValidation.h"
#include "ConfigurationSetParser_0_3.h"
#include "ConfigurationSetUtilities.h"
#include <AppInstallerStrings.h>

using namespace AppInstaller::Utility;
using namespace AppInstaller::YAML;
using namespace winrt::Windows::Foundation;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    hstring ConfigurationSetSerializer_0_3::Serialize(ConfigurationSet* configurationSet)
    {
        Emitter emitter;

        emitter << BeginMap;

        emitter << Key << GetConfigurationFieldName(ConfigurationField::Schema) << Value << ConvertToUTF8(configurationSet->SchemaUri().ToString());

        WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Metadata, configurationSet->Metadata());
        WriteYamlParameters(emitter, configurationSet->Parameters());
        WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Variables, configurationSet->Variables());
        WriteYamlConfigurationUnits(emitter, configurationSet->Units());

        emitter << EndMap;

        std::wostringstream result;
        result << GetSchemaVersionCommentPrefix() << static_cast<std::wstring_view>(configurationSet->SchemaVersion()) << L"\n" << ConvertToUTF16(emitter.str());
        return hstring{ std::move(result).str() };
    }

    void ConfigurationSetSerializer_0_3::WriteYamlParameters(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::IVector<Configuration::ConfigurationParameter>& values)
    {
        if (!values || values.Size() == 0)
        {
            return;
        }

        emitter << Key << GetConfigurationFieldName(ConfigurationField::Parameters);

        emitter << BeginMap;

        for (const Configuration::ConfigurationParameter& parameter : values)
        {
            emitter << Key << ConvertToUTF8(parameter.Name());

            emitter << BeginMap;

            auto type = parameter.Type();

            emitter << Key << GetConfigurationFieldName(ConfigurationField::Type) << Value << ToString(type, parameter.IsSecure());
            WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Metadata, parameter.Metadata());
            WriteYamlStringValueIfNotEmpty(emitter, ConfigurationField::Description, parameter.Description());
            WriteYamlValueIfNotEmpty(emitter, ConfigurationField::DefaultValue, parameter.DefaultValue());

            auto allowedValues = parameter.AllowedValues();
            if (allowedValues && allowedValues.Size() != 0)
            {
                emitter << Key << GetConfigurationFieldName(ConfigurationField::AllowedValues);

                emitter << BeginSeq;

                for (const auto& value : allowedValues)
                {
                    emitter << Value;
                    WriteYamlValue(emitter, value);
                }

                emitter << EndSeq;
            }

            if (IsLengthType(type))
            {
                uint32_t minimumLength = parameter.MinimumLength();
                if (minimumLength != 0)
                {
                    emitter << Key << GetConfigurationFieldName(ConfigurationField::MinimumLength) << Value << static_cast<int64_t>(minimumLength);
                }

                uint32_t maximumLength = parameter.MaximumLength();
                if (maximumLength != std::numeric_limits<uint32_t>::max())
                {
                    emitter << Key << GetConfigurationFieldName(ConfigurationField::MaximumLength) << Value << static_cast<int64_t>(maximumLength);
                }
            }

            if (IsComparableType(type))
            {
                WriteYamlValueIfNotEmpty(emitter, ConfigurationField::MinimumValue, parameter.MinimumValue());
                WriteYamlValueIfNotEmpty(emitter, ConfigurationField::MaximumValue, parameter.MaximumValue());
            }

            emitter << EndMap;
        }

        emitter << EndMap;
    }

    void ConfigurationSetSerializer_0_3::WriteYamlConfigurationUnits(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>& values)
    {
        emitter << Key << GetConfigurationFieldName(ConfigurationField::Resources);

        emitter << BeginSeq;

        for (const Configuration::ConfigurationUnit& unit : values)
        {
            emitter << BeginMap;

            emitter << Key << GetConfigurationFieldName(ConfigurationField::Name) << Value << ConvertToUTF8(unit.Identifier());
            emitter << Key << GetConfigurationFieldName(ConfigurationField::Type) << Value << ConvertToUTF8(unit.Type());
            WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Metadata, unit.Metadata());

            auto dependencies = unit.Dependencies();
            if (dependencies && dependencies.Size() != 0)
            {
                emitter << Key << GetConfigurationFieldName(ConfigurationField::DependsOn);

                emitter << BeginSeq;

                for (const auto& value : dependencies)
                {
                    emitter << ConvertToUTF8(value);
                }

                emitter << EndSeq;
            }

            WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Properties, unit.Settings());

            emitter << EndMap;
        }

        emitter << EndSeq;
    }
}
