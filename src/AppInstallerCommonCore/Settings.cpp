// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Settings.h"
#include "Public/AppInstallerDownloader.h"
#include "Public/AppInstallerErrors.h"
#include "Public/AppInstallerMsixInfo.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerSHA256.h"
#include "Public/winget/Yaml.h"

namespace AppInstaller::Settings
{
    using namespace std::string_view_literals;
    using namespace Runtime;
    using namespace Utility;

    namespace
    {
        void ValidateSettingNamePath(const std::filesystem::path& name)
        {
            THROW_HR_IF(E_INVALIDARG, !name.has_relative_path());
            THROW_HR_IF(E_INVALIDARG, name.has_root_path());
            THROW_HR_IF(E_INVALIDARG, !name.has_filename());
        }

        void LogSettingAction(std::string_view action, const StreamDefinition& def)
        {
            AICLI_LOG(Core, Verbose, << "Setting action: " << action << ", Type: " << ToString(def.Type) << ", Name: " << def.Name);
        }

#ifndef WINGET_DISABLE_FOR_FUZZING
        // A settings container backed by the ApplicationDataContainer functionality.
        struct ApplicationDataSettingsContainer : public details::ISettingsContainer
        {
            using Container = winrt::Windows::Storage::ApplicationDataContainer;

            ApplicationDataSettingsContainer(const Container& container, const std::filesystem::path& name)
            {
                m_parentContainer = GetRelativeContainer(container, name.parent_path());
                m_settingName = winrt::to_hstring(name.filename().c_str());
            }

            static Container GetRelativeContainer(const Container& container, const std::filesystem::path& offset)
            {
                auto result = container;

                for (const auto& part : offset)
                {
                    auto partHstring = winrt::to_hstring(part.c_str());
                    result = result.CreateContainer(partHstring, winrt::Windows::Storage::ApplicationDataCreateDisposition::Always);
                }

                return result;
            }

            std::unique_ptr<std::istream> Get() override
            {
                auto settingsValues = m_parentContainer.Values();
                if (settingsValues.HasKey(m_settingName))
                {
                    auto value = winrt::unbox_value<winrt::hstring>(settingsValues.Lookup(m_settingName));
                    return std::make_unique<std::istringstream>(Utility::ConvertToUTF8(value.c_str()));
                }
                else
                {
                    return {};
                }
            }

            bool Set(std::string_view value) override
            {
                m_parentContainer.Values().Insert(m_settingName, winrt::box_value(winrt::to_hstring(value)));
                return true;
            }

            void Remove() override
            {
                m_parentContainer.Values().Remove(m_settingName);
            }

            std::filesystem::path PathTo() override
            {
                THROW_HR(E_UNEXPECTED);
            }

        private:
            Container m_parentContainer = nullptr;
            winrt::hstring m_settingName;
        };
#endif

        // A settings container backed by the filesystem.
        struct FileSettingsContainer : public details::ISettingsContainer
        {
            FileSettingsContainer(std::filesystem::path root, const std::filesystem::path& name) : m_settingFile(std::move(root))
            {
                m_settingFile /= name;
            }

            std::unique_ptr<std::istream> Get() override
            {
                if (std::filesystem::exists(m_settingFile))
                {
                    auto result = std::make_unique<std::ifstream>(m_settingFile);
                    THROW_LAST_ERROR_IF(result->fail());
                    return result;
                }
                else
                {
                    return {};
                }
            }

            bool Set(std::string_view value) override
            {
                EnsureParentPath();

                std::ofstream stream(m_settingFile, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
                THROW_LAST_ERROR_IF(stream.fail());
                stream << value << std::flush;
                THROW_LAST_ERROR_IF(stream.fail());

                return true;
            }

            void Remove() override
            {
                std::filesystem::remove(m_settingFile);
            }

            std::filesystem::path PathTo() override
            {
                return m_settingFile;
            }

        private:
            void EnsureParentPath()
            {
                std::filesystem::create_directories(m_settingFile.parent_path());
            }

            std::filesystem::path m_settingFile;
        };

        // A settings container that manages safely writing to its value with exchange semantics.
        // Only allows Set to succeed if the hash value of the setting is the same as the last time it was read.
        struct ExchangeSettingsContainer : public details::ISettingsContainer
        {
            ExchangeSettingsContainer(std::unique_ptr<ISettingsContainer>&& container, std::string_view name) :
                m_container(std::move(container)), m_name(name) {}

            std::unique_ptr<std::istream> Get() override
            {
                return GetInternal(m_hash);
            }

