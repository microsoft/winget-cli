// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Runtime
{
    namespace
    {
        // Gets a boolean indicating whether the current process has identity.
        bool DoesCurrentProcessHaveIdentity()
        {
            UINT32 length = 0;
            LONG result = GetPackageFamilyName(GetCurrentProcess(), &length, nullptr);
            return (result != APPMODEL_ERROR_NO_PACKAGE);
        }

#ifndef AICLI_DISABLE_TEST_HOOKS
        static std::filesystem::path s_Settings_TestHook_ForcedContainerPrepend;
#endif

        void ValidateSettingNamePath(std::filesystem::path& name)
        {
            THROW_HR_IF(E_INVALIDARG, !name.has_relative_path());
            THROW_HR_IF(E_INVALIDARG, name.has_root_path());
            THROW_HR_IF(E_INVALIDARG, !name.has_filename());

#ifndef AICLI_DISABLE_TEST_HOOKS
            if (!s_Settings_TestHook_ForcedContainerPrepend.empty())
            {
                std::filesystem::path result = s_Settings_TestHook_ForcedContainerPrepend;
                result /= name;
                name = std::move(result);
            }
#endif
        }

        // Gets the container within LocalSettings for the given path.
        auto GetLocalSettingsContainerForPath(const std::filesystem::path& name)
        {
            auto result = winrt::Windows::Storage::ApplicationData::Current().LocalSettings();

            for (const auto& part : name.parent_path())
            {
                auto partHstring = winrt::to_hstring(part.c_str());
                result = result.CreateContainer(partHstring, winrt::Windows::Storage::ApplicationDataCreateDisposition::Always);
            }

            return result;
        }

        // Gets the path to the appdata root.
        // *Only used by non packaged version!*
        std::filesystem::path GetPathToAppDataRoot()
        {
            THROW_HR_IF(E_NOT_VALID_STATE, IsRunningInPackagedContext());

            DWORD charCount = ExpandEnvironmentStringsW(L"%LOCALAPPDATA%", nullptr, 0);
            THROW_LAST_ERROR_IF(charCount == 0);

            std::wstring localAppDataPath(charCount + 1, L'\0');
            charCount = ExpandEnvironmentStringsW(L"%LOCALAPPDATA%", &localAppDataPath[0], charCount + 1);
            THROW_LAST_ERROR_IF(charCount == 0);

            localAppDataPath.resize(charCount - 1);

            std::filesystem::path result = localAppDataPath;
            result /= "Microsoft/AppInstaller";
            return result;
        }

        // Gets the path to the settings root.
        // Creates the directory if it does not already exist.
        std::filesystem::path GetPathToSettingsRoot()
        {
            std::filesystem::path result = GetPathToAppDataRoot();
            result /= "Settings";

            if (std::filesystem::exists(result))
            {
                if (!std::filesystem::is_directory(result))
                {
                    // STATUS_NOT_A_DIRECTORY: A requested opened file is not a directory.
                    THROW_NTSTATUS_MSG(0xC0000103, "Settings is not a directory");
                }
            }
            else
            {
                std::filesystem::create_directories(result);
            }

            return result;
        }

        // Gets the path to the settings directory for the given setting.
        // Creates the directory if it does not already exist.
        std::filesystem::path GetPathToSettings(const std::filesystem::path& name)
        {
            std::filesystem::path result = GetPathToAppDataRoot();
            if (name.has_parent_path())
            {
                result /= name.parent_path();
                std::filesystem::create_directories(result);
            }
            return result;
        }
    }

    bool IsRunningInPackagedContext()
    {
        static bool result = DoesCurrentProcessHaveIdentity();
        return result;
    }

    std::string GetClientVersion()
    {
        using namespace std::string_literals;

        if (IsRunningInPackagedContext())
        {
            UINT32 bufferLength = 0;
            LONG gcpiResult = GetCurrentPackageId(&bufferLength, nullptr);
            THROW_HR_IF(E_UNEXPECTED, gcpiResult != ERROR_INSUFFICIENT_BUFFER);

            std::unique_ptr<byte[]> buffer = std::make_unique<byte[]>(bufferLength);

            gcpiResult = GetCurrentPackageId(&bufferLength, buffer.get());
            if (FAILED_WIN32_LOG(gcpiResult))
            {
                return "error"s;
            }

            PACKAGE_ID* packageId = reinterpret_cast<PACKAGE_ID*>(buffer.get());
            PACKAGE_VERSION& version = packageId->version;

            std::ostringstream strstr;
            strstr << version.Major << '.' << version.Minor << '.' << version.Build << '.' << version.Revision;

            return strstr.str();
        }
        else
        {
            return "unknown"s;
        }
    }

    std::filesystem::path GetPathToTemp()
    {
        if (IsRunningInPackagedContext())
        {
            return { winrt::Windows::Storage::ApplicationData::Current().TemporaryFolder().Path().c_str() };
        }
        else
        {
            wchar_t tempPath[MAX_PATH + 1];
            DWORD tempChars = GetTempPathW(ARRAYSIZE(tempPath), tempPath);
            return { std::wstring_view{ tempPath, static_cast<size_t>(tempChars) } };
        }
    }

    std::unique_ptr<std::istream> GetSettingStream(std::filesystem::path name)
    {
        ValidateSettingNamePath(name);

        if (IsRunningInPackagedContext())
        {
            auto container = GetLocalSettingsContainerForPath(name);
            auto filenameHstring = winrt::to_hstring(name.filename().c_str());
            auto settingsValues = container.Values();
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
        else
        {
            auto settingFileName = GetPathToSettings(name);
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
    }

    void SetSetting(std::filesystem::path name, std::string_view value)
    {
        ValidateSettingNamePath(name);

        if (IsRunningInPackagedContext())
        {
            GetLocalSettingsContainerForPath(name).Values().
                Insert(winrt::to_hstring(name.filename().c_str()), winrt::box_value(winrt::to_hstring(value)));
        }
        else
        {
            auto settingFileName = GetPathToSettings(name);
            settingFileName /= name.filename();

            std::ofstream stream(settingFileName, std::ios_base::out | std::ios_base::binary | std::ios_base::trunc);
            stream << value << std::flush;
        }
    }

    void RemoveSetting(std::filesystem::path name)
    {
        ValidateSettingNamePath(name);

        if (IsRunningInPackagedContext())
        {
            GetLocalSettingsContainerForPath(name).Values().Remove(winrt::to_hstring(name.filename().c_str()));
        }
        else
        {
            auto settingFileName = GetPathToSettings(name);
            settingFileName /= name.filename();

            std::filesystem::remove(settingFileName);
        }
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void TestHook_ForceContainerPrepend(const std::filesystem::path& prepend)
    {
        s_Settings_TestHook_ForcedContainerPrepend = prepend;
    }
#endif
}
