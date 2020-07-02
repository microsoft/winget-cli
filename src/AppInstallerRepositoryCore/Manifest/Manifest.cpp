// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Manifest.h"

namespace AppInstaller::Manifest
{
    namespace
    {
        std::vector<Manifest::string_t> SplitMultiValueField(const std::string& input)
        {
            if (input.empty())
            {
                return {};
            }

            std::vector<Manifest::string_t> result;
            size_t currentPos = 0;
            while (currentPos < input.size())
            {
                size_t splitPos = input.find(',', currentPos);
                if (splitPos == std::string::npos)
                {
                    splitPos = input.size();
                }

                std::string splitVal = input.substr(currentPos, splitPos - currentPos);
                Utility::Trim(splitVal);
                if (!splitVal.empty())
                {
                    result.emplace_back(std::move(splitVal));
                }
                currentPos = splitPos + 1;
            }

            return result;
        }
    }

    std::vector<ValidationError> Manifest::PopulateManifestFields(const YAML::Node& rootNode, bool fullValidation)
    {
        // Detect manifest version first to determine expected fields
        // Use index to access ManifestVersion directly. If there're duplicates or other general errors, it'll be detected in later
        // processing of iterating the whole manifest.
        // Todo: make ManifestVersion required when all manifests in our repo have been updated to contain a ManifestVersion
        if (rootNode["ManifestVersion"])
        {
            auto manifestVersionValue = rootNode["ManifestVersion"].as<std::string>();
            ManifestVersion = ManifestVer(manifestVersionValue, fullValidation);
        }
        else
        {
            ManifestVersion = PreviewManifestVersion;
        }

        // Check manifest version is supported
        if (ManifestVersion.Major() > MaxSupportedMajorVersion)
        {
            THROW_EXCEPTION_MSG(ManifestException(APPINSTALLER_CLI_ERROR_UNSUPPORTED_MANIFESTVERSION), "Unsupported ManifestVersion: %S", ManifestVersion.ToString().c_str());
        }

        std::vector<ManifestFieldInfo> fieldInfos =
        {
            { "ManifestVersion", [this](const YAML::Node&) { /* ManifestVersion already processed */ }, false,
              // Regex here is to prevent leading 0s in the version, this also keeps consistent with other versions in the manifest
              "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])(\\.(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])){2}$" },
        };

        YAML::Node switchesNode;
        YAML::Node installersNode;
        YAML::Node localizationsNode;

        // Todo: The FieldInfo can be a table with an entry specifying which version the field is introduced
        // so that we can query applicable fields given a ManifestVersion
        if (ManifestVersion >= PreviewManifestVersion)
        {
            // Add preview fields
            std::vector<ManifestFieldInfo> previewFieldInfos =
            {
                { "Id", [this](const YAML::Node& value) { Id = value.as<std::string>(); Utility::Trim(Id); }, true, "^[\\S]+\\.[\\S]+$" },
                { "Name", [this](const YAML::Node& value) { Name = value.as<std::string>(); Utility::Trim(Name); }, true },
                { "Version", [this](const YAML::Node& value) { Version = value.as<std::string>(); Utility::Trim(Version); }, true,
                  /* File name chars not allowed */ "^[^\\\\/:\\*\\?\"<>\\|\\x01-\\x1f]+$" },
                { "Publisher", [this](const YAML::Node& value) { Publisher = value.as<std::string>(); }, true },
                { "AppMoniker", [this](const YAML::Node& value) { AppMoniker = value.as<std::string>(); Utility::Trim(AppMoniker); } },
                { "Channel", [this](const YAML::Node& value) { Channel = value.as<std::string>(); Utility::Trim(Channel); } },
                { "Author", [this](const YAML::Node& value) { Author = value.as<std::string>(); } },
                { "License", [this](const YAML::Node& value) { License = value.as<std::string>(); } },
                { "MinOSVersion", [this](const YAML::Node& value) { MinOSVersion = value.as<std::string>(); Utility::Trim(MinOSVersion); }, false,
                  "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])(\\.(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])){0,3}$" },
                { "Tags", [this](const YAML::Node& value) { Tags = SplitMultiValueField(value.as<std::string>()); } },
                { "Commands", [this](const YAML::Node& value) { Commands = SplitMultiValueField(value.as<std::string>()); } },
                { "Protocols", [this](const YAML::Node& value) { Protocols = SplitMultiValueField(value.as<std::string>()); } },
                { "FileExtensions", [this](const YAML::Node& value) { FileExtensions = SplitMultiValueField(value.as<std::string>()); } },
                { "InstallerType", [this](const YAML::Node& value) { InstallerType = ManifestInstaller::ConvertToInstallerTypeEnum(value.as<std::string>()); } },
                { "Description", [this](const YAML::Node& value) { Description = value.as<std::string>(); } },
                { "Homepage", [this](const YAML::Node& value) { Homepage = value.as<std::string>(); } },
                { "LicenseUrl", [this](const YAML::Node& value) { LicenseUrl = value.as<std::string>(); } },
                { "Switches", [&](const YAML::Node& value) { switchesNode = value; } },
                { "Installers", [&](const YAML::Node& value) { installersNode = value; }, true },
                { "Localization", [&](const YAML::Node& value) { localizationsNode = value; } },
            };

            std::move(previewFieldInfos.begin(), previewFieldInfos.end(), std::inserter(fieldInfos, fieldInfos.end()));
        }

        std::vector<ValidationError> resultErrors = ValidateAndProcessFields(rootNode, fieldInfos, fullValidation);

        if (!switchesNode.IsNull())
        {
            auto errors = ManifestInstaller::PopulateSwitchesFields(switchesNode, this->Switches, fullValidation, ManifestVersion);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
        }

        // Create default ManifestInstaller to be used to populate default value when optional fields are not found.
        ManifestInstaller defaultInstaller;
        defaultInstaller.InstallerType = this->InstallerType;
        defaultInstaller.Switches = this->Switches;

        for (std::size_t i = 0; i < installersNode.size(); i++) {
            YAML::Node installerNode = installersNode[i];
            ManifestInstaller installer;
            auto errors = installer.PopulateInstallerFields(installerNode, defaultInstaller, fullValidation, ManifestVersion);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            this->Installers.emplace_back(std::move(installer));
        }

        // Create default ManifestLocalization to be used to populate default value when optional fields are not found.
        ManifestLocalization defaultLocalization;
        defaultLocalization.Description = this->Description;
        defaultLocalization.Homepage = this->Homepage;
        defaultLocalization.LicenseUrl = this->LicenseUrl;

        if (!localizationsNode.IsNull())
        {
            for (std::size_t i = 0; i < localizationsNode.size(); i++) {
                YAML::Node localizationNode = localizationsNode[i];
                ManifestLocalization localization;
                auto errors = localization.PopulateLocalizationFields(localizationNode, defaultLocalization, fullValidation, ManifestVersion);
                std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
                this->Localization.emplace_back(std::move(localization));
            }
        }

        // Extra semantic validations after basic validation and field population
        if (fullValidation)
        {
            // Channel is not supported currently
            if (!Channel.empty())
            {
                resultErrors.emplace_back(ManifestError::FieldNotSupported, "Channel", Channel);
            }

            try
            {
                // Version value should be successfully parsed
                Utility::Version test{ Version };
            }
            catch (const std::exception&)
            {
                resultErrors.emplace_back(ManifestError::InvalidFieldValue, "Version", Version);
            }

            // License field is required
            if (License.empty())
            {
                resultErrors.emplace_back(ManifestError::RequiredFieldMissing, "License");
            }

            // Check duplicate installer entry. {installerType, arch, language and scope} combination is the key.
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

                if (in1.Language != in2.Language)
                {
                    return in1.Language < in2.Language;
                }

                if (in1.Scope != in2.Scope)
                {
                    return in1.Scope < in2.Scope;
                }

                return false;
            };

            std::set<ManifestInstaller, decltype(installerCmp)> installerSet(installerCmp);

            for (auto const& installer : Installers)
            {
                if (!installerSet.insert(installer).second)
                {
                    resultErrors.emplace_back(ManifestError::DuplicateInstallerEntry);
                    break;
                }
            }
        }

