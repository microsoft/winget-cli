// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Settings.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Settings
{
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
        };

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

        private:
            Container m_root;
        };

        // A settings container backed by the filesystem.
        struct FileSettingsContainer : public ISettingsContainer
        {
            using Container = winrt::Windows::Storage::ApplicationDataContainer;

            FileSettingsContainer(std::filesystem::path root) : m_root(std::move(root)) {}

            std::filesystem::path GetRelativePath(const std::filesystem::path& name)
            {
                std::filesystem::path result = m_root;
                if (name.has_parent_path())
                {
                    result /= name.parent_path();
                    std::filesystem::create_directories(result);
                }
                return result;
            }

            std::unique_ptr<std::istream> Get(const std::filesystem::path& name) override
            {
                std::filesystem::path settingFileName = GetRelativePath(name);
                settingFileName /= name.filename();

                if (std::filesystem::exists(settingFileName))
                {
                    return std::make_unique<std::ifstream>(settingFileName);
                }
                else
                {
                    return {};
                }
            }

            void Set(const std::filesystem::path& name, std::string_view value) override
            {
                std::filesystem::path settingFileName = GetRelativePath(name);
                settingFileName /= name.filename();

                std::ofstream stream(settingFileName, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
                stream << value << std::flush;
            }

            void Remove(const std::filesystem::path& name) override
            {
                std::filesystem::path settingFileName = GetRelativePath(name);
                settingFileName /= name.filename();

                std::filesystem::remove(settingFileName);
            }

        private:
            std::filesystem::path m_root;
        };

        std::unique_ptr<ISettingsContainer> GetSettingsContainer(Type type)
        {
            if (IsRunningInPackagedContext())
            {
                switch (type)
                {
                case AppInstaller::Settings::Type::Standard:
                    return std::make_unique<ApplicationDataSettingsContainer>(
                        ApplicationDataSettingsContainer::GetRelativeContainer(
                            winrt::Windows::Storage::ApplicationData::Current().LocalSettings(), GetPathTo(PathName::StandardSettings)));
                case AppInstaller::Settings::Type::UserFile:
                    return std::make_unique<FileSettingsContainer>(GetPathTo(PathName::UserFileSettings));
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }
            else
            {
                switch (type)
                {
                case AppInstaller::Settings::Type::Standard:
                    return std::make_unique<FileSettingsContainer>(GetPathTo(PathName::StandardSettings));
                case AppInstaller::Settings::Type::UserFile:
                    return std::make_unique<FileSettingsContainer>(GetPathTo(PathName::UserFileSettings));
                default:
                    THROW_HR(E_UNEXPECTED);
                }
            }
        }
    }

    std::unique_ptr<std::istream> GetSettingStream(Type type, const std::filesystem::path& name)
    {
        ValidateSettingNamePath(name);
        return GetSettingsContainer(type)->Get(name);
    }

    void SetSetting(Type type, const std::filesystem::path& name, std::string_view value)
    {
        ValidateSettingNamePath(name);
        GetSettingsContainer(type)->Set(name, value);
    }

    void RemoveSetting(Type type, const std::filesystem::path& name)
    {
        ValidateSettingNamePath(name);
        GetSettingsContainer(type)->Remove(name);
    }
}
