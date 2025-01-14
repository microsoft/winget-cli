// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "AppInstallerMsixInfo.h"
#include "winget/MsixManifest.h"
#include "winget/ManifestValidation.h"
#include "winget/MsixManifestValidation.h"
#include "winget/Locale.h"
#include "winget/Filesystem.h"

namespace AppInstaller::Manifest
{
    namespace
    {
        const auto& GetErrorIdToMessageMap()
        {
            static std::map<AppInstaller::StringResource::StringId, std::string_view> ErrorIdToMessageMap = {
                { AppInstaller::Manifest::ManifestError::InvalidRootNode, "Encountered unexpected root node."sv },
                { AppInstaller::Manifest::ManifestError::FieldUnknown, "Unknown field."sv },
                { AppInstaller::Manifest::ManifestError::FieldIsNotPascalCase, "All field names should be PascalCased."sv },
                { AppInstaller::Manifest::ManifestError::FieldDuplicate, "Duplicate field found in the manifest."sv },
                { AppInstaller::Manifest::ManifestError::RequiredFieldEmpty, "Required field with empty value."sv },
                { AppInstaller::Manifest::ManifestError::RequiredFieldMissing,  "Required field missing."sv },
                { AppInstaller::Manifest::ManifestError::InvalidFieldValue, "Invalid field value."sv },
                { AppInstaller::Manifest::ManifestError::ExeInstallerMissingSilentSwitches, "Silent and SilentWithProgress switches are not specified for InstallerType exe. Please make sure the installer can run unattended."sv },
                { AppInstaller::Manifest::ManifestError::FieldNotSupported, "Field is not supported."sv },
                { AppInstaller::Manifest::ManifestError::FieldValueNotSupported, "Field value is not supported."sv },
                { AppInstaller::Manifest::ManifestError::DuplicateInstallerEntry, "Duplicate installer entry found."sv },
                { AppInstaller::Manifest::ManifestError::InstallerTypeDoesNotSupportPackageFamilyName, "The specified installer type does not support PackageFamilyName."sv },
                { AppInstaller::Manifest::ManifestError::InstallerTypeDoesNotSupportProductCode, "The specified installer type does not support ProductCode."sv },
                { AppInstaller::Manifest::ManifestError::InstallerTypeDoesNotWriteAppsAndFeaturesEntry, "The specified installer type does not write to Apps and Features entry."sv },
                { AppInstaller::Manifest::ManifestError::IncompleteMultiFileManifest, "The multi file manifest is incomplete.A multi file manifest must contain at least version, installer and defaultLocale manifest."sv },
                { AppInstaller::Manifest::ManifestError::InconsistentMultiFileManifestFieldValue, "The multi file manifest has inconsistent field values."sv },
                { AppInstaller::Manifest::ManifestError::DuplicatePortableCommandAlias, "Duplicate portable command alias found."sv },
                { AppInstaller::Manifest::ManifestError::DuplicateRelativeFilePath, "Duplicate relative file path found."sv },
                { AppInstaller::Manifest::ManifestError::DuplicateMultiFileManifestType, "The multi file manifest should contain only one file with the particular ManifestType."sv },
                { AppInstaller::Manifest::ManifestError::DuplicateMultiFileManifestLocale, "The multi file manifest contains duplicate PackageLocale."sv },
                { AppInstaller::Manifest::ManifestError::UnsupportedMultiFileManifestType, "The multi file manifest should not contain file with the particular ManifestType."sv },
                { AppInstaller::Manifest::ManifestError::InconsistentMultiFileManifestDefaultLocale, "DefaultLocale value in version manifest does not match PackageLocale value in defaultLocale manifest."sv },
                { AppInstaller::Manifest::ManifestError::FieldFailedToProcess, "Failed to process field."sv },
                { AppInstaller::Manifest::ManifestError::InvalidBcp47Value, "The locale value is not a well formed bcp47 language tag."sv },
                { AppInstaller::Manifest::ManifestError::BothAllowedAndExcludedMarketsDefined, "Both AllowedMarkets and ExcludedMarkets defined."sv },
                { AppInstaller::Manifest::ManifestError::DuplicateReturnCodeEntry, "Duplicate installer return code found."sv },
                { AppInstaller::Manifest::ManifestError::FieldRequireVerifiedPublisher, "Field usage requires verified publishers."sv },
                { AppInstaller::Manifest::ManifestError::SingleManifestPackageHasDependencies, "Package has a single manifest and is a dependency of other manifests."sv },
                { AppInstaller::Manifest::ManifestError::MultiManifestPackageHasDependencies, "Deleting the manifest will be break the following dependencies."sv },
                { AppInstaller::Manifest::ManifestError::MissingManifestDependenciesNode, "Dependency not found: "sv },
                { AppInstaller::Manifest::ManifestError::NoSuitableMinVersionDependency,"No Suitable Minimum Version: "sv },
                { AppInstaller::Manifest::ManifestError::FoundDependencyLoop, "Loop found."sv },
                { AppInstaller::Manifest::ManifestError::ExceededAppsAndFeaturesEntryLimit, "Only zero or one entry for Apps and Features may be specified for InstallerType portable."sv },
                { AppInstaller::Manifest::ManifestError::ExceededCommandsLimit, "Only zero or one value for Commands may be specified for InstallerType portable."sv },
                { AppInstaller::Manifest::ManifestError::ScopeNotSupported, "Scope is not supported for InstallerType portable."sv },
                { AppInstaller::Manifest::ManifestError::InstallerMsixInconsistencies, "Inconsistent value in the manifest."sv },
                { AppInstaller::Manifest::ManifestError::OptionalFieldMissing, "Optional field missing."sv },
                { AppInstaller::Manifest::ManifestError::InstallerFailedToProcess, "Failed to process installer."sv },
                { AppInstaller::Manifest::ManifestError::NoSupportedPlatforms, "No supported platforms."sv },
                { AppInstaller::Manifest::ManifestError::ApproximateVersionNotAllowed, "Approximate version not allowed."sv },
                { AppInstaller::Manifest::ManifestError::ArpVersionOverlapWithIndex, "DisplayVersion declared in the manifest has overlap with existing DisplayVersion range in the index. Existing DisplayVersion range in index: "sv },
                { AppInstaller::Manifest::ManifestError::ArpVersionValidationInternalError, "Internal error while validating DisplayVersion against index."sv },
                { AppInstaller::Manifest::ManifestError::ExceededNestedInstallerFilesLimit, "Only one entry for NestedInstallerFiles can be specified for non-portable InstallerTypes."sv },
                { AppInstaller::Manifest::ManifestError::RelativeFilePathEscapesDirectory, "Relative file path must not point to a location outside of archive directory."sv },
                { AppInstaller::Manifest::ManifestError::ArpValidationError, "Arp Validation Error."sv },
                { AppInstaller::Manifest::ManifestError::SchemaError, "Schema Error."sv },
                { AppInstaller::Manifest::ManifestError::MsixSignatureHashFailed, "Failed to calculate MSIX signature hash.Please verify that the input file is a valid, signed MSIX."sv },
                { AppInstaller::Manifest::ManifestError::ShadowManifestNotAllowed, "Shadow manifest is not allowed." },
                { AppInstaller::Manifest::ManifestError::SchemaHeaderNotFound, "Schema header not found." },
                { AppInstaller::Manifest::ManifestError::InvalidSchemaHeader , "The schema header is invalid. Please verify that the schema header is present and formatted correctly."sv },
                { AppInstaller::Manifest::ManifestError::SchemaHeaderManifestTypeMismatch , "The manifest type in the schema header does not match the ManifestType property value in the manifest."sv },
                { AppInstaller::Manifest::ManifestError::SchemaHeaderManifestVersionMismatch, "The manifest version in the schema header does not match the ManifestVersion property value in the manifest."sv },
                { AppInstaller::Manifest::ManifestError::SchemaHeaderUrlPatternMismatch, "The schema header URL does not match the expected pattern."sv },
            };

            return ErrorIdToMessageMap;
        }
    }
    std::vector<ValidationError> ValidateManifest(const Manifest& manifest, bool fullValidation)
    {
        std::vector<ValidationError> resultErrors;

        // Channel is not supported currently
        if (!manifest.Channel.empty())
        {
            resultErrors.emplace_back(ManifestError::FieldNotSupported, "Channel", manifest.Channel);
        }

        try
        {
            // Version value should be successfully parsed
            Utility::Version testVersion{ manifest.Version };
            if (testVersion.IsApproximate())
            {
                resultErrors.emplace_back(ManifestError::ApproximateVersionNotAllowed, "PackageVersion", manifest.Version);
            }
        }
        catch (const std::exception&)
        {
            resultErrors.emplace_back(ManifestError::InvalidFieldValue, "PackageVersion", manifest.Version);
        }

        auto defaultLocErrors = ValidateManifestLocalization(manifest.DefaultLocalization, !fullValidation);
        std::move(defaultLocErrors.begin(), defaultLocErrors.end(), std::inserter(resultErrors, resultErrors.end()));

        // Comparison function to check duplicate installer entry. {installerType, arch, language and scope} combination is the key.
        // Todo: use the comparator from ManifestComparator when that one is fully implemented.
        auto installerCmp = [](const ManifestInstaller& in1, const ManifestInstaller& in2)
        {
            if (in1.BaseInstallerType != in2.BaseInstallerType)
            {
                return in1.BaseInstallerType < in2.BaseInstallerType;
            }
            else if (IsArchiveType(in1.BaseInstallerType))
            {
                // Compare nested installer type if base installer type is archive.
                if (in1.NestedInstallerType != in2.NestedInstallerType)
                {
                    return in1.NestedInstallerType < in2.NestedInstallerType;
                }
            }

            if (in1.Arch != in2.Arch)
            {
                return in1.Arch < in2.Arch;
            }

            if (in1.Locale != in2.Locale)
            {
                return in1.Locale < in2.Locale;
            }

            // Unknown is considered equal to all other values for uniqueness.
            // If either value is unknown, don't compare them.
            if (in1.Scope != in2.Scope && in1.Scope != ScopeEnum::Unknown && in2.Scope != ScopeEnum::Unknown)
            {
                return in1.Scope < in2.Scope;
            }

            return false;
        };

        std::set<ManifestInstaller, decltype(installerCmp)> installerSet(installerCmp);
        bool duplicateInstallerFound = false;

        // Validate installers
        for (auto const& installer : manifest.Installers)
        {
            // If not full validation, for future compatibility, skip validating unknown installers.
            if (installer.EffectiveInstallerType() == InstallerTypeEnum::Unknown && !fullValidation)
            {
                continue;
            }

            if (!duplicateInstallerFound && !installerSet.insert(installer).second)
            {
                AICLI_LOG(Core, Error, << "Duplicate installer: Type [" << InstallerTypeToString(installer.EffectiveInstallerType()) <<
                    "], Architecture [" << Utility::ToString(installer.Arch) << "], Locale [" << installer.Locale <<
                    "], Scope [" << ScopeToString(installer.Scope) << "]");

                resultErrors.emplace_back(ManifestError::DuplicateInstallerEntry);
                duplicateInstallerFound = true;
            }

            if (installer.Arch == Utility::Architecture::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Architecture");
            }

            if (installer.EffectiveInstallerType() == InstallerTypeEnum::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "InstallerType");
            }

            if (installer.UpdateBehavior == UpdateBehaviorEnum::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "UpgradeBehavior");
            }