        return resultErrors;
    }

    Manifest Manifest::CreateFromPath(const std::filesystem::path& inputFile, bool fullValidation, bool throwOnWarning)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            std::ifstream inputStream(inputFile);
            YAML::Node rootNode = YAML::Load(inputStream);
            errors = manifest.PopulateManifestFields(rootNode, fullValidation);
        }
        catch (const ManifestException&)
        {
            // Prevent ManifestException from being wrapped in another ManifestException
            throw;
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        if (!errors.empty())
        {
            ManifestException ex{ std::move(errors) };

            if (throwOnWarning || !ex.IsWarningOnly())
            {
                THROW_EXCEPTION(ex);
            }
        }

        return manifest;
    }

    Manifest Manifest::Create(const std::string& input, bool fullValidation, bool throwOnWarning)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            YAML::Node rootNode = YAML::Load(input);
            errors = manifest.PopulateManifestFields(rootNode, fullValidation);
        }
        catch (const ManifestException&)
        {
            // Prevent ManifestException from being wrapped in another ManifestException
            throw;
        }
        catch (const std::exception& e)
        {
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        if (!errors.empty())
        {
            ManifestException ex{ std::move(errors) };

            if (throwOnWarning || !ex.IsWarningOnly())
            {
                THROW_EXCEPTION(ex);
            }
        }

        return manifest;
    }
}
