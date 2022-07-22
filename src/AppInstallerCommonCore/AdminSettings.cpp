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
        constexpr std::string_view s_AdminSettingsYaml_LocalManifestFiles = "LocalManifestFiles"sv;
        constexpr std::string_view s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore = "BypassCertificatePinningForMicrosoftStore"sv;

        // Attempts to read a single scalar value from the node.
        template<typename Value>
        bool TryReadScalar(const YAML::Node& rootNode, std::string_view name, Value& value)
        {
            YAML::Node valueNode = rootNode[std::string{ name }];

            if (!valueNode || !valueNode.IsScalar())
            {
                AICLI_LOG(Core, Verbose, << "Admin setting '" << name << "' was not found or did not contain the expected format");
                return false;
            }

            value = valueNode.as<Value>();
            return true;
        }

        struct AdminSettingValues
        {
            bool LocalManifestFiles = false;
            bool BypassCertificatePinningForMicrosoftStore = false;
        };

        struct AdminSettingsInternal
        {
            AdminSettingsInternal();

            void SetAdminSetting(AdminSetting setting, bool enabled);

            bool GetAdminSettingBoolValue(AdminSetting setting) const;

        private:
            void LoadAdminSettings();
            [[nodiscard]] bool SaveAdminSettings();

            Stream m_settingStream;
            AdminSettingValues m_settingValues;
        };

        AdminSettingsInternal::AdminSettingsInternal() : m_settingStream(Stream::AdminSettings)
        {
            LoadAdminSettings();
        }

        void AdminSettingsInternal::SetAdminSetting(AdminSetting setting, bool enabled)
        {
            for (size_t i = 0; i < 10; ++i)
            {
                switch (setting)
                {
                case AdminSetting::LocalManifestFiles:
                    m_settingValues.LocalManifestFiles = enabled;
                    break;
                case AdminSetting::BypassCertificatePinningForMicrosoftStore:
                    m_settingValues.BypassCertificatePinningForMicrosoftStore = enabled;
                    break;
                default:
                    return;
                }

                if (SaveAdminSettings())
                {
                    return;
                }

                // We need to reload the settings as they have changed
                LoadAdminSettings();
            }

            THROW_HR_MSG(E_UNEXPECTED, "Too many attempts at SaveAdminSettings");
        }

        bool AdminSettingsInternal::GetAdminSettingBoolValue(AdminSetting setting) const
        {
            switch (setting)
            {
            case AdminSetting::LocalManifestFiles:
                return m_settingValues.LocalManifestFiles;
            case AdminSetting::BypassCertificatePinningForMicrosoftStore:
                return m_settingValues.BypassCertificatePinningForMicrosoftStore;
            default:
                return false;
            }
        }

        void AdminSettingsInternal::LoadAdminSettings()
        {
            auto stream = m_settingStream.Get();
            if (!stream)
            {
                AICLI_LOG(Core, Verbose, << "Admin settings was not found");
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
            TryReadScalar<bool>(document, s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore, m_settingValues.BypassCertificatePinningForMicrosoftStore);
        }

        bool AdminSettingsInternal::SaveAdminSettings()
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << s_AdminSettingsYaml_LocalManifestFiles << YAML::Value << m_settingValues.LocalManifestFiles;
            out << YAML::Key << s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore << YAML::Value << m_settingValues.BypassCertificatePinningForMicrosoftStore;
            out << YAML::EndMap;

            return m_settingStream.Set(out.str());
        }
    }

    AdminSetting StringToAdminSetting(std::string_view in)
    {
        AdminSetting result = AdminSetting::Unknown;

        if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_LocalManifestFiles, in))
        {
            result = AdminSetting::LocalManifestFiles;
        }
        else if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore, in))
        {
            result = AdminSetting::BypassCertificatePinningForMicrosoftStore;
        }

        return result;
    }

    std::string_view AdminSettingToString(AdminSetting setting)
    {
        switch (setting)
        {
        case AdminSetting::LocalManifestFiles:
            return s_AdminSettingsYaml_LocalManifestFiles;
        case AdminSetting::BypassCertificatePinningForMicrosoftStore:
            return s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore;
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
        // Check for a policy that overrides this setting.
        if (setting == AdminSetting::LocalManifestFiles)
        {
            PolicyState localManifestFilesPolicy = GroupPolicies().GetState(TogglePolicy::Policy::LocalManifestFiles);
            if (localManifestFilesPolicy != PolicyState::NotConfigured)
            {
                return localManifestFilesPolicy == PolicyState::Enabled;
            }
        }

        AdminSettingsInternal adminSettingsInternal;
        return adminSettingsInternal.GetAdminSettingBoolValue(setting);
    }
}
