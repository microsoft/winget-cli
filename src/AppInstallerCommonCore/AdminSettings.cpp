// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "AppInstallerLogging.h"
#include "AppInstallerStrings.h"
#include "winget/Settings.h"
#include "winget/AdminSettings.h"
#include "winget/GroupPolicy.h"
#include "winget/Yaml.h"

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;
    using namespace Utility;

    namespace
    {
        constexpr std::string_view s_AdminSettingsYaml_LocalManifestFiles = "LocalManifestFile"sv;

        // Attempts to read a single scalar value from the node.
        template<typename Value>
        bool TryReadScalar(const YAML::Node& rootNode, std::string_view name, Value& value)
        {
            YAML::Node valueNode = rootNode[std::string{ name }];

            if (!valueNode || !valueNode.IsScalar())
            {
                AICLI_LOG(Core, Info, << "Admin setting '" << name << "' was not found or did not contain the expected format");
                return false;
            }

            value = valueNode.as<Value>();
            return true;
        }

        struct AdminSettingValues
        {
            bool LocalManifestFiles = false;
        };

        struct AdminSettingsInternal
        {
            AdminSettingsInternal();

            void SetAdminSetting(AdminSetting setting, bool enabled);

            bool GetAdminSettingBoolValue(AdminSetting setting) const;

        private:
            void LoadAdminSettings();
            void SaveAdminSettings() const;

            AdminSettingValues m_settingValues;
        };

        AdminSettingsInternal::AdminSettingsInternal()
        {
            LoadAdminSettings();
        }

        void AdminSettingsInternal::SetAdminSetting(AdminSetting setting, bool enabled)
        {
            switch (setting)
            {
            case AdminSetting::LocalManifestFiles:
                m_settingValues.LocalManifestFiles = enabled;
                break;
            default:
                return;
            }

            SaveAdminSettings();
        }

        bool AdminSettingsInternal::GetAdminSettingBoolValue(AdminSetting setting) const
        {
            switch (setting)
            {
            case AdminSetting::LocalManifestFiles:
                return m_settingValues.LocalManifestFiles;
            default:
                return false;
            }
        }

        void AdminSettingsInternal::LoadAdminSettings()
        {
            auto stream = Settings::GetSettingStream(Settings::Streams::AdminSettings);
            if (!stream)
            {
                AICLI_LOG(Core, Info, << "Admin settings was not found");
                return;
            }

            std::string adminSettingsYaml = Utility::ReadEntireStream(*stream);

            YAML::Node document;
            try
            {
                document = YAML::Load(adminSettingsYaml);
            }
            catch (const std::exception& e)
            {
                AICLI_LOG(YAML, Error, << "Admin settings contained invalid YAML (" << e.what() << ")");
                return;
            }

            if (document.IsNull())
            {
                AICLI_LOG(Core, Info, << "Admin settings is empty");
                return;
            }

            if (!document.IsMap())
            {
                AICLI_LOG(Core, Error, << "Admin settings did not contain the expected format");
                return;
            }

            TryReadScalar<bool>(document, s_AdminSettingsYaml_LocalManifestFiles, m_settingValues.LocalManifestFiles);
        }

        void AdminSettingsInternal::SaveAdminSettings() const
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << s_AdminSettingsYaml_LocalManifestFiles << YAML::Value << m_settingValues.LocalManifestFiles;
            out << YAML::EndMap;

            Settings::SetSetting(Settings::Streams::AdminSettings, out.str());
        }
    }

    AdminSetting StringToAdminSetting(std::string_view in)
    {
        AdminSetting result = AdminSetting::Unknown;

        if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_LocalManifestFiles, in))
        {
            result = AdminSetting::LocalManifestFiles;
        }

        return result;
    }

    std::string_view AdminSettingToString(AdminSetting setting)
    {
        switch (setting)
        {
        case AdminSetting::LocalManifestFiles:
            return s_AdminSettingsYaml_LocalManifestFiles;
        default:
            return "Unknown"sv;
        }
    }

    void EnableAdminSetting(AdminSetting setting)
    {
        AdminSettingsInternal adminSettingsInternal;
        adminSettingsInternal.SetAdminSetting(setting, true);
    }

    void DisableAdminSetting(AdminSetting setting)
    {
        AdminSettingsInternal adminSettingsInternal;
        adminSettingsInternal.SetAdminSetting(setting, false);
    }

    bool IsAdminSettingEnabled(AdminSetting setting)
    {
        AdminSettingsInternal adminSettingsInternal;
        bool isEnabled = adminSettingsInternal.GetAdminSettingBoolValue(setting);

        // For some admin settings, even if it's disabled, if the corresponding policy is enabled then override it.
        if (setting == AdminSetting::LocalManifestFiles &&
            GroupPolicies().GetState(TogglePolicy::Policy::LocalManifestFiles) == PolicyState::Enabled)
        {
            isEnabled = true;
        }

        return isEnabled;
    }
}
