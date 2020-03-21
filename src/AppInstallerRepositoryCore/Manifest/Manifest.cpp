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
                if (!splitVal.empty())
                {
                    result.emplace_back(std::move(splitVal));
                }
                currentPos = splitPos + 1;
            }

            return result;
        }
    }

    std::vector<ValidationError> Manifest::PopulateManifestFields(const YAML::Node& rootNode, bool extraValidation)
    {
        const std::vector<ManifestFieldInfo> FieldInfos =
        {
            { "Id", [this](const YAML::Node& value) { Id = value.as<std::string>(); }, true, "^[\\S]+\\.[\\S]+$" },
            { "Name", [this](const YAML::Node& value) { Name = value.as<std::string>(); }, true },
            { "Version", [this](const YAML::Node& value) { Version = value.as<std::string>(); }, true,
              "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])(\\.(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])){0,3}$" },
            { "Publisher", [this](const YAML::Node& value) { Publisher = value.as<std::string>(); }, true },
            { "AppMoniker", [this](const YAML::Node& value) { AppMoniker = value.as<std::string>(); } },
            { "Channel", [this](const YAML::Node& value) { Channel = value.as<std::string>(); } },
            { "Author", [this](const YAML::Node& value) { Author = value.as<std::string>(); } },
            { "License", [this](const YAML::Node& value) { License = value.as<std::string>(); } },
            { "MinOSVersion", [this](const YAML::Node& value) { MinOSVersion = value.as<std::string>(); }, false,
              "^(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])(\\.(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])){0,3}$" },
            { "Tags", [this](const YAML::Node& value) { Tags = SplitMultiValueField(value.as<std::string>()); } },
            { "Commands", [this](const YAML::Node& value) { Commands = SplitMultiValueField(value.as<std::string>()); } },
            { "Protocols", [this](const YAML::Node& value) { Protocols = SplitMultiValueField(value.as<std::string>()); } },
            { "FileExtensions", [this](const YAML::Node& value) { FileExtensions = SplitMultiValueField(value.as<std::string>()); } },
            { "InstallerType", [this](const YAML::Node& value) { InstallerType = ManifestInstaller::ConvertToInstallerTypeEnum(value.as<std::string>()); } },
            { "Description", [this](const YAML::Node& value) { Description = value.as<std::string>(); } },
            { "Homepage", [this](const YAML::Node& value) { Homepage = value.as<std::string>(); } },
            { "LicenseUrl", [this](const YAML::Node& value) { LicenseUrl = value.as<std::string>(); } },
            { "Switches", [this](const YAML::Node& value) { m_switchesNode = value; } },
            { "Installers", [this](const YAML::Node& value) { m_installersNode = value; }, true },
            { "Localization", [this](const YAML::Node& value) { m_localizationNode = value; } },
        };

        std::vector<ValidationError> resultErrors = ValidateAndProcessFields(rootNode, FieldInfos);

        if (!m_switchesNode.IsNull())
        {
            auto errors = ManifestInstaller::PopulateSwitchesFields(m_switchesNode, this->Switches);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
        }

        // Create default ManifestInstaller to be used to populate default value when optional fields are not found.
        ManifestInstaller defaultInstaller;
        defaultInstaller.InstallerType = this->InstallerType;
        defaultInstaller.Switches = this->Switches;

        for (std::size_t i = 0; i < m_installersNode.size(); i++) {
            YAML::Node installerNode = m_installersNode[i];
            ManifestInstaller installer;
            auto errors = installer.PopulateInstallerFields(installerNode, defaultInstaller, extraValidation);
            std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
            this->Installers.emplace_back(std::move(installer));
        }

        // Create default ManifestLocalization to be used to populate default value when optional fields are not found.
        ManifestLocalization defaultLocalization;
        defaultLocalization.Description = this->Description;
        defaultLocalization.Homepage = this->Homepage;
        defaultLocalization.LicenseUrl = this->LicenseUrl;

        if (!m_localizationNode.IsNull())
        {
            for (std::size_t i = 0; i < m_localizationNode.size(); i++) {
                YAML::Node localizationNode = m_localizationNode[i];
                ManifestLocalization localization;
                auto errors = localization.PopulateLocalizationFields(localizationNode, defaultLocalization);
                std::move(errors.begin(), errors.end(), std::inserter(resultErrors, resultErrors.end()));
                this->Localization.emplace_back(std::move(localization));
            }
        }

        // Extra semantic validations after basic validation and field population
        if (extraValidation)
        {
            // Channel is not supported
            if (!Channel.empty())
            {
                resultErrors.emplace_back(ManifestError::FieldNotSupported, "Channel", Channel);
            }

            // Check duplicate installer entry. {installerType, arch, language and scope} combination is the key.
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

    Manifest Manifest::CreateFromPath(const std::filesystem::path& inputFile, bool extraValidation)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            YAML::Node rootNode = YAML::LoadFile(inputFile.u8string());
            errors = manifest.PopulateManifestFields(rootNode, extraValidation);
        }
        catch (std::runtime_error& e)
        {
            AICLI_LOG(YAML, Error, << "Failed to create manifest from file: " << inputFile.u8string());
            THROW_EXCEPTION_MSG(ManifestException(std::move(errors)), e.what());
        }

        if (errors.size() > 0)
        {
            THROW_EXCEPTION(ManifestException(std::move(errors)));
        }

        return manifest;
    }

    Manifest Manifest::Create(const std::string& input, bool extraValidation)
    {
        Manifest manifest;
        std::vector<ValidationError> errors;

        try
        {
            YAML::Node rootNode = YAML::Load(input);
            errors = manifest.PopulateManifestFields(rootNode, extraValidation);
        }
        catch (std::runtime_error& e)
        {
            AICLI_LOG(YAML, Error, << "Failed to create manifest: " << input);
            THROW_EXCEPTION_MSG(ManifestException(std::move(errors)), e.what());
        }

        if (errors.size() > 0)
        {
            THROW_EXCEPTION(ManifestException(std::move(errors)));
        }

        return manifest;
    }
}
