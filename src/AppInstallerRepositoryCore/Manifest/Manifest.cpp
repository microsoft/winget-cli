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

    std::vector<ValidationError> Manifest::PopulateManifestFields(const YAML::Node& rootNode)
    {
        const std::vector<ManifestFieldInfo> FieldInfos =
        {
            { "Id", [this](const YAML::Node& value) { Id = value.as<std::string>(); }, true },
            { "Name", [this](const YAML::Node& value) { Name = value.as<std::string>(); }, true },
            { "Version", [this](const YAML::Node& value) { Version = value.as<std::string>(); }, true },
            { "Publisher", [this](const YAML::Node& value) { Publisher = value.as<std::string>(); }, true },
            { "AppMoniker", [this](const YAML::Node& value) { AppMoniker = value.as<std::string>(); } },
            { "Channel", [this](const YAML::Node& value) { Channel = value.as<std::string>(); } },
            { "Author", [this](const YAML::Node& value) { Author = value.as<std::string>(); } },
            { "License", [this](const YAML::Node& value) { License = value.as<std::string>(); } },
            { "MinOSVersion", [this](const YAML::Node& value) { MinOSVersion = value.as<std::string>(); } },
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
            auto errors = installer.PopulateInstallerFields(installerNode, defaultInstaller);
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

        return resultErrors;
    }

    Manifest Manifest::CreateFromPath(const std::filesystem::path& inputFile)
    {
        Manifest manifest;

        try
        {
            YAML::Node rootNode = YAML::LoadFile(inputFile.u8string());
            manifest.PopulateManifestFields(rootNode);
        }
        catch (std::runtime_error& e)
        {
            AICLI_LOG(YAML, Error, << "Failed to create manifest from file: " << inputFile.u8string());
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        return manifest;
    }

    Manifest Manifest::Create(const std::string& input)
    {
        Manifest manifest;

        try
        {
            YAML::Node rootNode = YAML::Load(input);
            manifest.PopulateManifestFields(rootNode);
        }
        catch (std::runtime_error& e)
        {
            AICLI_LOG(YAML, Error, << "Failed to create manifest: " << input);
            THROW_EXCEPTION_MSG(ManifestException(), e.what());
        }

        return manifest;
    }
}
