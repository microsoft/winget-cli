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

        ValidationError(std::string message, std::string field = "", std::string value = "", int line = -1, int column = -1) :
            Message(message), Field(field), Value(value), Line(line), Column(column) {}
    };

    // This struct contains individual app manifest field info
    struct ManifestFieldInfo
    {
        std::string Name;
        std::function<void(const YAML::Node&)> ProcessFunc;
        bool Required = false;
        std::string RegEx = "";
    };

    // This method takes YAML root node and list of manifest field info.
    // Yaml-cpp does not support case insensitive search and it allows duplicate keys. If duplicate keys exist,
    // the value is undefined. So in this method, we will iterate through the node map and process each individual
    // pair ourselves. This also helps with generating aggregated error rather than throwing on first failure.
    std::vector<ValidationError> ValidateAndProcessFields(const YAML::Node& rootNode, const std::vector<ManifestFieldInfo> fieldInfos);

    struct ManifestException : public wil::ResultException
    {
        ManifestException(std::vector<ValidationError>&& errors) :
            m_errors(std::move(errors)), wil::ResultException(APPINSTALLER_CLI_ERROR_MANIFEST_FAILED) {}

        const char* what() const noexcept override
        {
            if (m_message.empty())
            {
                m_message = ResultException::what();
                m_message += "Invalid manifest:\n";

                for (auto const& error : m_errors)
                {
                    m_message += error.Message + ' ';
                    if (!error.Field.empty())
                    {
                        m_message += "Field: " + error.Field + ' ';
                    }
                    if (!error.Value.empty())
                    {
                        m_message += "Value: " + error.Value + ' ';
                    }
                    if (error.Line >= 0 && error.Column >= 0)
                    {
                        m_message += "Line: " + std::to_string(error.Line) + ", Column: " + std::to_string(error.Column);
                    }
                    m_message += '\n';
                }
            }
            return m_message.c_str();
        }

    private:
        std::vector<ValidationError> m_errors;
        mutable std::string m_message;
    };
}