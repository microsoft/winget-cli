// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"

#include <AppInstallerLogging.h>
#include <AppInstallerVersions.h>
#include <AppInstallerStrings.h>

#include "ConfigurationSetSerializer.h"
#include "ConfigurationSetSerializer_0_2.h"
#include "ConfigurationSetSerializer_0_3.h"
#include "ConfigurationSetUtilities.h"

using namespace AppInstaller::Utility;
using namespace AppInstaller::YAML;
using namespace winrt::Windows::Foundation;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace anon
    {
        static constexpr std::string_view s_nullValue = "null";
    }

    std::unique_ptr<ConfigurationSetSerializer> ConfigurationSetSerializer::CreateSerializer(hstring version, bool strictVersionMatching)
    {
        // Create the parser based on the version selected
        SemanticVersion schemaVersion(std::move(winrt::to_string(version)));

        // TODO: Consider having the version/uri/type information all together in the future
        if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 1)
        {
            // Remove this once the 0.1 serializer is implemented.
            THROW_HR_IF(E_NOTIMPL, strictVersionMatching);

            return std::make_unique<ConfigurationSetSerializer_0_2>();

        }
        else if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 2)
        {
            return std::make_unique<ConfigurationSetSerializer_0_2>();
        }
        else if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 3)
        {
            return std::make_unique<ConfigurationSetSerializer_0_3>();
        }
        else
        {
            AICLI_LOG(Config, Error, << "Unknown configuration version: " << schemaVersion.ToString());
            THROW_HR(E_UNEXPECTED);
        }
    }

    std::string ConfigurationSetSerializer::SerializeValueSet(const Windows::Foundation::Collections::ValueSet& valueSet)
    {
        Emitter emitter;
        WriteYamlValueSet(emitter, valueSet);
        return emitter.str();
    }

    std::string ConfigurationSetSerializer::SerializeStringArray(const Windows::Foundation::Collections::IVector<hstring>& stringArray)
    {
        Emitter emitter;
        WriteYamlStringArray(emitter, stringArray);
        return emitter.str();
    }

    void ConfigurationSetSerializer::WriteYamlValueSetIfNotEmpty(AppInstaller::YAML::Emitter& emitter, ConfigurationField key, const Windows::Foundation::Collections::ValueSet& valueSet)
    {
        if (valueSet && valueSet.Size() != 0)
        {
            emitter << Key << GetConfigurationFieldName(key);
            WriteYamlValueSet(emitter, valueSet);
        }
    }

    void ConfigurationSetSerializer::WriteYamlValueSet(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSet, const std::vector<ValueSetOverride>& overrides)
    {
        // Create a sorted list of the field names to exclude
        std::vector<winrt::hstring> exclusionStrings;
        for (const ValueSetOverride& override : overrides)
        {
            exclusionStrings.emplace_back(GetConfigurationFieldNameHString(override.Field));
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

        for (const ValueSetOverride & override : overrides)
        {
            if (override.Value != nullptr)
            {
                std::string_view keyName = GetConfigurationFieldName(override.Field);
                emitter << Key << keyName << Value;
                WriteYamlValue(emitter, override.Value);
            }
        }

        emitter << EndMap;
    }

    void ConfigurationSetSerializer::WriteYamlStringArray(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::IVector<hstring>& values)
    {
        emitter << BeginSeq;

        for (const auto& value : values)
        {
            emitter << ConvertToUTF8(value);
        }

        emitter << EndSeq;
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
                    emitter << ScalarStyle::DoubleQuoted << ConvertToUTF8(property.GetString());
                }
                else if (type == PropertyType::Int64)
                {
                    emitter << property.GetInt64();
                }
                else
                {
                    THROW_HR(E_NOTIMPL);
                }
            }
        }
    }

    void ConfigurationSetSerializer::WriteYamlValueIfNotEmpty(AppInstaller::YAML::Emitter& emitter, ConfigurationField key, const winrt::Windows::Foundation::IInspectable& value)
    {
        if (value != nullptr)
        {
            emitter << Key << GetConfigurationFieldName(key) << Value;
            WriteYamlValue(emitter, value);
        }
    }

    void ConfigurationSetSerializer::WriteYamlStringValueIfNotEmpty(AppInstaller::YAML::Emitter& emitter, ConfigurationField key, hstring value)
    {
        if (!value.empty())
        {
            emitter << Key << GetConfigurationFieldName(key) << Value << ConvertToUTF8(value);
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

    std::wstring_view ConfigurationSetSerializer::GetSchemaVersionCommentPrefix()
    {
        return L"# yaml-language-server: $schema=https://aka.ms/configuration-dsc-schema/"sv;
    }
}
