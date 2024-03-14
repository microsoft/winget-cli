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
        constexpr Utility::LocIndView s_AdminSettingsYaml_ProxyCommandLineOptions = "ProxyCommandLineOptions"_liv;

        constexpr Utility::LocIndView s_AdminSettingsYaml_DefaultProxy = "DefaultProxy"_liv;

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
            bool ProxyCommandLineOptions = false;

            std::optional<std::string> DefaultProxy;
        };

        struct AdminSettingsInternal
        {
            AdminSettingsInternal();

            void SetAdminSetting(BoolAdminSetting setting, bool enabled);
            void SetAdminSetting(StringAdminSetting setting, const std::optional<std::string>& value);

            bool GetAdminSettingValue(BoolAdminSetting setting) const;
            std::optional<std::string> GetAdminSettingValue(StringAdminSetting setting) const;

        private:
            void LoadAdminSettings();
            [[nodiscard]] bool SaveAdminSettings();

            // Sets the value of an admin setting using the given function and then saves the changes.
            // Encapsulates the retry and reload logic.
            // Stops if the value cannot be set, as indicated by the return value of setValue()
            void SetAdminSettingAndSave(std::function<bool()> setValue);

            Stream m_settingStream;
            AdminSettingValues m_settingValues;
        };

        AdminSettingsInternal::AdminSettingsInternal() : m_settingStream(Stream::AdminSettings)
        {
            LoadAdminSettings();
        }

        void AdminSettingsInternal::SetAdminSettingAndSave(std::function<bool()> setValue)
        {
            for (size_t i = 0; i < 10; ++i)
            {
                if (!setValue())
                {
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

        void AdminSettingsInternal::SetAdminSetting(BoolAdminSetting setting, bool enabled)
        {
            SetAdminSettingAndSave([&]()
                {
                    switch (setting)
                    {
                    case BoolAdminSetting::LocalManifestFiles:
                        m_settingValues.LocalManifestFiles = enabled;
                        return true;
                    case BoolAdminSetting::BypassCertificatePinningForMicrosoftStore:
                        m_settingValues.BypassCertificatePinningForMicrosoftStore = enabled;
                        return true;
                    case BoolAdminSetting::InstallerHashOverride:
                        m_settingValues.InstallerHashOverride = enabled;
                        return true;
                    case BoolAdminSetting::LocalArchiveMalwareScanOverride:
                        m_settingValues.LocalArchiveMalwareScanOverride = enabled;
                        return true;
                    case BoolAdminSetting::ProxyCommandLineOptions:
                        m_settingValues.ProxyCommandLineOptions = enabled;
                        return true;
                    default:
                        return false;
                    }
                });
        }

        void AdminSettingsInternal::SetAdminSetting(StringAdminSetting setting, const std::optional<std::string>& value)
        {
            SetAdminSettingAndSave([&]()
                {
                    switch (setting)
                    {
                    case StringAdminSetting::DefaultProxy:
                        m_settingValues.DefaultProxy = value;
                        return true;
                    default:
                        return false;
                    }
                });
        }

        bool AdminSettingsInternal::GetAdminSettingValue(BoolAdminSetting setting) const
        {
            switch (setting)
            {
            case BoolAdminSetting::LocalManifestFiles:
                return m_settingValues.LocalManifestFiles;
            case BoolAdminSetting::BypassCertificatePinningForMicrosoftStore:
                return m_settingValues.BypassCertificatePinningForMicrosoftStore;
            case BoolAdminSetting::InstallerHashOverride:
                return m_settingValues.InstallerHashOverride;
            case BoolAdminSetting::LocalArchiveMalwareScanOverride:
                return m_settingValues.LocalArchiveMalwareScanOverride;
            case BoolAdminSetting::ProxyCommandLineOptions:
                return m_settingValues.ProxyCommandLineOptions;
            default:
                return false;
            }
        }

        std::optional<std::string> AdminSettingsInternal::GetAdminSettingValue(StringAdminSetting setting) const
        {
            switch (setting)
            {
            case StringAdminSetting::DefaultProxy:
                return m_settingValues.DefaultProxy;
            default:
                return std::nullopt;
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
            TryReadScalar<bool>(document, s_AdminSettingsYaml_ProxyCommandLineOptions, m_settingValues.ProxyCommandLineOptions);

            std::string defaultProxy;
            if (TryReadScalar<std::string>(document, s_AdminSettingsYaml_DefaultProxy, defaultProxy))
            {
                m_settingValues.DefaultProxy.emplace(std::move(defaultProxy));
            }
        }

        bool AdminSettingsInternal::SaveAdminSettings()
        {
            YAML::Emitter out;
            out << YAML::BeginMap;
            out << YAML::Key << s_AdminSettingsYaml_LocalManifestFiles << YAML::Value << m_settingValues.LocalManifestFiles;
            out << YAML::Key << s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore << YAML::Value << m_settingValues.BypassCertificatePinningForMicrosoftStore;
            out << YAML::Key << s_AdminSettingsYaml_InstallerHashOverride << YAML::Value << m_settingValues.InstallerHashOverride;
            out << YAML::Key << s_AdminSettingsYaml_LocalArchiveMalwareScanOverride << YAML::Value << m_settingValues.LocalArchiveMalwareScanOverride;
            out << YAML::Key << s_AdminSettingsYaml_ProxyCommandLineOptions << YAML::Value << m_settingValues.ProxyCommandLineOptions;

            if (m_settingValues.DefaultProxy)
            {
                out << YAML::Key << s_AdminSettingsYaml_DefaultProxy << YAML::Value << m_settingValues.DefaultProxy.value();
            }

            out << YAML::EndMap;

            return m_settingStream.Set(out.str());
        }

        auto GetPolicyStateForSetting(BoolAdminSetting setting)
        {
            auto policy = GetAdminSettingPolicy(setting);
            return GroupPolicies().GetState(policy);
        }

        std::optional<std::reference_wrapper<const std::string>> GetPolicyStateForSetting(StringAdminSetting setting)
        {
            switch (setting)
            {
            case AppInstaller::Settings::StringAdminSetting::DefaultProxy:
                return GroupPolicies().GetValueRef<ValuePolicy::DefaultProxy>();
            default:
                return std::nullopt;
            }
        }
    }

    BoolAdminSetting StringToBoolAdminSetting(std::string_view in)
    {
        BoolAdminSetting result = BoolAdminSetting::Unknown;

        if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_LocalManifestFiles, in))
        {
            result = BoolAdminSetting::LocalManifestFiles;
        }
        else if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore, in))
        {
            result = BoolAdminSetting::BypassCertificatePinningForMicrosoftStore;
        }
        else if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_InstallerHashOverride, in))
        {
            result = BoolAdminSetting::InstallerHashOverride;
        }
        else if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_LocalArchiveMalwareScanOverride, in))
        {
            result = BoolAdminSetting::LocalArchiveMalwareScanOverride;
        }
        else if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_ProxyCommandLineOptions, in))
        {
            result = BoolAdminSetting::ProxyCommandLineOptions;
        }

        return result;
    }

    StringAdminSetting StringToStringAdminSetting(std::string_view in)
    {
        StringAdminSetting result = StringAdminSetting::Unknown;

        if (Utility::CaseInsensitiveEquals(s_AdminSettingsYaml_DefaultProxy, in))
        {
            result = StringAdminSetting::DefaultProxy;
        }

        return result;
    }

    Utility::LocIndView AdminSettingToString(BoolAdminSetting setting)
    {
        switch (setting)
        {
        case BoolAdminSetting::LocalManifestFiles:
            return s_AdminSettingsYaml_LocalManifestFiles;
        case BoolAdminSetting::BypassCertificatePinningForMicrosoftStore:
            return s_AdminSettingsYaml_BypassCertificatePinningForMicrosoftStore;
        case BoolAdminSetting::InstallerHashOverride:
            return s_AdminSettingsYaml_InstallerHashOverride;
        case BoolAdminSetting::LocalArchiveMalwareScanOverride:
            return s_AdminSettingsYaml_LocalArchiveMalwareScanOverride;
        case BoolAdminSetting::ProxyCommandLineOptions:
            return s_AdminSettingsYaml_ProxyCommandLineOptions;
        default:
            return "Unknown"_liv;
        }
    }

    Utility::LocIndView AdminSettingToString(StringAdminSetting setting)
    {
        switch (setting)
        {
        case StringAdminSetting::DefaultProxy:
            return s_AdminSettingsYaml_DefaultProxy;
        default:
            return "Unknown"_liv;
        }
    }

    TogglePolicy::Policy GetAdminSettingPolicy(BoolAdminSetting setting)
    {
        switch (setting)
        {
        case BoolAdminSetting::LocalManifestFiles:
            return TogglePolicy::Policy::LocalManifestFiles;
        case BoolAdminSetting::BypassCertificatePinningForMicrosoftStore:
            return TogglePolicy::Policy::BypassCertificatePinningForMicrosoftStore;
        case BoolAdminSetting::InstallerHashOverride:
            return TogglePolicy::Policy::HashOverride;
        case BoolAdminSetting::LocalArchiveMalwareScanOverride:
            return TogglePolicy::Policy::LocalArchiveMalwareScanOverride;
        case BoolAdminSetting::ProxyCommandLineOptions:
            return TogglePolicy::Policy::ProxyCommandLineOptions;
        default:
            return TogglePolicy::Policy::None;
        }
    }

    bool EnableAdminSetting(BoolAdminSetting setting)
    {
        if (GetPolicyStateForSetting(setting) == PolicyState::Disabled)
        {
            return false;
        }

        AdminSettingsInternal adminSettingsInternal;
        adminSettingsInternal.SetAdminSetting(setting, true);
        return true;
    }

    bool DisableAdminSetting(BoolAdminSetting setting)
    {
        if (GetPolicyStateForSetting(setting) == PolicyState::Enabled)
        {
            return false;
        }

        AdminSettingsInternal adminSettingsInternal;
        adminSettingsInternal.SetAdminSetting(setting, false);
        return true;
    }

    bool IsAdminSettingEnabled(BoolAdminSetting setting)
    {
        // Check for a policy that overrides this setting.
        auto policyState = GetPolicyStateForSetting(setting);
        if (policyState != PolicyState::NotConfigured)
        {
            return policyState == PolicyState::Enabled;
        }

        AdminSettingsInternal adminSettingsInternal;
        return adminSettingsInternal.GetAdminSettingValue(setting);
    }

    bool SetAdminSetting(StringAdminSetting setting, std::string_view value)
    {
        if (GetPolicyStateForSetting(setting))
        {
            return false;
        }

        AdminSettingsInternal adminSettingsInternal;
        adminSettingsInternal.SetAdminSetting(setting, std::string{ value });
        return true;
    }

    bool ResetAdminSetting(StringAdminSetting setting)
    {
        if (GetPolicyStateForSetting(setting))
        {
            return false;
        }

        AdminSettingsInternal adminSettingsInternal;
        adminSettingsInternal.SetAdminSetting(setting, std::nullopt);
        return true;
    }

    std::optional<std::string> GetAdminSetting(StringAdminSetting setting)
    {
        // Check for a policy that overrides this setting.
        auto policyState = GetPolicyStateForSetting(setting);
        if (policyState)
        {
            return policyState.value();
        }

        AdminSettingsInternal adminSettingsInternal;
        return adminSettingsInternal.GetAdminSettingValue(setting);
    }

    std::vector<BoolAdminSetting> GetAllBoolAdminSettings()
    {
        return GetAllSequentialEnumValues(BoolAdminSetting::Unknown);
    }

    std::vector<StringAdminSetting> GetAllStringAdminSettings()
    {
        return GetAllSequentialEnumValues(StringAdminSetting::Unknown);
    }

}
