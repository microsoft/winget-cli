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
    using namespace Utility::literals;

    namespace
    {
        constexpr Utility::LocIndView s_AdminSettingsYaml_LocalManifestFiles = "LocalManifestFiles"_liv;
        constexpr Utility::LocIndView s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore = "BypassCertificatePinningForMicrosoftStore"_liv;
        constexpr Utility::LocIndView s_AdminSettingsYaml_InstallerHashOverride = "InstallerHashOverride"_liv;
        constexpr Utility::LocIndView s_AdminSettingsYaml_LocalArchiveMalwareScanOverride = "LocalArchiveMalwareScanOverride"_liv;

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
            bool InstallerHashOverride = false;
            bool LocalArchiveMalwareScanOverride = false;
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
                case AdminSetting::InstallerHashOverride:
                    m_settingValues.InstallerHashOverride = enabled;
                    break;
                case AdminSetting::LocalArchiveMalwareScanOverride:
                    m_settingValues.LocalArchiveMalwareScanOverride = enabled;
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
            case AdminSetting::InstallerHashOverride:
                return m_settingValues.InstallerHashOverride;
            case AdminSetting::LocalArchiveMalwareScanOverride:
                return m_settingValues.LocalArchiveMalwareScanOverride;
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
            TryReadScalar<bool>(document, s_AdminSettingsYaml_InstallerHashOverride, m_settingValues.InstallerHashOverride);
            TryReadScalar<bool>(document, s_AdminSettingsYaml_LocalArchiveMalwareScanOverride, m_settingValues.LocalArchiveMalwareScanOverride);
        }

        bool AdminSettingsInternal::SaveAdminSettings()
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << s_AdminSettingsYaml_LocalManifestFiles << YAML::Value << m_settingValues.LocalManifestFiles;
            out << YAML::Key << s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore << YAML::Value << m_settingValues.BypassCertificatePinningForMicrosoftStore;
            out << YAML::Key << s_AdminSettingsYaml_InstallerHashOverride << YAML::Value << m_settingValues.InstallerHashOverride;
            out << YAML::Key << s_AdminSettingsYaml_LocalArchiveMalwareScanOverride << YAML::Value << m_settingValues.LocalArchiveMalwareScanOverride;
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
        else if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_InstallerHashOverride, in))
        {
            result = AdminSetting::InstallerHashOverride;
        }
        else if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_LocalArchiveMalwareScanOverride, in))
        {
            result = AdminSetting::LocalArchiveMalwareScanOverride;
        }

        return result;
    }

    Utility::LocIndView AdminSettingToString(AdminSetting setting)
    {
        switch (setting)
        {
        case AdminSetting::LocalManifestFiles:
            return s_AdminSettingsYaml_LocalManifestFiles;
        case AdminSetting::BypassCertificatePinningForMicrosoftStore:
            return s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore;
        case AdminSetting::InstallerHashOverride:
            return s_AdminSettingsYaml_InstallerHashOverride;
        case AdminSetting::LocalArchiveMalwareScanOverride:
            return s_AdminSettingsYaml_LocalArchiveMalwareScanOverride;
        default:
            return "Unknown"_liv;
        }
    }

    TogglePolicy::Policy GetAdminSettingPolicy(AdminSetting setting)
    {
        switch (setting)
        {
        case AdminSetting::LocalManifestFiles:
            return TogglePolicy::Policy::LocalManifestFiles;
        case AdminSetting::BypassCertificatePinningForMicrosoftStore:
            return TogglePolicy::Policy::BypassCertificatePinningForMicrosoftStore;
        case AdminSetting::InstallerHashOverride:
            return TogglePolicy::Policy::HashOverride;
        case AdminSetting::LocalArchiveMalwareScanOverride:
            return TogglePolicy::Policy::LocalArchiveMalwareScanOverride;
        default:
            return TogglePolicy::Policy::None;
        }
    }

    bool EnableAdminSetting(AdminSetting setting)
    {
        auto policy = GetAdminSettingPolicy(setting);
        if (GroupPolicies().GetState(policy) == PolicyState::Disabled)
        {
            return false;
        }

        AdminSettingsInternal adminSettingsInternal;
        adminSettingsInternal.SetAdminSetting(setting, true);
        return true;
    }

    bool DisableAdminSetting(AdminSetting setting)
    {
        auto policy = GetAdminSettingPolicy(setting);
        if (GroupPolicies().GetState(policy) == PolicyState::Enabled)
        {
            return false;
        }

        AdminSettingsInternal adminSettingsInternal;
        adminSettingsInternal.SetAdminSetting(setting, false);
        return true;
    }

    bool IsAdminSettingEnabled(AdminSetting setting)
    {
        // Check for a policy that overrides this setting.
        auto policy = GetAdminSettingPolicy(setting);
        auto policyState = GroupPolicies().GetState(policy);
        if (policyState != PolicyState::NotConfigured)
        {
            return policyState == PolicyState::Enabled;
        }

        AdminSettingsInternal adminSettingsInternal;
        return adminSettingsInternal.GetAdminSettingBoolValue(setting);
    }

    std::vector<AdminSetting> GetAllAdminSettings()
    {
        std::vector<AdminSetting> result;
        using AdminSetting_t = std::underlying_type_t<AdminSetting>;

        // Skip Unknown.
        for (AdminSetting_t i = 1 + static_cast<AdminSetting_t>(AdminSetting::Unknown); i < static_cast<AdminSetting_t>(AdminSetting::Max); ++i)
        {
            result.emplace_back(static_cast<AdminSetting>(i));;
        }

        return result;
    }
}
