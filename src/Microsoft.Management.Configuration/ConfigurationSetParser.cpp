// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <ConfigurationSetParser.h>

#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>
#include <winget/Yaml.h>

#include "ConfigurationSetParserError.h"
#include "ConfigurationSetParser_0_1.h"
#include "ConfigurationSetParser_0_2.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        std::string StreamToString(const Windows::Storage::Streams::IInputStream& stream)
        {
            uint32_t bufferSize = 1 << 20;
            Windows::Storage::Streams::Buffer buffer(bufferSize);
            Windows::Storage::Streams::InputStreamOptions readOptions = Windows::Storage::Streams::InputStreamOptions::Partial | Windows::Storage::Streams::InputStreamOptions::ReadAhead;
            std::string result;

            for (;;)
            {
                Windows::Storage::Streams::IBuffer readBuffer = stream.ReadAsync(buffer, bufferSize, readOptions).get();

                size_t readSize = static_cast<size_t>(readBuffer.Length());
                if (readSize)
                {
                    static_assert(sizeof(char) == sizeof(*readBuffer.data()));
                    result.append(reinterpret_cast<char*>(readBuffer.data()), readSize);
                }
                else
                {
                    break;
                }
            }

            return result;
        }
    }

    std::unique_ptr<ConfigurationSetParser> ConfigurationSetParser::Create(const Windows::Storage::Streams::IInputStream& stream)
    {
        return Create(StreamToString(stream));
    }

    std::unique_ptr<ConfigurationSetParser> ConfigurationSetParser::Create(std::string_view input)
    {
        AICLI_LOG_LARGE_STRING(Config, Verbose, << "Parsing configuration set:", input);

        AppInstaller::YAML::Node document;
        
        try
        {
            document = AppInstaller::YAML::Load(input);
        }
        CATCH_LOG();

        if (!document.IsMap())
        {
            AICLI_LOG(Config, Info, << "Invalid YAML");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_YAML);
        }

        AppInstaller::YAML::Node& propertiesNode = document[GetFieldName(FieldName::Properties)];
        if (!propertiesNode.IsMap())
        {
            AICLI_LOG(Config, Info, << "Invalid properties");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, GetFieldName(FieldName::Properties));
        }

        AppInstaller::YAML::Node& versionNode = propertiesNode[GetFieldName(FieldName::ConfigurationVersion)];
        if (!versionNode.IsScalar())
        {
            AICLI_LOG(Config, Info, << "Invalid configuration version");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, GetFieldName(FieldName::ConfigurationVersion));
        }

        AppInstaller::Utility::SemanticVersion schemaVersion(versionNode.as<std::string>());

        if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 1)
        {
            return std::make_unique<ConfigurationSetParser_0_1>(std::move(document));
        }
        else if (schemaVersion.PartAt(0).Integer == 0 && schemaVersion.PartAt(1).Integer == 2)
        {
            return std::make_unique<ConfigurationSetParser_0_2>(std::move(document));
        }

        AICLI_LOG(Config, Info, << "Unknown configuration version: " << schemaVersion.ToString());
        return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, GetFieldName(FieldName::ConfigurationVersion), versionNode.as<std::string>());
    }

    bool ConfigurationSetParser::IsRecognizedSchemaVersion(hstring value) try
    {
        using namespace AppInstaller::Utility;

        SemanticVersion schemaVersion(ConvertToUTF8(value));

        return (schemaVersion == SemanticVersion{ "0.1" });
    }
    catch (...) { LOG_CAUGHT_EXCEPTION(); return false; }

    hstring ConfigurationSetParser::LatestVersion()
    {
        return hstring{ L"0.1" };
    }

    void ConfigurationSetParser::SetError(hresult result, std::string_view field, std::string_view value)
    {
        AICLI_LOG(Config, Error, << "ConfigurationSetParser error: " << AppInstaller::Logging::SetHRFormat << result << " for " << field << " with value `" << value << "`");
        m_result = result;
        m_field = AppInstaller::Utility::ConvertToUTF16(field);
        m_value = AppInstaller::Utility::ConvertToUTF16(value);
    }

    std::string_view ConfigurationSetParser::GetFieldName(FieldName fieldName)
    {
        switch (fieldName)
        {
        case FieldName::ConfigurationVersion: return "configurationVersion"sv;
        case FieldName::Properties: return "properties"sv;
        case FieldName::Resource: return "resource"sv;
        case FieldName::ModuleDirective: return "module"sv;
        }

        THROW_HR(E_UNEXPECTED);
    }

    hstring ConfigurationSetParser::GetFieldNameHString(FieldName fieldName)
    {
        return hstring{ AppInstaller::Utility::ConvertToUTF16(GetFieldName(fieldName)) };
    }
}
