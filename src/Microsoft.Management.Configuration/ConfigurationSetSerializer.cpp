// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#include <AppInstallerLogging.h>
#include <AppInstallerVersions.h>
#include <AppInstallerStrings.h>

#include "ConfigurationSetSerializer.h"
#include "ConfigurationSetSerializer_0_2.h"
#include "ConfigurationSetUtilities.h"

using namespace AppInstaller::YAML;
using namespace winrt::Windows::Foundation;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace anon
    {
        static constexpr std::string_view s_nullValue = "null";
    }

    std::unique_ptr<ConfigurationSetSerializer> ConfigurationSetSerializer::CreateSerializer(hstring version)
    {
        // Create the parser based on the version selected
        AppInstaller::Utility::SemanticVersion schemaVersion(std::move(winrt::to_string(version)));

        // TODO: Consider having the version/uri/type information all together in the future
        if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 1)
        {
            THROW_HR(E_NOTIMPL);
        }
        else if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 2)
        {
            return std::make_unique<ConfigurationSetSerializer_0_2>();
        }
        else if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 3)
        {
            THROW_HR(E_NOTIMPL);
        }
        else
        {
            AICLI_LOG(Config, Error, << "Unknown configuration version: " << schemaVersion.ToString());
            THROW_HR(E_UNEXPECTED);
        }
    }

    void ConfigurationSetSerializer::WriteYamlValueSet(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSet, std::initializer_list<ConfigurationField> exclusions)
    {
        // Create a sorted list of the field names to exclude
        std::vector<winrt::hstring> exclusionStrings;
        for (ConfigurationField field : exclusions)
        {
            exclusionStrings.emplace_back(GetConfigurationFieldNameHString(field));
        }
        std::sort(exclusionStrings.begin(), exclusionStrings.end());

        emitter << BeginMap;

        for (const auto& [key, value] : valueSet)
        {
            if (value != nullptr &&
                !std::binary_search(exclusionStrings.begin(), exclusionStrings.end(), key))
            {
                std::string keyName = winrt::to_string(key);
                emitter << Key << keyName << Value;
                WriteYamlValue(emitter, value);
            }
        }

        emitter << EndMap;
    }


    void ConfigurationSetSerializer::WriteYamlValue(AppInstaller::YAML::Emitter& emitter, const winrt::Windows::Foundation::IInspectable& value)
    {
        if (value == nullptr)
        {
            emitter << anon::s_nullValue;
        }
        else
        {
            const auto& currentValueSet = value.try_as<Windows::Foundation::Collections::ValueSet>();
            if (currentValueSet)
            {
                if (currentValueSet.HasKey(L"treatAsArray"))
                {
                    WriteYamlValueSetAsArray(emitter, currentValueSet);
                }
                else
                {
                    WriteYamlValueSet(emitter, currentValueSet);
                }
            }
            else
            {
                IPropertyValue property = value.as<IPropertyValue>();
                auto type = property.Type();

                if (type == PropertyType::Boolean)
                {
                    emitter << property.GetBoolean();
                }
                else if (type == PropertyType::String)
                {
                    emitter << AppInstaller::Utility::ConvertToUTF8(property.GetString());
                }
                else if (type == PropertyType::Int64)
                {
                    emitter << property.GetInt64();
                }
                else
                {
                    THROW_HR(E_NOTIMPL);;
                }
            }
        }
    }

    void ConfigurationSetSerializer::WriteYamlValueSetAsArray(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSetArray)
    {
        std::vector<std::pair<int, winrt::Windows::Foundation::IInspectable>> arrayValues;
        for (const auto& arrayValue : valueSetArray)
        {
            if (arrayValue.Key() != L"treatAsArray")
            {
                arrayValues.emplace_back(std::make_pair(std::stoi(arrayValue.Key().c_str()), arrayValue.Value()));
            }
        }

        std::sort(
            arrayValues.begin(),
            arrayValues.end(),
            [](const std::pair<int, winrt::Windows::Foundation::IInspectable>& a, const std::pair<int, winrt::Windows::Foundation::IInspectable>& b)
            {
                return a.first < b.first;
            });

        emitter << BeginSeq;

        for (const auto& arrayValue : arrayValues)
        {
            WriteYamlValue(emitter, arrayValue.second);
        }

        emitter << EndSeq;
    }

    void ConfigurationSetSerializer::WriteYamlConfigurationUnits(AppInstaller::YAML::Emitter& emitter, const std::vector<ConfigurationUnit>& units)
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

    winrt::hstring ConfigurationSetSerializer::GetResourceName(const ConfigurationUnit& unit)
    {
        return unit.Type();
    }

    void ConfigurationSetSerializer::WriteResourceDirectives(AppInstaller::YAML::Emitter& emitter, const ConfigurationUnit& unit)
    {
        const auto& metadata = unit.Metadata();
        emitter << Key << GetConfigurationFieldName(ConfigurationField::Directives);
        WriteYamlValueSet(emitter, metadata);
    }

    winrt::hstring ConfigurationSetSerializer::GetSchemaVersionComment(winrt::hstring version)
    {
        return winrt::to_hstring(L"# yaml-language-server: $schema=https://aka.ms/configuration-dsc-schema/") + version;
    }
}
