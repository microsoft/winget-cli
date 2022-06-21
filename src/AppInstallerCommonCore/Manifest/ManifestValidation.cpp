// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "AppInstallerMsixInfo.h"
#include "winget/MsixManifest.h"
#include "winget/ManifestValidation.h"
#include "winget/Locale.h"

namespace AppInstaller::Manifest
{
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
            if (in1.InstallerType != in2.InstallerType)
            {
                return in1.InstallerType < in2.InstallerType;
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
            if (installer.InstallerType == InstallerTypeEnum::Unknown && !fullValidation)
            {
                continue;
            }

            if (!duplicateInstallerFound && !installerSet.insert(installer).second)
            {
                AICLI_LOG(Core, Error, << "Duplicate installer: Type[" << InstallerTypeToString(installer.InstallerType) <<
                    "] Architecture[" << Utility::ToString(installer.Arch) << "] Locale[" << installer.Locale <<
                    "] Scope[" << ScopeToString(installer.Scope) << "]");

                resultErrors.emplace_back(ManifestError::DuplicateInstallerEntry);
                duplicateInstallerFound = true;
            }

            if (installer.Arch == Utility::Architecture::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Architecture");
            }

            if (installer.InstallerType == InstallerTypeEnum::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "InstallerType");
            }

            if (installer.UpdateBehavior == UpdateBehaviorEnum::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "UpdateBehavior");
            }

            // Validate system reference strings if they are set at the installer level
            // Allow PackageFamilyName to be declared with non msix installers to support nested installer scenarios after manifest version 1.1
            if (manifest.ManifestVersion <= ManifestVer{ s_ManifestVersionV1_1 } && !installer.PackageFamilyName.empty() && !DoesInstallerTypeUsePackageFamilyName(installer.InstallerType))
            {
                resultErrors.emplace_back(ManifestError::InstallerTypeDoesNotSupportPackageFamilyName, "InstallerType", InstallerTypeToString(installer.InstallerType));
            }

            if (!installer.ProductCode.empty() && !DoesInstallerTypeUseProductCode(installer.InstallerType))
            {
                resultErrors.emplace_back(ManifestError::InstallerTypeDoesNotSupportProductCode, "InstallerType", InstallerTypeToString(installer.InstallerType));
            }

            if (!installer.AppsAndFeaturesEntries.empty() && !DoesInstallerTypeWriteAppsAndFeaturesEntry(installer.InstallerType))
            {
                resultErrors.emplace_back(ManifestError::InstallerTypeDoesNotWriteAppsAndFeaturesEntry, "InstallerType", InstallerTypeToString(installer.InstallerType));
            }

            if (installer.InstallerType == InstallerTypeEnum::MSStore)
            {
                if (fullValidation)
                {
                    // MSStore type is not supported in community repo
                    resultErrors.emplace_back(
                        ManifestError::FieldValueNotSupported, "InstallerType",
                        InstallerTypeToString(installer.InstallerType));
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

            if (installer.InstallerType == InstallerTypeEnum::Exe &&
                (installer.Switches.find(InstallerSwitchType::SilentWithProgress) == installer.Switches.end() ||
                 installer.Switches.find(InstallerSwitchType::Silent) == installer.Switches.end()))
            {
                resultErrors.emplace_back(ManifestError::ExeInstallerMissingSilentSwitches, ValidationError::Level::Warning);
            }

            if (installer.InstallerType == InstallerTypeEnum::Portable)
            {
                if (installer.AppsAndFeaturesEntries.size() > 1)
                {
                    resultErrors.emplace_back(ManifestError::ExceededAppsAndFeaturesEntryLimit);
                }
                if (installer.Commands.size() > 1)
                {
                    resultErrors.emplace_back(ManifestError::ExceededCommandsLimit);
                }
                if (installer.Scope != ScopeEnum::Unknown)
                {
                    resultErrors.emplace_back(ManifestError::ScopeNotSupported, ValidationError::Level::Warning);
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

    // Validate msix and msixbundle installer manifest
    std::vector<ValidationError> ValidateMsixManifest(
        const Msix::PackageVersion& packageVersion,
        const ManifestInstaller& installer,
        Msix::MsixPackageManifestCache& msixManifestsCache,
        bool treatErrorAsWarning)
    {
        std::vector<ValidationError> errors;
        std::optional<Msix::OSVersion> installerMinOSVersion;

        try
        {
            if (!installer.MinOSVersion.empty())
            {
                installerMinOSVersion = std::make_optional<Msix::OSVersion>(installer.MinOSVersion);
            }
        }
        catch (const std::exception&)
        {
            errors.emplace_back(ManifestError::InvalidFieldValue, "MinimumOSVersion", installer.MinOSVersion);
        }

        std::vector<Msix::MsixPackageManifest> msixManifests;
        try
        {
            msixManifests = msixManifestsCache.GetAppPackageManifests(installer.Url);
        }
        catch (...)
        {
            errors.emplace_back(ManifestError::InstallerFailedToProcess, "InstallerUrl", installer.Url);
        }

        for (auto msixManifest : msixManifests)
        {
            // Validate package family name
            auto msixManifestIdentity = msixManifest.GetIdentity();
            auto msixPackageFamilyName = msixManifestIdentity.GetPackageFamilyName();
            if (!installer.PackageFamilyName.empty())
            {
                if (installer.PackageFamilyName != msixPackageFamilyName)
                {
                    errors.emplace_back(ManifestError::InstallerMsixInconsistencies, "PackageFamilyName", msixPackageFamilyName);
                }
            }
            // Yaml manifest missing package family name
            else
            {
                errors.emplace_back(
                    ManifestError::OptionalFieldMissing,
                    "PackageFamilyName",
                    msixPackageFamilyName,
                    treatErrorAsWarning ? ValidationError::Level::Warning : ValidationError::Level::Error);
            }

            // Validate package version
            auto msixVersion = msixManifestIdentity.GetVersion();
            if (msixVersion != packageVersion)
            {
                errors.emplace_back(ManifestError::InstallerMsixInconsistencies, "PackageVersion", msixVersion.ToString());
            }

            // Validate min OS version
            auto targetMinOSVersion = msixManifest.GetMinimumOSVersionForSupportedPlatforms();
            if (!targetMinOSVersion.has_value())
            {
                errors.emplace_back(ManifestError::NoSupportedPlatforms, "InstallerUrl", installer.Url);
            }
            else if (installerMinOSVersion.has_value())
            {
                if (targetMinOSVersion.value() != installerMinOSVersion.value())
                {
                    errors.emplace_back(ManifestError::InstallerMsixInconsistencies, "MinimumOSVersion", targetMinOSVersion.value().ToString());
                }
            }
            else
            {
                errors.emplace_back(
                    ManifestError::OptionalFieldMissing,
                    "MinimumOSVersion",
                    targetMinOSVersion.value().ToString(),
                    treatErrorAsWarning ? ValidationError::Level::Warning : ValidationError::Level::Error);
            }
        }

        return errors;
    }

    std::vector<ValidationError> ValidateManifestInstallers(const Manifest& manifest, bool treatErrorAsWarning)
    {
        std::vector<ValidationError> errors;
        Msix::MsixPackageManifestCache msixManifestsCache;
        for (const auto& installer : manifest.Installers)
        {
            // Installer msix or msixbundle
            if (installer.InstallerType == InstallerTypeEnum::Msix)
            {
                auto installerErrors = ValidateMsixManifest(Msix::PackageVersion(manifest.Version), installer, msixManifestsCache, treatErrorAsWarning);
                std::move(installerErrors.begin(), installerErrors.end(), std::inserter(errors, errors.end()));
            }
        }

        return errors;
    }
}