            bool Set(std::string_view value) override
            {
                THROW_HR_IF(E_UNEXPECTED, value.size() > std::numeric_limits<uint32_t>::max());

                // If Set is called without ever reading the value, then we can assume that caller wants
                // to overwrite it regardless. Also, we don't have any previous value to compare against
                // anyway so the only other option would be to always reject it.
                if (m_hash)
                {
                    std::optional<SHA256::HashBuffer> currentHash;
                    std::ignore = GetInternal(currentHash);

                    if (currentHash && !SHA256::AreEqual(m_hash.value(), currentHash.value()))
                    {
                        AICLI_LOG(Core, Verbose, << "Setting value for '" << m_name << "' has changed since last read; rejecting Set");
                        return false;
                    }
                }

                SHA256::HashBuffer newHash = SHA256::ComputeHash(reinterpret_cast<const uint8_t*>(value.data()), static_cast<uint32_t>(value.size()));
                if (m_container->Set(value))
                {
                    m_hash = std::move(newHash);
                    return true;
                }
                else
                {
                    return false;
                }
            }

            void Remove() override
            {
                m_container->Remove();
                m_hash.reset();
            }

            std::filesystem::path PathTo() override
            {
                return m_container->PathTo();
            }

        protected:
            std::string_view m_name;
            std::optional<SHA256::HashBuffer> m_hash;

        private:
            std::unique_ptr<std::istream> GetInternal(std::optional<SHA256::HashBuffer>& hashStorage)
            {
                std::unique_ptr<std::istream> stream = m_container->Get();

                if (!stream)
                {
                    // If no stream exists, then no hashing needs to be done.
                    // Return an empty hash vector to indicate the attempted read but no result.
                    hashStorage.emplace();
                    return stream;
                }

                std::string streamContents = Utility::ReadEntireStream(*stream);
                THROW_HR_IF(E_UNEXPECTED, streamContents.size() > std::numeric_limits<uint32_t>::max());

                hashStorage = SHA256::ComputeHash(reinterpret_cast<const uint8_t*>(streamContents.c_str()), static_cast<uint32_t>(streamContents.size()));

                // Return a stream over the contents that we read in and hashed, to prevent a race.
                return std::make_unique<std::istringstream>(streamContents);
            }

            std::unique_ptr<ISettingsContainer> m_container;
        };

        // A settings container wrapper that enforces security.
        struct SecureSettingsContainer : public ExchangeSettingsContainer
        {
            constexpr static std::string_view NodeName_Sha256 = "SHA256"sv;

            SecureSettingsContainer(std::unique_ptr<ISettingsContainer>&& container, std::string_view name) :
                ExchangeSettingsContainer(std::move(container), name), m_secure(GetPathTo(PathName::SecureSettingsForRead), name) {}

        private:
            struct VerificationData
            {
                bool Found = false;
                SHA256::HashBuffer Hash;
            };

            VerificationData GetVerificationData()
            {
                std::unique_ptr<std::istream> stream = m_secure.Get();

                if (!stream)
                {
                    return {};
                }

                std::string streamContents = Utility::ReadEntireStream(*stream);

                YAML::Node document;
                try
                {
                    document = YAML::Load(streamContents);
                }
                catch (const std::runtime_error& e)
                {
                    AICLI_LOG(Core, Error, << "Secure setting metadata for '" << m_name << "' contained invalid YAML (" << e.what() << "):\n" << streamContents);
                    return {};
                }

                std::string hashString;

                try
                {
                    hashString = document[NodeName_Sha256].as<std::string>();
                }
                catch (const std::runtime_error& e)
                {
                    AICLI_LOG(Core, Error, << "Secure setting metadata for '" << m_name << "' contained invalid YAML (" << e.what() << "):\n" << streamContents);
                    return {};
                }

                VerificationData result;
                result.Found = true;
                result.Hash = SHA256::ConvertToBytes(hashString);

                return result;
            }

            void SetVerificationData(VerificationData data)
            {
                YAML::Emitter out;
                out << YAML::BeginMap;
                out << YAML::Key << NodeName_Sha256 << YAML::Value << SHA256::ConvertToString(data.Hash);
                out << YAML::EndMap;

                m_secure.Set(out.str());
            }

