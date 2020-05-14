// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>
#include <functional>
#include <wil/result.h>
#include <AppInstallerErrors.h>
#include <AppInstallerVersions.h>

namespace YAML { class Node; }

namespace AppInstaller::Manifest
{
    // ManifestVer is inherited from Utility::Version and is a more restricted version.
    // ManifestVer is used to specify the version of app manifest itself.
    // Currently ManifestVer is a 3 part version in the format of [0-65535].[0-65535].[0-65535]
    struct ManifestVer : public Utility::Version
    {
        ManifestVer() = default;

        ManifestVer(std::string version, bool fullValidation);

        uint64_t Major() { return m_parts.size() > 0 ? m_parts[0].Integer : 0; }
        uint64_t Minor() { return m_parts.size() > 1 ? m_parts[1].Integer : 0; }
        uint64_t Patch() { return m_parts.size() > 2 ? m_parts[2].Integer : 0; }
    };

    static const uint64_t MaxSupportedMajorVersion = 1;
    static const ManifestVer PreviewManifestVersion = ManifestVer("0.1.0", false);

    namespace ManifestError
    {
        const char* const ErrorMessagePrefix = "Manifest Error: ";
        const char* const WarningMessagePrefix = "Manifest Warning: ";

        const char* const InvalidRootNode = "Encountered unexpected root node.";
        const char* const FieldUnknown = "Unknown field.";
        const char* const FieldIsNotPascalCase = "All field names should be PascalCased.";
        const char* const FieldDuplicate = "Duplicate field found in the manifest.";
        const char* const RequiredFieldEmpty = "Required field with empty value.";
        const char* const RequiredFieldMissing = "Required field missing.";
        const char* const InvalidFieldValue = "Invalid field value.";
        const char* const ExeInstallerMissingSilentSwitches = "Silent and SilentWithProgress switches are not specified for InstallerType exe. Please make sure the installer can run unattended.";
        const char* const FieldNotSupported = "Field is not supported.";
        const char* const DuplicateInstallerEntry = "Duplicate installer entry found.";
    }

    struct ValidationError
    {
        enum class Level
        {
            Warning,
            Error
        };

        std::string Message;
        std::string Field = {};
        std::string Value = {};
        // line and column is 0 based
        int Line = -1;
        int Column = -1;
        Level ErrorLevel = Level::Error;

        ValidationError(std::string message) :
            Message(std::move(message)) {}

        ValidationError(std::string message, Level level) :
            Message(std::move(message)), ErrorLevel(level) {}

        ValidationError(std::string message, std::string field) :
            Message(std::move(message)), Field(std::move(field)) {}

        ValidationError(std::string message, std::string field, std::string value) :
            Message(std::move(message)), Field(std::move(field)), Value(std::move(value)) {}

        ValidationError(std::string message, std::string field, std::string value, int line, int column) :
            Message(std::move(message)), Field(std::move(field)), Value(std::move(value)), Line(line), Column(column) {}

        ValidationError(std::string message, std::string field, std::string value, int line, int column, Level level) :
            Message(std::move(message)), Field(std::move(field)), Value(std::move(value)), Line(line), Column(column), ErrorLevel(level) {}
    };

    // This struct contains individual app manifest field info
    struct ManifestFieldInfo
    {
        std::string Name;
        std::function<void(const YAML::Node&)> ProcessFunc;
        bool Required = false;
        std::string RegEx = {};
    };

    // This method takes YAML root node and list of manifest field info.
    // Yaml-cpp does not support case insensitive search and it allows duplicate keys. If duplicate keys exist,
    // the value is undefined. So in this method, we will iterate through the node map and process each individual
    // pair ourselves. This also helps with generating aggregated error rather than throwing on first failure.
    std::vector<ValidationError> ValidateAndProcessFields(
        const YAML::Node& rootNode,
        const std::vector<ManifestFieldInfo> fieldInfos,
        bool fullValidation);

    struct ManifestException : public wil::ResultException
    {
        ManifestException(std::vector<ValidationError>&& errors = {}, HRESULT hr = APPINSTALLER_CLI_ERROR_MANIFEST_FAILED) :
            m_errors(std::move(errors)), wil::ResultException(hr)
        {
            auto p = [&](ValidationError const& e) {
                return e.ErrorLevel == ValidationError::Level::Error;
            };

            m_warningOnly = !m_errors.empty() && std::find_if(m_errors.begin(), m_errors.end(), p) == m_errors.end();
        }

        ManifestException(HRESULT hr) : ManifestException({}, hr) {}

        // Error message without wil diagnostic info
        const std::string& GetManifestErrorMessage() const noexcept
        {
            if (m_manifestErrorMessage.empty())
            {
                if (m_errors.empty())
                {
                    // Syntax error, yaml-cpp error is stored in FailureInfo
                    m_manifestErrorMessage = Utility::ConvertToUTF8(GetFailureInfo().pszMessage);
                }
                else
                {
                    for (auto const& error : m_errors)
                    {
                        if (error.ErrorLevel == ValidationError::Level::Error)
                        {
                            m_manifestErrorMessage += ManifestError::ErrorMessagePrefix;
                        }
                        else if (error.ErrorLevel == ValidationError::Level::Warning)
                        {
                            m_manifestErrorMessage += ManifestError::WarningMessagePrefix;
                        }
                        m_manifestErrorMessage += error.Message;

                        if (!error.Field.empty())
                        {
                            m_manifestErrorMessage += " Field: " + error.Field;
                        }
                        if (!error.Value.empty())
                        {
                            m_manifestErrorMessage += " Value: " + error.Value;
                        }
                        if (error.Line >= 0 && error.Column >= 0)
                        {
                            m_manifestErrorMessage += " Line: " + std::to_string(error.Line + 1) + ", Column: " + std::to_string(error.Column + 1);
                        }
                        m_manifestErrorMessage += '\n';
                    }
                }
            }
            return m_manifestErrorMessage;
        }

        bool IsWarningOnly() const noexcept
        {
            return m_warningOnly;
        }

        const char* what() const noexcept override
        {
            if (m_whatMessage.empty())
            {
                m_whatMessage = ResultException::what();

                if (!m_errors.empty())
                {
                    m_whatMessage += GetManifestErrorMessage();
                }
            }
            return m_whatMessage.c_str();
        }

    private:
        std::vector<ValidationError> m_errors;
        mutable std::string m_whatMessage;
        mutable std::string m_manifestErrorMessage;
        bool m_warningOnly;
    };
}