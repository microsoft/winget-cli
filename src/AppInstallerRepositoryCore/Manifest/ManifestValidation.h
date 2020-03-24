// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <string>
#include <functional>
#include <wil/result.h>
#include <AppInstallerErrors.h>

namespace YAML { class Node; }

namespace AppInstaller::Manifest
{
    namespace ManifestError
    {
        const char* const InvalidRootNode = "Manifest: Encountered unexpected root node.";
        const char* const FieldUnknown = "Manifest: Unknown field.";
        const char* const FieldIsNotPascalCase = "Manifest: All field names should be PascalCased.";
        const char* const FieldDuplicate = "Manifest: Duplicate field found in the manifest.";
        const char* const RequiredFieldEmpty = "Manifest: Required field with empty value.";
        const char* const RequiredFieldMissing = "Manifest: Required field missing.";
        const char* const InvalidFieldValue = "Manifest: Invalid field value.";
        const char* const ExeInstallerMissingSilentSwitches = "Manifest: Silent switches are required for InstallerType exe.";
        const char* const FieldNotSupported = "Manifest: Field is not supported.";
        const char* const DuplicateInstallerEntry = "Manifest: Duplicate installer entry found.";
    }

    struct ValidationError
    {
        std::string Message;
        std::string Field;
        std::string Value;
        int Line;
        int Column;

        ValidationError(std::string message, std::string field = {}, std::string value = {}, int line = -1, int column = -1) :
            Message(std::move(message)), Field(std::move(field)), Value(std::move(value)), Line(line), Column(column) {}
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
    std::vector<ValidationError> ValidateAndProcessFields(const YAML::Node& rootNode, const std::vector<ManifestFieldInfo> fieldInfos, bool fullValidation);

    struct ManifestException : public wil::ResultException
    {
        ManifestException(std::vector<ValidationError>&& errors = {}) :
            m_errors(std::move(errors)), wil::ResultException(APPINSTALLER_CLI_ERROR_MANIFEST_FAILED) {}

        // Error message without wil diagnostic info
        const std::string& GetManifestErrorMessage() const noexcept
        {
            if (m_manifestErrorMessage.empty())
            {
                if (m_errors.empty())
                {
                    // Syntax error, Yaml-cpp error is stored in FailureInfo
                    m_manifestErrorMessage = Utility::ConvertToUTF8(GetFailureInfo().pszMessage);
                }
                else
                {
                    for (auto const& error : m_errors)
                    {
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
                            m_manifestErrorMessage += " Line: " + std::to_string(error.Line) + ", Column: " + std::to_string(error.Column);
                        }
                        m_manifestErrorMessage += '\n';
                    }
                }
            }
            return m_manifestErrorMessage;
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
    };
}