        public:
            std::unique_ptr<std::istream> Get() override
            {
                std::unique_ptr<std::istream> stream = ExchangeSettingsContainer::Get();

                if (!stream)
                {
                    // If no stream exists, then no verification needs to be done.
                    return stream;
                }

                VerificationData verData = GetVerificationData();

                // This case should be very rare, so a very identifiable error is helpful.
                // Plus the text for this one is fairly on point for what has happened.
                THROW_HR_IF(SPAPI_E_FILE_HASH_NOT_IN_CATALOG, !verData.Found);

                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR), !SHA256::AreEqual(m_hash.value(), verData.Hash));

                // ExchangeSettingsContainer already produces an in memory stream that we can use.
                return stream;
            }

            bool Set(std::string_view value) override
            {
                // Force the creation of the secure settings location with appropriate ACLs
                GetPathTo(PathName::SecureSettingsForWrite);

                bool exchangeResult = ExchangeSettingsContainer::Set(value);

                if (exchangeResult)
                {
                    VerificationData verData;
                    verData.Hash = m_hash.value();

                    SetVerificationData(verData);
                }

                return exchangeResult;
            }

            void Remove() override
            {
                ExchangeSettingsContainer::Remove();
                m_secure.Remove();
            }

            std::filesystem::path PathTo() override
            {
                THROW_HR(E_UNEXPECTED);
            }

        private:
            FileSettingsContainer m_secure;
        };

        // A settings container wrapper that reads remote settings and verifies trust before loading.
        struct TrustedRemoteSettingsContainer : public details::ISettingsContainer
        {
            // TODO: Values may need to be updated after server side work is done.
            constexpr static std::string_view s_RemoteSettings_Url = "https://cdn.winget.microsoft.com/settings/settings.msix"sv;
            constexpr static std::string_view s_RemoteSettings_PackageFamilyName = "Microsoft.Winget.Settings_8wekyb3d8bbwe"sv;

            TrustedRemoteSettingsContainer(std::string_view name)
            {
                m_settingsFile = GetPathTo(PathName::RemoteSettings) / name;

                if (!TryLockCachedSettingsFile())
                {
                    AICLI_LOG(Core, Info, << "Failed to open cached remote settings file, try downloading new.");
                    m_fileLock.reset();

                    std::filesystem::path tempSettingsPath = GetPathTo(PathName::Temp) / (std::string{ name } + ".dnld.msix");
                    ProgressCallback progress;
                    AppInstaller::Utility::Download(std::string{ s_RemoteSettings_Url }, tempSettingsPath, AppInstaller::Utility::DownloadType::RemoteSettings, progress);

                    bool tempSettingsTrusted = false;
                    {
                        // Extra scope to release the file lock right after trust validation.
                        std::unique_ptr<Msix::WriteLockedMsixFile> tempFileLock;
                        tempSettingsTrusted = LockSettingsFile(tempSettingsPath, tempFileLock);
                    }

                    if (!tempSettingsTrusted)
                    {
                        AICLI_LOG(Core, Error, << "Failed to open remote settings, downloaded temp remote settings not trusted.");
                        THROW_HR(APPINSTALLER_CLI_ERROR_REMOTE_SETTINGS_NOT_TRUSTED);
                    }

                    std::filesystem::rename(tempSettingsPath, m_settingsFile);
                    if (!LockSettingsFile(m_settingsFile, m_fileLock))
                    {
                        AICLI_LOG(Core, Error, << "Failed to open remote settings, downloaded remote settings not trusted.");
                        THROW_HR(APPINSTALLER_CLI_ERROR_REMOTE_SETTINGS_NOT_TRUSTED);
                    }
                }

                AICLI_LOG(Repo, Info, << "Opened trusted remote settings.");
            }

            std::unique_ptr<std::istream> Get() override
            {
                if (std::filesystem::exists(m_settingsFile))
                {
                    auto result = std::make_unique<std::ifstream>(m_settingsFile);
                    THROW_LAST_ERROR_IF(result->fail());
                    return result;
                }
                else
                {
                    return {};
                }
            }

            bool Set(std::string_view) override
            {
                THROW_HR(E_UNEXPECTED);
            }

            void Remove() override
            {
                THROW_HR(E_UNEXPECTED);
            }

            std::filesystem::path PathTo() override
            {
                THROW_HR(E_UNEXPECTED);
            }

        private:
            // Try to use cached copy of the settings file if applicable.
            // Returns true if the settings file is trusted, up to date and successfully locked. Otherwise false.
            bool TryLockCachedSettingsFile()
            {
                if (!std::filesystem::exists(m_settingsFile))
                {
                    AICLI_LOG(Core, Info, << "Cached remote settings file does not exist.");
                    return false;
                }

                if (!LockSettingsFile(m_settingsFile, m_fileLock))
                {
                    return false;
                }

                Msix::MsixInfo remoteInfo{ s_RemoteSettings_Url };
                if (remoteInfo.IsNewerThan(m_settingsFile))
                {
                    AICLI_LOG(Core, Info, << "Cached remote settings file is not up to date.");
                    return false;
                }

                return true;
            }

