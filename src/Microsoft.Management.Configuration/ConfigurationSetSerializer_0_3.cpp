// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetSerializer_0_3.h"
#include "ArgumentValidation.h"
#include "ConfigurationSetParser_0_3.h"
#include "ConfigurationSetUtilities.h"
#include "ConfigurationEnvironment.h"
#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>

using namespace AppInstaller::Utility;
using namespace AppInstaller::YAML;
using namespace winrt::Windows::Foundation;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        Windows::Foundation::Collections::ValueSet GetWingetProcessorMetadataValueSet(Windows::Foundation::Collections::ValueSet& metadata)
        {
            Windows::Foundation::Collections::ValueSet result = nullptr;
            hstring processorMetadataKey = GetConfigurationFieldNameHString(ConfigurationField::ProcessorMetadata);

            if (metadata)
            {
                Windows::Foundation::IInspectable processorMetadataObject = metadata.TryLookup(processorMetadataKey);
                if (processorMetadataObject)
                {
                    result = processorMetadataObject.try_as<Windows::Foundation::Collections::ValueSet>();
                    THROW_HR_IF(WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, !result);
                }
            }
            else
            {
                metadata = Collections::ValueSet{};
            }

            if (!result)
            {
                result = Collections::ValueSet{};
                metadata.Insert(processorMetadataKey, result);
            }

            return result;
        }

        Windows::Foundation::Collections::ValueSet CreateValueSetFromStringMap(const Windows::Foundation::Collections::IMap<hstring, hstring>& map)
        {
            Windows::Foundation::Collections::ValueSet result;
            if (map)
            {
                for (const auto& item : map)
                {
                    result.Insert(item.Key(), PropertyValue::CreateString(item.Value()));
                }
            }
            return result;
        }

        void AddEnvironmentToMetadata(
            Windows::Foundation::Collections::ValueSet& metadata,
            SecurityContext context,
            hstring processor,
            Windows::Foundation::Collections::IMap<hstring, hstring> properties,
            SecurityContext defaultContext = SecurityContext::Current,
            hstring defaultProcessor = {},
            Windows::Foundation::Collections::IMap<hstring, hstring> defaultProperties = nullptr)
        {
            if (context != defaultContext)
            {
                if (!metadata)
                {
                    metadata = Collections::ValueSet{};
                }

                metadata.Insert(GetConfigurationFieldNameHString(ConfigurationField::SecurityContextMetadata), PropertyValue::CreateString(ToWString(context)));
            }

            Windows::Foundation::Collections::ValueSet processorValueSet{ nullptr };

            if (processor != defaultProcessor)
            {
                if (!processorValueSet)
                {
                    processorValueSet = GetWingetProcessorMetadataValueSet(metadata);
                }

                processorValueSet.Insert(GetConfigurationFieldNameHString(ConfigurationField::ProcessorIdentifierMetadata), PropertyValue::CreateString(processor));
            }

            if (!ConfigurationEnvironment::AreEqual(properties, defaultProperties))
            {
                if (!processorValueSet)
                {
                    processorValueSet = GetWingetProcessorMetadataValueSet(metadata);
                }

                processorValueSet.Insert(GetConfigurationFieldNameHString(ConfigurationField::ProcessorPropertiesMetadata), CreateValueSetFromStringMap(properties));
            }
        }

        void AddEnvironmentToMetadata(
            Windows::Foundation::Collections::ValueSet& metadata,
            const com_ptr<implementation::ConfigurationEnvironment>& environment)
        {
            if (environment)
            {
                AddEnvironmentToMetadata(metadata, environment->Context(), environment->ProcessorIdentifier(), environment->ProcessorProperties());
            }
        }

        void AddEnvironmentToMetadata(
            Windows::Foundation::Collections::ValueSet& metadata,
            const Configuration::ConfigurationEnvironment& environment)
        {
            AddEnvironmentToMetadata(metadata,
                environment.Context(), environment.ProcessorIdentifier(), environment.ProcessorProperties());
        }
    }

    hstring ConfigurationSetSerializer_0_3::Serialize(ConfigurationSet* configurationSet)
    {
        Emitter emitter;

        emitter << BeginMap;

        emitter << Key << GetConfigurationFieldName(ConfigurationField::Schema) << Value << ConvertToUTF8(configurationSet->SchemaUri().ToString());

        // Prepare an override if necessary
        Collections::ValueSet wingetMetadataOverride = nullptr;
        AddEnvironmentToMetadata(wingetMetadataOverride, configurationSet->EnvironmentInternal());

        WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Metadata, configurationSet->Metadata(),
            {
                { ConfigurationField::WingetMetadataRoot, wingetMetadataOverride },
                { ConfigurationField::SecurityContextMetadata, nullptr },
            });

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

    void ConfigurationSetSerializer_0_3::WriteYamlConfigurationUnits(
        AppInstaller::YAML::Emitter& emitter,
        const Windows::Foundation::Collections::IVector<Configuration::ConfigurationUnit>& values)
    {
        emitter << Key << GetConfigurationFieldName(ConfigurationField::Resources);

        emitter << BeginSeq;

        for (const Configuration::ConfigurationUnit& unit : values)
        {
            emitter << BeginMap;

            hstring identifier = unit.Identifier();
            THROW_HR_IF(WINGET_CONFIG_ERROR_MISSING_FIELD, identifier.empty());
            emitter << Key << GetConfigurationFieldName(ConfigurationField::Name) << Value << ConvertToUTF8(identifier);

            hstring type = unit.Type();
            THROW_HR_IF(WINGET_CONFIG_ERROR_MISSING_FIELD, type.empty());
            emitter << Key << GetConfigurationFieldName(ConfigurationField::Type) << Value << ConvertToUTF8(type);

            // Prepare an override if necessary
            Collections::ValueSet wingetMetadataOverride = nullptr;
            Configuration::ConfigurationEnvironment unitEnvironment = unit.Environment();
            AddEnvironmentToMetadata(wingetMetadataOverride, unitEnvironment);

            WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Metadata, unit.Metadata(),
                { { ConfigurationField::WingetMetadataRoot, wingetMetadataOverride } });

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

            // If this unit is a group, write the units directly
            if (unit.IsGroup())
            {
                auto groupUnits = unit.Units();

                if (groupUnits.Size() != 0)
                {
                    emitter << Key << GetConfigurationFieldName(ConfigurationField::Properties);
                    emitter << BeginMap;

                    // Write everything but the resources
                    WriteYamlValueSetValues(emitter, unit.Settings(),
                        { { ConfigurationField::Resources, nullptr } });

                    // Write the resources from the individual units
                    WriteYamlConfigurationUnits(emitter, groupUnits);

                    emitter << EndMap;
                }
            }
            else
            {
                WriteYamlValueSetIfNotEmpty(emitter, ConfigurationField::Properties, unit.Settings());
            }

            emitter << EndMap;
        }

        emitter << EndSeq;
    }
}
