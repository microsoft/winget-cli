// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerErrors.h>
#include <winget/Manifest.h>
#include <winget/Resources.h>

#include <wil/result.h>
#include <functional>

namespace YAML { class Node; }

namespace AppInstaller::Manifest
{
    namespace ManifestError
    {

        const char* const ErrorMessagePrefix = "Manifest Error: ";
        const char* const WarningMessagePrefix = "Manifest Warning: ";

        WINGET_DEFINE_RESOURCE_STRINGID(ApproximateVersionNotAllowed);
        WINGET_DEFINE_RESOURCE_STRINGID(ArpValidationError);
        WINGET_DEFINE_RESOURCE_STRINGID(ArpVersionOverlapWithIndex);
        WINGET_DEFINE_RESOURCE_STRINGID(ArpVersionValidationInternalError);
        WINGET_DEFINE_RESOURCE_STRINGID(BothAllowedAndExcludedMarketsDefined);
        WINGET_DEFINE_RESOURCE_STRINGID(DuplicatePortableCommandAlias);
        WINGET_DEFINE_RESOURCE_STRINGID(DuplicateRelativeFilePath);
        WINGET_DEFINE_RESOURCE_STRINGID(DuplicateMultiFileManifestLocale);
        WINGET_DEFINE_RESOURCE_STRINGID(DuplicateMultiFileManifestType);
        WINGET_DEFINE_RESOURCE_STRINGID(DuplicateInstallerEntry);
        WINGET_DEFINE_RESOURCE_STRINGID(DuplicateReturnCodeEntry);
        WINGET_DEFINE_RESOURCE_STRINGID(ExceededAppsAndFeaturesEntryLimit);
        WINGET_DEFINE_RESOURCE_STRINGID(ExceededCommandsLimit);
        WINGET_DEFINE_RESOURCE_STRINGID(ExceededNestedInstallerFilesLimit);
        WINGET_DEFINE_RESOURCE_STRINGID(ExeInstallerMissingSilentSwitches);
        WINGET_DEFINE_RESOURCE_STRINGID(FieldDuplicate);
        WINGET_DEFINE_RESOURCE_STRINGID(FieldFailedToProcess);
        WINGET_DEFINE_RESOURCE_STRINGID(FieldIsNotPascalCase);
        WINGET_DEFINE_RESOURCE_STRINGID(FieldNotSupported);
        WINGET_DEFINE_RESOURCE_STRINGID(FieldRequireVerifiedPublisher);
        WINGET_DEFINE_RESOURCE_STRINGID(FieldUnknown);
        WINGET_DEFINE_RESOURCE_STRINGID(FieldValueNotSupported);
        WINGET_DEFINE_RESOURCE_STRINGID(FoundDependencyLoop);
        WINGET_DEFINE_RESOURCE_STRINGID(IncompleteMultiFileManifest);
        WINGET_DEFINE_RESOURCE_STRINGID(InconsistentMultiFileManifestDefaultLocale);
        WINGET_DEFINE_RESOURCE_STRINGID(InconsistentMultiFileManifestFieldValue);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerFailedToProcess);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerMsixInconsistencies);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerTypeDoesNotSupportPackageFamilyName);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerTypeDoesNotSupportProductCode);
        WINGET_DEFINE_RESOURCE_STRINGID(InstallerTypeDoesNotWriteAppsAndFeaturesEntry);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidBcp47Value);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidFieldValue);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidRootNode);
        WINGET_DEFINE_RESOURCE_STRINGID(MissingManifestDependenciesNode);
        WINGET_DEFINE_RESOURCE_STRINGID(MsixSignatureHashFailed);
        WINGET_DEFINE_RESOURCE_STRINGID(MultiManifestPackageHasDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(NoSuitableMinVersionDependency);
        WINGET_DEFINE_RESOURCE_STRINGID(NoSupportedPlatforms);
        WINGET_DEFINE_RESOURCE_STRINGID(OptionalFieldMissing);
        WINGET_DEFINE_RESOURCE_STRINGID(RelativeFilePathEscapesDirectory);
        WINGET_DEFINE_RESOURCE_STRINGID(RequiredFieldEmpty);
        WINGET_DEFINE_RESOURCE_STRINGID(RequiredFieldMissing);
        WINGET_DEFINE_RESOURCE_STRINGID(SchemaError);
        WINGET_DEFINE_RESOURCE_STRINGID(ScopeNotSupported);
        WINGET_DEFINE_RESOURCE_STRINGID(ShadowManifestNotAllowed);
        WINGET_DEFINE_RESOURCE_STRINGID(SingleManifestPackageHasDependencies);
        WINGET_DEFINE_RESOURCE_STRINGID(UnsupportedMultiFileManifestType);
        WINGET_DEFINE_RESOURCE_STRINGID(SchemaHeaderNotFound);
        WINGET_DEFINE_RESOURCE_STRINGID(InvalidSchemaHeader);
        WINGET_DEFINE_RESOURCE_STRINGID(SchemaHeaderManifestTypeMismatch);
        WINGET_DEFINE_RESOURCE_STRINGID(SchemaHeaderManifestVersionMismatch);
        WINGET_DEFINE_RESOURCE_STRINGID(SchemaHeaderUrlPatternMismatch);
    }

    struct ValidationError
    {
        enum class Level
        {
            Warning,
            Error
        };

        AppInstaller::StringResource::StringId Message;
        std::string Context = {};
        std::string Value = {};
        // line and column are 1 based
        size_t Line = 0;
        size_t Column = 0;
        Level ErrorLevel = Level::Error;
        std::string FileName;

        ValidationError(AppInstaller::StringResource::StringId message) :
            Message(std::move(message)) {}

        ValidationError(AppInstaller::StringResource::StringId message, Level level) :
            Message(std::move(message)), ErrorLevel(level) {}

        ValidationError(AppInstaller::StringResource::StringId message, std::string context) :
            Message(std::move(message)), Context(std::move(context)) {}

        ValidationError(AppInstaller::StringResource::StringId message, std::string context, Level level) :
            Message(std::move(message)), Context(std::move(context)), ErrorLevel(level) {}

        ValidationError(AppInstaller::StringResource::StringId message, std::string context, std::string_view value) :
            Message(std::move(message)), Context(std::move(context)), Value(value) {}

        ValidationError(AppInstaller::StringResource::StringId message, std::string context, std::string value) :
            Message(std::move(message)), Context(std::move(context)), Value(std::move(value)) {}

        ValidationError(AppInstaller::StringResource::StringId message, std::string context, std::string value, Level level) :
            Message(std::move(message)), Context(std::move(context)), Value(std::move(value)), ErrorLevel(level) {}

        ValidationError(AppInstaller::StringResource::StringId message, std::string context, std::string value, size_t line, size_t column) :
            Message(std::move(message)), Context(std::move(context)), Value(std::move(value)), Line(line), Column(column) {}

        ValidationError(AppInstaller::StringResource::StringId message, std::string context, std::string value, size_t line, size_t column, Level level) :
            Message(std::move(message)), Context(std::move(context)), Value(std::move(value)), Line(line), Column(column), ErrorLevel(level) {}

        std::string GetErrorMessage() const;

        static ValidationError MessageWithFile(AppInstaller::StringResource::StringId message, std::string file)
        {
            ValidationError error{ message };
            error.FileName = file;
            return error;
        }

        static ValidationError MessageContextWithFile(AppInstaller::StringResource::StringId message, std::string context, std::string file)
        {
            ValidationError error{ message, context };
            error.FileName = file;
            return error;
        }

        static ValidationError MessageContextValueWithFile(AppInstaller::StringResource::StringId message, std::string context, std::string value, std::string file)
        {
            ValidationError error{ message, context, value };
            error.FileName = file;
            return error;
        }

        static ValidationError MessageLevelWithFile(AppInstaller::StringResource::StringId message, Level level, std::string file)
        {
            ValidationError error{ message, level };
            error.FileName = file;
            return error;
        }

        static ValidationError MessageContextValueLineLevelWithFile(AppInstaller::StringResource::StringId message, std::string context, std::string value, size_t line, size_t column , Level level , std::string file)
        {
            ValidationError error{ message, context, value, line, column, level };
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
                        m_manifestErrorMessage += error.GetErrorMessage();

                        if (!error.Context.empty())
                        {
                            m_manifestErrorMessage += " [" + error.Context + "]";
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

    // fullValidation: bool to set if manifest validation should perform extra validation that is not required for reading a manifest.
    std::vector<ValidationError> ValidateManifest(const Manifest& manifest, bool fullValidation = true);
    std::vector<ValidationError> ValidateManifestLocalization(const ManifestLocalization& localization, bool treatErrorAsWarning = false);
    std::vector<ValidationError> ValidateManifestInstallers(const Manifest& manifest, bool treatErrorAsWarning = false);
}
