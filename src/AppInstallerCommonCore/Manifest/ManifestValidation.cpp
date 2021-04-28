// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ManifestValidation.h"

namespace AppInstaller::Manifest
{
    std::vector<ValidationError> ValidateManifest(const Manifest& manifest)
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
            Utility::Version test{ manifest.Version };
        }
        catch (const std::exception&)
        {
            resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Version", manifest.Version);
        }

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
            if (!duplicateInstallerFound && !installerSet.insert(installer).second)
            {
                resultErrors.emplace_back(ManifestError::DuplicateInstallerEntry);
                duplicateInstallerFound = true;
            }

            if (installer.Arch == Utility::Architecture::Unknown)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Arch");
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
            if (!installer.PackageFamilyName.empty() && !DoesInstallerTypeUsePackageFamilyName(installer.InstallerType))
            {
                resultErrors.emplace_back(ManifestError::InstallerTypeDoesNotSupportPackageFamilyName, "InstallerType", InstallerTypeToString(installer.InstallerType));
            }

            if (!installer.ProductCode.empty() && !DoesInstallerTypeUseProductCode(installer.InstallerType))
            {
                resultErrors.emplace_back(ManifestError::InstallerTypeDoesNotSupportProductCode, "InstallerType", InstallerTypeToString(installer.InstallerType));
            }

            if (installer.InstallerType == InstallerTypeEnum::MSStore)
            {
                // MSStore type is not supported in community repo
                resultErrors.emplace_back(
                    ManifestError::FieldValueNotSupported, "InstallerType",
                    InstallerTypeToString(installer.InstallerType));

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
                    resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "Url");
                }
                if (installer.Sha256.empty())
                {
                    resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "Sha256");
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

            // Check empty string before calling IsValidUrl to avoid duplicate error reporting.
            if (!installer.Url.empty() && IsValidURL(NULL, Utility::ConvertToUTF16(installer.Url).c_str(), 0) == S_FALSE)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Url", installer.Url);
            }
        }

        return resultErrors;
    }
}