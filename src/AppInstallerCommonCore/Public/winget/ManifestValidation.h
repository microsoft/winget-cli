// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
#include <winget/Manifest.h>

#include <wil/result.h>

#include <functional>
#include <string>

namespace YAML { class Node; }

namespace AppInstaller::Manifest
{
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
        const char* const FieldValueNotSupported = "Field value is not supported.";
        const char* const DuplicateInstallerEntry = "Duplicate installer entry found.";
        const char* const InstallerTypeDoesNotSupportPackageFamilyName = "The specified installer type does not support PackageFamilyName.";
        const char* const InstallerTypeDoesNotSupportProductCode = "The specified installer type does not support ProductCode.";
        const char* const IncompleteMultiFileManifest = "The multi file manifest is incomplete. A multi file manifest must contain at least version, installer and defaultLocale manifest.";
        const char* const InconsistentMultiFileManifestFieldValue = "The multi file manifest has inconsistent field values.";
        const char* const DuplicateMultiFileManifestType = "The multi file manifest should contain only one file with the particular ManifestType.";
        const char* const DuplicateMultiFileManifestLocale = "The multi file manifest contains duplicate PackageLocale.";
        const char* const UnsupportedMultiFileManifestType = "The multi file manifest should not contain file with the particular ManifestType.";
        const char* const InconsistentMultiFileManifestDefaultLocale = "DefaultLocale value in version manifest does not match PackageLocale value in defaultLocale manifest.";
        const char* const FieldFailedToProcess = "Failed to process field.";
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
        // line and column are 1 based
        size_t Line = 0;
        size_t Column = 0;
        Level ErrorLevel = Level::Error;
        std::string FileName;

        ValidationError(std::string message) :
            Message(std::move(message)) {}

        ValidationError(std::string message, Level level) :
            Message(std::move(message)), ErrorLevel(level) {}

        ValidationError(std::string message, std::string field) :
            Message(std::move(message)), Field(std::move(field)) {}

        ValidationError(std::string message, std::string field, std::string_view value) :
            Message(std::move(message)), Field(std::move(field)), Value(value) {}

        ValidationError(std::string message, std::string field, std::string value) :
            Message(std::move(message)), Field(std::move(field)), Value(std::move(value)) {}

        ValidationError(std::string message, std::string field, std::string value, size_t line, size_t column) :
            Message(std::move(message)), Field(std::move(field)), Value(std::move(value)), Line(line), Column(column) {}

        ValidationError(std::string message, std::string field, std::string value, size_t line, size_t column, Level level) :
            Message(std::move(message)), Field(std::move(field)), Value(std::move(value)), Line(line), Column(column), ErrorLevel(level) {}

        static ValidationError MessageWithFile(std::string message, std::string file)
        {
            ValidationError error{ message };
            error.FileName = file;
            return error;
        }

        static ValidationError MessageFieldWithFile(std::string message, std::string field, std::string file)
        {
            ValidationError error{ message, field };
            error.FileName = file;
            return error;
        }

        static ValidationError MessageFieldValueWithFile(std::string message, std::string field, std::string value, std::string file)
        {
            ValidationError error{ message, field, value };
            error.FileName = file;
            return error;
        }
    };

    struct ManifestException : public wil::ResultException
    {
        ManifestException(std::vector<ValidationError>&& errors = {}, HRESULT hr = APPINSTALLER_CLI_ERROR_MANIFEST_FAILED) :
            wil::ResultException(hr), m_errors(std::move(errors))
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
                    // Syntax error, yaml parser error is stored in FailureInfo
                    if (GetFailureInfo().pszMessage)
                    {
                        m_manifestErrorMessage = Utility::ConvertToUTF8(GetFailureInfo().pszMessage);
                    }
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
                        if (error.Line > 0 && error.Column > 0)
                        {
                            m_manifestErrorMessage += " Line: " + std::to_string(error.Line) + ", Column: " + std::to_string(error.Column);
                        }
                        if (!error.FileName.empty())
                        {
                            m_manifestErrorMessage += " File: " + error.FileName;
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

        const std::vector<ValidationError>& Errors() const { return m_errors; }

    private:
        std::vector<ValidationError> m_errors;
        mutable std::string m_whatMessage;
        mutable std::string m_manifestErrorMessage;
        bool m_warningOnly;
    };

    std::vector<ValidationError> ValidateManifest(const Manifest& manifest);
}