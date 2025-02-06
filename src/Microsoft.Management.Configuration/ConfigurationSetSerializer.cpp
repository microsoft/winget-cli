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

        struct ValueSetWriter
        {
            ValueSetWriter(const Windows::Foundation::Collections::ValueSet& valueSet, const std::vector<std::pair<ConfigurationField, Windows::Foundation::IInspectable>>& overrides) :
                m_valueSet(valueSet), m_overrides(overrides)
            {
                // Create a sorted list of the field names to exclude
                for (const auto & override : m_overrides)
                {
                    m_exclusionStrings.push_back(GetConfigurationFieldNameHString(override.first));
                }
                std::sort(m_exclusionStrings.begin(), m_exclusionStrings.end());
            }

            bool IsResultEmpty()
            {
                size_t nullOverrides = 0;

                for (const auto& override : m_overrides)
                {
                    // A non-null override will always be output
                    if (override.second)
                    {
                        return false;
                    }
                    else
                    {
                        ++nullOverrides;
                    }
                }

                if (m_valueSet)
                {
                    // If there are more values than null overrides, something will be output
                    if (static_cast<size_t>(m_valueSet.Size()) > nullOverrides)
                    {
                        return false;
                    }

                    // Check for a value that we would output
                    for (const auto& [key, value] : m_valueSet)
                    {
                        if (value != nullptr &&
                            !std::binary_search(m_exclusionStrings.begin(), m_exclusionStrings.end(), key))
                        {
                            return false;
                        }
                    }
                }

                return true;
            }

            void Write(AppInstaller::YAML::Emitter& emitter, void(* WriteYamlValue)(AppInstaller::YAML::Emitter& emitter, const winrt::Windows::Foundation::IInspectable& value))
            {
                emitter << BeginMap;

                WriteValues(emitter, WriteYamlValue);

                emitter << EndMap;
            }

            void WriteValues(AppInstaller::YAML::Emitter& emitter, void(*WriteYamlValue)(AppInstaller::YAML::Emitter& emitter, const winrt::Windows::Foundation::IInspectable& value))
            {
                if (m_valueSet)
                {
                    for (const auto& [key, value] : m_valueSet)
                    {
                        if (value != nullptr &&
                            !std::binary_search(m_exclusionStrings.begin(), m_exclusionStrings.end(), key))
                        {
                            std::string keyName = winrt::to_string(key);
                            emitter << Key << keyName << Value;
                            WriteYamlValue(emitter, value);
                        }
                    }
                }

                for (const auto & override : m_overrides)
                {
                    if (override.second != nullptr)
                    {
                        std::string_view keyName = GetConfigurationFieldName(override.first);
                        emitter << Key << keyName << Value;
                        WriteYamlValue(emitter, override.second);
                    }
                }
            }

        private:
            const Windows::Foundation::Collections::ValueSet& m_valueSet;
            const std::vector<std::pair<ConfigurationField, Windows::Foundation::IInspectable>>& m_overrides;
            std::vector<winrt::hstring> m_exclusionStrings;
        };
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

    void ConfigurationSetSerializer::WriteYamlValueSetIfNotEmpty(AppInstaller::YAML::Emitter& emitter, ConfigurationField key, const Windows::Foundation::Collections::ValueSet& valueSet, const std::vector<std::pair<ConfigurationField, Windows::Foundation::IInspectable>>& overrides)
    {
        anon::ValueSetWriter writer{ valueSet, overrides };

        if (!writer.IsResultEmpty())
        {
            emitter << Key << GetConfigurationFieldName(key);
            writer.Write(emitter, WriteYamlValue);
        }
    }

    void ConfigurationSetSerializer::WriteYamlValueSet(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSet, const std::vector<std::pair<ConfigurationField, Windows::Foundation::IInspectable>>& overrides)
    {
        anon::ValueSetWriter writer{ valueSet, overrides };
        writer.Write(emitter, WriteYamlValue);
    }

    void ConfigurationSetSerializer::WriteYamlValueSetValues(AppInstaller::YAML::Emitter& emitter, const Windows::Foundation::Collections::ValueSet& valueSet, const std::vector<std::pair<ConfigurationField, Windows::Foundation::IInspectable>>& overrides)
    {
        anon::ValueSetWriter writer{ valueSet, overrides };
        writer.WriteValues(emitter, WriteYamlValue);
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