            // Validate system reference strings if they are set at the installer level
            // Allow PackageFamilyName to be declared with non msix installers to support nested installer scenarios. But still report as warning to notify user of this uncommon case.
            if (!installer.PackageFamilyName.empty() && !DoesInstallerTypeUsePackageFamilyName(installer.EffectiveInstallerType()))
            {
                resultErrors.emplace_back(ManifestError::InstallerTypeDoesNotSupportPackageFamilyName, "InstallerType", std::string{ InstallerTypeToString(installer.EffectiveInstallerType()) }, ValidationError::Level::Warning);
            }

            if (!installer.ProductCode.empty() && !DoesInstallerTypeUseProductCode(installer.EffectiveInstallerType()))
            {
                resultErrors.emplace_back(ManifestError::InstallerTypeDoesNotSupportProductCode, "InstallerType", InstallerTypeToString(installer.EffectiveInstallerType()));
            }

            if (!installer.AppsAndFeaturesEntries.empty() && !DoesInstallerTypeWriteAppsAndFeaturesEntry(installer.EffectiveInstallerType()))
            {
                resultErrors.emplace_back(ManifestError::InstallerTypeDoesNotWriteAppsAndFeaturesEntry, "InstallerType", InstallerTypeToString(installer.EffectiveInstallerType()));
            }

