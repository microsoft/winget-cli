// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Settings.h"
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
            AICLI_LOG(Core, Info, << "Setting action: " << action << ", Type: " << ToString(def.Type) << ", Name: " << def.Path);
        }

        // A settings container.
        struct ISettingsContainer
        {
            virtual ~ISettingsContainer() = default;

            // Gets a stream containing the named setting's value, if present.
            // If the setting does not exist, returns an empty value.
            virtual std::unique_ptr<std::istream> Get(const std::filesystem::path& name) = 0;

            // Sets the named setting to the given value.
            virtual void Set(const std::filesystem::path& name, std::string_view value) = 0;

            // Deletes the given setting.
            virtual void Remove(const std::filesystem::path& name) = 0;

            // Gets the path to the named setting, if reasonable.
            virtual std::filesystem::path PathTo(const std::filesystem::path& name) = 0;
        };

#ifndef WINGET_DISABLE_FOR_FUZZING
        // A settings container backed by the ApplicationDataContainer functionality.
        struct ApplicationDataSettingsContainer : public ISettingsContainer
        {
            using Container = winrt::Windows::Storage::ApplicationDataContainer;

            ApplicationDataSettingsContainer(Container&& container) : m_root(std::move(container)) {}

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

            std::unique_ptr<std::istream> Get(const std::filesystem::path& name) override
            {
                Container parent = GetRelativeContainer(m_root, name.parent_path());

                auto filenameHstring = winrt::to_hstring(name.filename().c_str());
                auto settingsValues = parent.Values();
                if (settingsValues.HasKey(filenameHstring))
                {
                    auto value = winrt::unbox_value<winrt::hstring>(settingsValues.Lookup(filenameHstring));
                    return std::make_unique<std::istringstream>(Utility::ConvertToUTF8(value.c_str()));
                }
                else
                {
                    return {};
                }
            }

            void Set(const std::filesystem::path& name, std::string_view value) override
            {
                Container parent = GetRelativeContainer(m_root, name.parent_path());
                parent.Values().Insert(winrt::to_hstring(name.filename().c_str()), winrt::box_value(winrt::to_hstring(value)));
            }

            void Remove(const std::filesystem::path& name) override
            {
                Container parent = GetRelativeContainer(m_root, name.parent_path());
                parent.Values().Remove(winrt::to_hstring(name.filename().c_str()));
            }

            std::filesystem::path PathTo(const std::filesystem::path&) override
            {
                THROW_HR(E_UNEXPECTED);
            }

        private:
            Container m_root;
        };