            bool LockSettingsFile(const std::filesystem::path& settingsFile, std::unique_ptr<Msix::WriteLockedMsixFile>& lock)
            {
                lock = std::make_unique<Msix::WriteLockedMsixFile>(settingsFile);
                if (!lock->ValidateTrustInfo(true))
                {
                    AICLI_LOG(Core, Error, << "Cached remote settings file failed trust validation.");
                    return false;
                }

                Msix::MsixInfo packageInfo{ settingsFile };
                if (s_RemoteSettings_PackageFamilyName != Msix::GetPackageFamilyNameFromFullName(packageInfo.GetPackageFullName()))
                {
                    AICLI_LOG(Core, Error, << "Cached remote settings file has unexpected Package Family Name.");
                    return false;
                }

                return true;
            }

            std::filesystem::path m_settingsFile;
            std::unique_ptr<Msix::WriteLockedMsixFile> m_fileLock;
        };

        std::unique_ptr<details::ISettingsContainer> GetRawSettingsContainer(Type type, std::string_view name)
        {
#ifndef WINGET_DISABLE_FOR_FUZZING
            if (IsRunningInPackagedContext())
            {
                switch (type)
                {
                case Type::Standard:
                    return std::make_unique<ApplicationDataSettingsContainer>(
                        ApplicationDataSettingsContainer::GetRelativeContainer(
                            winrt::Windows::Storage::ApplicationData::Current().LocalSettings(), GetPathTo(PathName::StandardSettings)),
                        name);
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }
            else
#endif
            {
                switch (type)
                {
                case Type::Standard:
                    return std::make_unique<FileSettingsContainer>(GetPathTo(PathName::StandardSettings), name);
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }
        }

        // The default is not a raw container, so we wrap some of the underlying containers to enable higher order behaviors.
        std::unique_ptr<details::ISettingsContainer> GetSettingsContainer(Type type, std::string_view name)
        {
            switch (type)
            {
            case Type::Standard:
                // Standard settings should use exchange semantics to prevent overwrites
                return std::make_unique<ExchangeSettingsContainer>(GetRawSettingsContainer(type, name), name);

            case Type::UserFile:
                // User file settings are not typically modified by us, so there is no need for exchange
                return std::make_unique<FileSettingsContainer>(GetPathTo(PathName::UserFileSettings), name);

            case Type::Secure:
                // Secure settings add hash verification on reads on top of exchange semantics
                return std::make_unique<SecureSettingsContainer>(GetRawSettingsContainer(Type::Standard, name), name);

            case Type::TrustedRemote:
                // Remote settings is read only, hosted on winget cdn and trust verified before loading
                return std::make_unique<TrustedRemoteSettingsContainer>(name);

            default:
                THROW_HR(E_UNEXPECTED);
            }
        }

        std::unique_ptr<details::ISettingsContainer> GetSettingsContainer(const StreamDefinition& streamDefinition)
        {
            return GetSettingsContainer(streamDefinition.Type, streamDefinition.Name);
        }
    }

    std::string_view ToString(Type type)
    {
        switch (type)
        {
        case Type::Standard:
            return "Standard"sv;
        case Type::UserFile:
            return "UserFile"sv;
        case Type::Secure:
            return "Secure"sv;
        default:
            THROW_HR(E_UNEXPECTED);
        }
    }

    Stream::Stream(const StreamDefinition& streamDefinition) :
        m_streamDefinition(streamDefinition), m_container(GetSettingsContainer(streamDefinition))
    {
        ValidateSettingNamePath(m_streamDefinition.Name);
    }

    std::unique_ptr<std::istream> Stream::Get()
    {
        LogSettingAction("Get", m_streamDefinition);
        return m_container->Get();
    }

    [[nodiscard]] bool Stream::Set(std::string_view value)
    {
        LogSettingAction("Set", m_streamDefinition);
        return m_container->Set(value);
    }

    void Stream::Remove()
    {
        LogSettingAction("Remove", m_streamDefinition);
        m_container->Remove();
    }

    std::string_view Stream::GetName() const
    {
        return m_streamDefinition.Name;
    }

    std::filesystem::path Stream::GetPath() const
    {
        return m_container->PathTo();
    }
}
