// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <ConfigurationSetParser.h>

#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>
#include <AppInstallerVersions.h>

#include "ConfigurationSetParserError.h"
#include "ConfigurationSetParser_0_1.h"
#include "ConfigurationSetParser_0_2.h"

using namespace AppInstaller::YAML;

namespace winrt::Microsoft::Management::Configuration::implementation
{
    std::unique_ptr<ConfigurationSetParser> ConfigurationSetParser::Create(std::string_view input)
    {
        AICLI_LOG_LARGE_STRING(Config, Verbose, << "Parsing configuration set:", input);

        Node document;
        std::string documentError;
        Mark documentErrorMark;

        try
        {
            document = Load(input);
        }
        catch (const Exception& exc)
        {
            documentError = exc.what();
            documentErrorMark = exc.GetMark();
        }
        CATCH_LOG();

        if (!document.IsMap())
        {
            AICLI_LOG(Config, Error, << "Invalid YAML: " << documentError << " at [line " << documentErrorMark.line << ", col " << documentErrorMark.column << "]");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_YAML, documentError, documentErrorMark);
        }

        Node& propertiesNode = document[GetFieldName(FieldName::Properties)];
        if (!propertiesNode)
        {
            AICLI_LOG(Config, Error, << "No properties");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_MISSING_FIELD, GetFieldName(FieldName::Properties));
        }
        else if (!propertiesNode.IsMap())
        {
            AICLI_LOG(Config, Error, << "Invalid properties type");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, GetFieldName(FieldName::Properties), propertiesNode.Mark());
        }

        Node& versionNode = propertiesNode[GetFieldName(FieldName::ConfigurationVersion)];
        if (!versionNode)
        {
            AICLI_LOG(Config, Error, << "No configuration version");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_MISSING_FIELD, GetFieldName(FieldName::ConfigurationVersion));
        }
        else if (!versionNode.IsScalar())
        {
            AICLI_LOG(Config, Error, << "Invalid configuration version type");
            return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, GetFieldName(FieldName::ConfigurationVersion), versionNode.Mark());
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

        AICLI_LOG(Config, Error, << "Unknown configuration version: " << schemaVersion.ToString());
        return std::make_unique<ConfigurationSetParserError>(WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, GetFieldName(FieldName::ConfigurationVersion), versionNode.as<std::string>());
    }

    bool ConfigurationSetParser::IsRecognizedSchemaVersion(hstring value) try
    {
        using namespace AppInstaller::Utility;

        SemanticVersion schemaVersion(ConvertToUTF8(value));

        return (schemaVersion == SemanticVersion{ "0.1" } || schemaVersion == SemanticVersion{ "0.2" });
    }
    catch (...) { LOG_CAUGHT_EXCEPTION(); return false; }

    hstring ConfigurationSetParser::LatestVersion()
    {
        return hstring{ L"0.2" };
    }

    void ConfigurationSetParser::SetError(hresult result, std::string_view field, std::string_view value, uint32_t line, uint32_t column)
    {
        AICLI_LOG(Config, Error, << "ConfigurationSetParser error: " << AppInstaller::Logging::SetHRFormat << result << " for " << field << " with value `" << value << "` at [line " << line << ", col " << column << "]");
        m_result = result;
        m_field = AppInstaller::Utility::ConvertToUTF16(field);
        m_value = AppInstaller::Utility::ConvertToUTF16(value);
        m_line = line;
        m_column = column;
    }

    void ConfigurationSetParser::SetError(hresult result, std::string_view field, const Mark& mark, std::string_view value)
    {
        SetError(result, field, value, static_cast<uint32_t>(mark.line), static_cast<uint32_t>(mark.column));
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