            if (installer.EffectiveInstallerType() == InstallerTypeEnum::MSStore)
            {
                if (fullValidation)
                {
                    // MSStore type is not supported in community repo
                    resultErrors.emplace_back(
                        ManifestError::FieldValueNotSupported, "InstallerType",
                        InstallerTypeToString(installer.EffectiveInstallerType()));
                }

                if (installer.ProductId.empty())
                {
                    resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "ProductId");
                }
            }
            else
            {
                // For other types, Url and Sha256 are required
                if (installer.Url.empty())
                {
                    resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "InstallerUrl");
                }
                if (installer.Sha256.empty())
                {
                    resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "InstallerSha256");
                }
                // ProductId should not be used
                if (!installer.ProductId.empty())
                {
                    resultErrors.emplace_back(ManifestError::FieldNotSupported, "ProductId");
                }
            }

            if (installer.EffectiveInstallerType() == InstallerTypeEnum::Exe &&
                (installer.Switches.find(InstallerSwitchType::SilentWithProgress) == installer.Switches.end() ||
                 installer.Switches.find(InstallerSwitchType::Silent) == installer.Switches.end()))
            {
                resultErrors.emplace_back(ManifestError::ExeInstallerMissingSilentSwitches, ValidationError::Level::Warning);
            }

            // The command field restriction only applies if the base installer type is Portable.
            if (installer.BaseInstallerType == InstallerTypeEnum::Portable)
            {
                if (installer.Commands.size() > 1)
                {
                    resultErrors.emplace_back(ManifestError::ExceededCommandsLimit);
                }
            }

            if (installer.EffectiveInstallerType() == InstallerTypeEnum::Portable)
            {
                if (installer.AppsAndFeaturesEntries.size() > 1)
                {
                    resultErrors.emplace_back(ManifestError::ExceededAppsAndFeaturesEntryLimit);
                }
                if (installer.Scope != ScopeEnum::Unknown)
                {
                    resultErrors.emplace_back(ManifestError::ScopeNotSupported, ValidationError::Level::Warning);
                }
            }

            if (IsArchiveType(installer.BaseInstallerType))
            {
                if (installer.NestedInstallerType == InstallerTypeEnum::Unknown)
                {
                    resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "NestedInstallerType");
                }
                if (installer.NestedInstallerFiles.size() == 0)
                {
                    resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "NestedInstallerFiles");
                }
                if (installer.NestedInstallerType != InstallerTypeEnum::Portable && installer.NestedInstallerFiles.size() != 1)
                {
                    resultErrors.emplace_back(ManifestError::ExceededNestedInstallerFilesLimit, "NestedInstallerFiles");
                }

                std::set<std::string> commandAliasSet;
                std::set<std::string> relativeFilePathSet;

                for (const auto& nestedInstallerFile : installer.NestedInstallerFiles)
                {
                    if (nestedInstallerFile.RelativeFilePath.empty())
                    {
                        resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "RelativeFilePath");
                        break;
                    }

                    // Check that the relative file path does not escape base directory.
                    const std::filesystem::path& basePath = std::filesystem::current_path();
                    const std::filesystem::path& fullPath = basePath / ConvertToUTF16(nestedInstallerFile.RelativeFilePath);
                    if (AppInstaller::Filesystem::PathEscapesBaseDirectory(fullPath, basePath))
                    {
                        resultErrors.emplace_back(ManifestError::RelativeFilePathEscapesDirectory, "RelativeFilePath");
                    }

                    // Check for duplicate relative filepath values.
                    if (!relativeFilePathSet.insert(Utility::ToLower(nestedInstallerFile.RelativeFilePath)).second)
                    {
                        resultErrors.emplace_back(ManifestError::DuplicateRelativeFilePath, "RelativeFilePath");
                    }

                    // Check for duplicate portable command alias values.
                    const auto& alias = Utility::ToLower(nestedInstallerFile.PortableCommandAlias);
                    if (!alias.empty() && !commandAliasSet.insert(alias).second)
                    {
                        resultErrors.emplace_back(ManifestError::DuplicatePortableCommandAlias, "PortableCommandAlias");
                        break;
                    }
                }
            }

            // Check empty string before calling IsValidUrl to avoid duplicate error reporting.
            if (!installer.Url.empty() && IsValidURL(NULL, Utility::ConvertToUTF16(installer.Url).c_str(), 0) == S_FALSE)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "InstallerUrl", installer.Url);
            }

            if (!installer.Locale.empty() && !Locale::IsWellFormedBcp47Tag(installer.Locale))
            {
                resultErrors.emplace_back(ManifestError::InvalidBcp47Value, "InstallerLocale", installer.Locale);
            }

            if (!installer.Markets.AllowedMarkets.empty() && !installer.Markets.ExcludedMarkets.empty())
            {
                resultErrors.emplace_back(ManifestError::BothAllowedAndExcludedMarketsDefined);
            }

            // Check expected return codes for duplicates between successful and expected error codes
            std::set<DWORD> returnCodeSet;
            returnCodeSet.insert(installer.InstallerSuccessCodes.begin(), installer.InstallerSuccessCodes.end());
            for (const auto& code : installer.ExpectedReturnCodes)
            {
                if (!returnCodeSet.insert(code.first).second)
                {
                    resultErrors.emplace_back(ManifestError::DuplicateReturnCodeEntry);

                    // Stop checking to avoid repeated errors
                    break;
                }
            }

            // Check no approximate version declared for DisplayVersion in AppsAndFeatureEntries
            for (auto const& entry : installer.AppsAndFeaturesEntries)
            {
                if (!entry.DisplayVersion.empty())
                {
                    try
                    {
                        Utility::Version displayVersion{ entry.DisplayVersion };
                        if (displayVersion.IsApproximate())
                        {
                            resultErrors.emplace_back(ManifestError::ApproximateVersionNotAllowed, "DisplayVersion", entry.DisplayVersion);
                        }
                    }
                    catch (const std::exception&)
                    {
                        resultErrors.emplace_back(ManifestError::InvalidFieldValue, "DisplayVersion", entry.DisplayVersion);
                    }
                }
            }

            // Check AuthInfo validity. For full validation (community repo), authentication type must be none.
            if (installer.AuthInfo.Type != Authentication::AuthenticationType::None)
            {
                if (fullValidation)
                {
                    // Authentication is not supported (must be none) in community repo.
                    resultErrors.emplace_back(ManifestError::FieldNotSupported, "Authentication");
                }

                if (!installer.AuthInfo.ValidateIntegrity())
                {
                    resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Authentication");
                }
            }
        }

        // Validate localizations
        for (auto const& localization : manifest.Localizations)
        {
            auto locErrors = ValidateManifestLocalization(localization, !fullValidation);
            std::move(locErrors.begin(), locErrors.end(), std::inserter(resultErrors, resultErrors.end()));
        }

        return resultErrors;
    }

    std::vector<ValidationError> ValidateManifestLocalization(const ManifestLocalization& localization, bool treatErrorAsWarning)
    {
        std::vector<ValidationError> resultErrors;

        if (!localization.Locale.empty() && !Locale::IsWellFormedBcp47Tag(localization.Locale))
        {
            resultErrors.emplace_back(ManifestError::InvalidBcp47Value, "PackageLocale", localization.Locale, treatErrorAsWarning ? ValidationError::Level::Warning : ValidationError::Level::Error);
        }

        if (localization.Contains(Localization::Agreements))
        {
            const auto& agreements = localization.Get<Localization::Agreements>();
            for (const auto& agreement : agreements)
            {
                // At least one must be present
                if (agreement.Label.empty() && agreement.AgreementText.empty() && agreement.AgreementUrl.empty())
                {
                    resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Agreements", treatErrorAsWarning ? ValidationError::Level::Warning : ValidationError::Level::Error);
                }
            }
        }

        return resultErrors;
    }

    std::vector<ValidationError> ValidateManifestInstallers(const Manifest& manifest, bool treatErrorAsWarning)
    {
        std::vector<ValidationError> errors;
        auto validationErrorLevel = treatErrorAsWarning ? ValidationError::Level::Warning : ValidationError::Level::Error;
        MsixManifestValidation msixManifestValidation(validationErrorLevel);
        for (const auto& installer : manifest.Installers)
        {
            // Installer msix or msixbundle
            if (installer.EffectiveInstallerType() == InstallerTypeEnum::Msix)
            {
                auto installerErrors = msixManifestValidation.Validate(manifest, installer);
                std::move(installerErrors.begin(), installerErrors.end(), std::inserter(errors, errors.end()));
            }
        }

        return errors;
    }

    std::string ValidationError::GetErrorMessage() const
    {
        const auto& ErrorIdToMessageMap = GetErrorIdToMessageMap();
        const auto itr = ErrorIdToMessageMap.find(Message);

        if (itr != ErrorIdToMessageMap.end())
        {
            return std::string(itr->second);
        }
        
        return Utility::ConvertToUTF8(Message);
    }
}