#endif

        // A settings container backed by the filesystem.
        struct FileSettingsContainer : public ISettingsContainer
        {
            FileSettingsContainer(std::filesystem::path root) : m_root(std::move(root)) {}

            std::unique_ptr<std::istream> Get(const std::filesystem::path& name) override
            {
                std::filesystem::path settingFileName = GetPath(name);

                if (std::filesystem::exists(settingFileName))
                {
                    auto result = std::make_unique<std::ifstream>(settingFileName);
                    THROW_LAST_ERROR_IF(result->fail());
                    return result;
                }
                else
                {
                    return {};
                }
            }

            void Set(const std::filesystem::path& name, std::string_view value) override
            {
                std::filesystem::path settingFileName = GetPath(name, true);

                std::ofstream stream(settingFileName, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
                THROW_LAST_ERROR_IF(stream.fail());
                stream << value << std::flush;
                THROW_LAST_ERROR_IF(stream.fail());
            }

            void Remove(const std::filesystem::path& name) override
            {
                std::filesystem::path settingFileName = GetPath(name);

                std::filesystem::remove(settingFileName);
            }

            std::filesystem::path PathTo(const std::filesystem::path& name) override
            {
                return GetPath(name);
            }

        private:
            std::filesystem::path GetPath(const std::filesystem::path& name, bool createParent = false)
            {
                std::filesystem::path result = m_root;

                if (name.has_parent_path())
                {
                    result /= name.parent_path();
                }

                if (createParent)
                {
                    std::filesystem::create_directories(result);
                }

                result /= name.filename();
                return result;
            }

            std::filesystem::path m_root;
        };

        // A settings container wrapper that enforces security.
        struct SecureSettingsContainer : public ISettingsContainer
        {
            constexpr static std::string_view NodeName_Sha256 = "SHA256"sv;

            SecureSettingsContainer(std::unique_ptr<ISettingsContainer>&& container) : m_container(std::move(container)), m_secure(GetPathTo(PathName::SecureSettings)) {}

            struct VerificationData
            {
                bool Found = false;
                SHA256::HashBuffer Hash;
            };

            VerificationData GetVerificationData(const std::filesystem::path& name)
            {
                std::unique_ptr<std::istream> stream = m_secure.Get(name);

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
                    AICLI_LOG(YAML, Error, << "Secure setting metadata for '" << name << "' contained invalid YAML (" << e.what() << "):\n" << streamContents);
                    return {};
                }

                std::string hashString;

                try
                {
                    hashString = document[NodeName_Sha256].as<std::string>();
                }
                catch (const std::runtime_error& e)
                {
                    AICLI_LOG(YAML, Error, << "Secure setting metadata for '" << name << "' contained invalid YAML (" << e.what() << "):\n" << streamContents);
                    return {};
                }

                VerificationData result;
                result.Found = true;
                result.Hash = SHA256::ConvertToBytes(hashString);

                return result;
            }

            void SetVerificationData(const std::filesystem::path& name, VerificationData data)
            {
                YAML::Emitter out;
                out << YAML::BeginMap;
                out << YAML::Key << NodeName_Sha256 << YAML::Value << SHA256::ConvertToString(data.Hash);
                out << YAML::EndMap;

                m_secure.Set(name, out.str());
            }

            std::unique_ptr<std::istream> Get(const std::filesystem::path& name) override
            {
                std::unique_ptr<std::istream> stream = m_container->Get(name);

                if (!stream)
                {
                    // If no stream exists, then no verification needs to be done.
                    return stream;
                }

                VerificationData verData = GetVerificationData(name);

                // This case should be very rare, so a very identifiable error is helpful.
                // Plus the text for this one is fairly on point for what has happened.
                THROW_HR_IF(SPAPI_E_FILE_HASH_NOT_IN_CATALOG, !verData.Found);

                std::string streamContents = Utility::ReadEntireStream(*stream);
                THROW_HR_IF(E_UNEXPECTED, streamContents.size() > std::numeric_limits<uint32_t>::max());

                auto streamHash = SHA256::ComputeHash(reinterpret_cast<const uint8_t*>(streamContents.c_str()), static_cast<uint32_t>(streamContents.size()));

                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_DATA_CHECKSUM_ERROR), !std::equal(streamHash.begin(), streamHash.end(), verData.Hash.begin()));

                // Return a stream over the contents that we read in and verified, to prevent a race attack.
                return std::make_unique<std::istringstream>(streamContents);
            }

            void Set(const std::filesystem::path& name, std::string_view value) override
            {
                THROW_HR_IF(E_UNEXPECTED, value.size() > std::numeric_limits<uint32_t>::max());

                VerificationData verData;
                verData.Hash = SHA256::ComputeHash(reinterpret_cast<const uint8_t*>(value.data()), static_cast<uint32_t>(value.size()));

                SetVerificationData(name, verData);

                m_container->Set(name, value);
            }

            void Remove(const std::filesystem::path& name) override
            {
                m_secure.Remove(name);
                m_container->Remove(name);
            }

            std::filesystem::path PathTo(const std::filesystem::path&) override
            {
                THROW_HR(E_UNEXPECTED);
            }

        private:
            std::unique_ptr<ISettingsContainer> m_container;
            FileSettingsContainer m_secure;
        };

        std::unique_ptr<ISettingsContainer> GetSettingsContainer(Type type)
        {
            if (type == Type::Secure)
            {
                return std::make_unique<SecureSettingsContainer>(GetSettingsContainer(Type::Standard));
            }

#ifndef WINGET_DISABLE_FOR_FUZZING
            if (IsRunningInPackagedContext())
            {
                switch (type)
                {
                case Type::Standard:
                    return std::make_unique<ApplicationDataSettingsContainer>(
                        ApplicationDataSettingsContainer::GetRelativeContainer(
                            winrt::Windows::Storage::ApplicationData::Current().LocalSettings(), GetPathTo(PathName::StandardSettings)));
                case Type::UserFile:
                    return std::make_unique<FileSettingsContainer>(GetPathTo(PathName::UserFileSettings));
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
                    return std::make_unique<FileSettingsContainer>(GetPathTo(PathName::StandardSettings));
                case Type::UserFile:
                    return std::make_unique<FileSettingsContainer>(GetPathTo(PathName::UserFileSettings));
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }
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

    std::unique_ptr<std::istream> GetSettingStream(const StreamDefinition& def)
    {
        LogSettingAction("Get", def);
        ValidateSettingNamePath(def.Path);
        return GetSettingsContainer(def.Type)->Get(def.Path);
    }

    void SetSetting(const StreamDefinition& def, std::string_view value)
    {
        LogSettingAction("Set", def);
        ValidateSettingNamePath(def.Path);
        GetSettingsContainer(def.Type)->Set(def.Path, value);
    }

    void RemoveSetting(const StreamDefinition& def)
    {
        LogSettingAction("Remove", def);
        ValidateSettingNamePath(def.Path);
        GetSettingsContainer(def.Type)->Remove(def.Path);
    }

    std::filesystem::path GetPathTo(const StreamDefinition& def)
    {
        return GetSettingsContainer(def.Type)->PathTo(def.Path);
    }
}
