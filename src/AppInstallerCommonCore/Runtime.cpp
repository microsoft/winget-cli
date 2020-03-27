// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <binver/version.h>
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Runtime
{
    namespace
    {
        using namespace std::string_view_literals;
        constexpr std::string_view s_AppDataDir_Settings = "Settings";
        constexpr std::string_view s_AppDataDir_State = "State";

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
        }

        // Gets the container within LocalSettings for the given path.
        auto GetLocalSettingsContainerForPath(const std::filesystem::path& name)
        {
            auto result = winrt::Windows::Storage::ApplicationData::Current().LocalSettings();

            std::filesystem::path pathToUse;

#ifndef AICLI_DISABLE_TEST_HOOKS
            if (!s_Settings_TestHook_ForcedContainerPrepend.empty())
            {
                pathToUse = s_Settings_TestHook_ForcedContainerPrepend;
                pathToUse /= name;
            }
            else
#endif
            {
                pathToUse = name;
            }

            for (const auto& part : pathToUse.parent_path())
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

#ifndef AICLI_DISABLE_TEST_HOOKS
            if (!s_Settings_TestHook_ForcedContainerPrepend.empty())
            {
                result /= s_Settings_TestHook_ForcedContainerPrepend;
            }
#endif

            return result;
        }

        // Gets the path to the app data relative directory.
        // Creates the directory if it does not already exist.
        std::filesystem::path GetPathToAppDataDir(const std::filesystem::path& relative)
        {
            THROW_HR_IF(E_INVALIDARG, !relative.has_relative_path());
            THROW_HR_IF(E_INVALIDARG, relative.has_root_path());
            THROW_HR_IF(E_INVALIDARG, !relative.has_filename());

            std::filesystem::path result = GetPathToAppDataRoot();
            result /= relative;

            if (std::filesystem::exists(result))
            {
                if (!std::filesystem::is_directory(result))
                {
                    // STATUS_NOT_A_DIRECTORY: A requested opened file is not a directory.
                    THROW_NTSTATUS_MSG(0xC0000103, "AppData location is not a directory");
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
            std::filesystem::path result = GetPathToAppDataDir(s_AppDataDir_Settings);
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
            return VER_FILE_VERSION_STR;
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

    std::filesystem::path GetPathToLocalState()
    {
        if (IsRunningInPackagedContext())
        {
            std::filesystem::path result = winrt::Windows::Storage::ApplicationData::Current().LocalFolder().Path().c_str();

#ifndef AICLI_DISABLE_TEST_HOOKS
            if (!s_Settings_TestHook_ForcedContainerPrepend.empty())
            {
                result /= s_Settings_TestHook_ForcedContainerPrepend;
            }
#endif

            return result;
        }
        else
        {
            return GetPathToAppDataDir(s_AppDataDir_State);
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

    bool IsCurrentOSVersionGreaterThanOrEqual(const Utility::Version& version)
    {
        DWORD versionParts[3] = {};

        for (size_t i = 0; i < ARRAYSIZE(versionParts) && i < version.GetParts().size(); ++i)
        {
            versionParts[i] = static_cast<DWORD>(std::min(static_cast<decltype(version.GetParts()[i].Integer)>(std::numeric_limits<DWORD>::max()), version.GetParts()[i].Integer));
        }

        OSVERSIONINFOEXW osVersionInfo{};
        osVersionInfo.dwOSVersionInfoSize = sizeof(osVersionInfo);
        osVersionInfo.dwMajorVersion = versionParts[0];
        osVersionInfo.dwMinorVersion = versionParts[1];
        osVersionInfo.dwBuildNumber = versionParts[2];
        osVersionInfo.wServicePackMajor = 0;
        osVersionInfo.wServicePackMinor = 0;

        DWORD mask = VER_MAJORVERSION | VER_MINORVERSION | VER_BUILDNUMBER | VER_SERVICEPACKMAJOR | VER_SERVICEPACKMINOR;

        DWORDLONG conditions = 0;
        VER_SET_CONDITION(conditions, VER_MAJORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditions, VER_MINORVERSION, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditions, VER_BUILDNUMBER, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditions, VER_SERVICEPACKMAJOR, VER_GREATER_EQUAL);
        VER_SET_CONDITION(conditions, VER_SERVICEPACKMINOR, VER_GREATER_EQUAL);

        BOOL result = VerifyVersionInfoW(&osVersionInfo, mask, conditions);
        if (!result)
        {
            THROW_LAST_ERROR_IF(GetLastError() != ERROR_OLD_WIN_VERSION);
        }
        return !!result;
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void TestHook_ForceContainerPrepend(const std::filesystem::path& prepend)
    {
        s_Settings_TestHook_ForcedContainerPrepend = prepend;
    }
#endif
}
