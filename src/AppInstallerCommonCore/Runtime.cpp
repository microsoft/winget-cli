// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <binver/version.h>
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

#define AICLI_DEFAULT_TEMP_DIRECTORY "WinGet"
#define WINGET_DEFAULT_LOG_DIRECTORY "DiagOutputDir"

namespace AppInstaller::Runtime
{
    using namespace Utility;

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

        std::unique_ptr<byte[]> GetPACKAGE_ID()
        {
            UINT32 bufferLength = 0;
            LONG gcpiResult = GetCurrentPackageId(&bufferLength, nullptr);
            THROW_HR_IF(E_UNEXPECTED, gcpiResult != ERROR_INSUFFICIENT_BUFFER);

            std::unique_ptr<byte[]> buffer = std::make_unique<byte[]>(bufferLength);

            gcpiResult = GetCurrentPackageId(&bufferLength, buffer.get());
            if (FAILED_WIN32_LOG(gcpiResult))
            {
                return {};
            }

            return buffer;
        }

        // Gets the package name; only succeeds if running in a packaged context.
        std::string GetPackageName()
        {
            std::unique_ptr<byte[]> buffer = GetPACKAGE_ID();
            if (!buffer)
            {
                return {};
            }

            PACKAGE_ID* packageId = reinterpret_cast<PACKAGE_ID*>(buffer.get());
            return Utility::ConvertToUTF8(packageId->name);
        }

        // Gets the package version; only succeeds if running in a packaged context.
        std::optional<PACKAGE_VERSION> GetPACKAGE_VERSION()
        {
            std::unique_ptr<byte[]> buffer = GetPACKAGE_ID();
            if (!buffer)
            {
                return {};
            }

            PACKAGE_ID* packageId = reinterpret_cast<PACKAGE_ID*>(buffer.get());
            return packageId->version;
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
            result /= "Microsoft/WinGet";

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

    LocIndString GetClientVersion()
    {
        using namespace std::string_literals;

        // Major and minor come directly from version.h
        std::ostringstream strstr;
        strstr << VERSION_MAJOR << '.' << VERSION_MINOR << '.';

        // Build comes from the package for now, if packaged.
        if (IsRunningInPackagedContext())
        {
            auto version = GetPACKAGE_VERSION();

            if (!version)
            {
                // In the extremely unlikely event of a failure, this is merely a sentinel value
                // to indicated such.  The only other option is to completely prevent execution,
                // which seems unnecessary.
                return LocIndString{ "error" };
            }

            strstr << version->Build;
        }
        else
        {
            strstr << VERSION_BUILD;
        }

        return LocIndString{ strstr.str() };
    }

    LocIndString GetPackageVersion()
    {
        using namespace std::string_literals;

        if (IsRunningInPackagedContext())
        {
            auto version = GetPACKAGE_VERSION();

            if (!version)
            {
                // In the extremely unlikely event of a failure, this is merely a sentinel value
                // to indicated such.  The only other option is to completely prevent execution,
                // which seems unnecessary.
                return LocIndString{ "error" };
            }

            std::ostringstream strstr;
            strstr << GetPackageName() << " v" << version->Major << '.' << version->Minor << '.' << version->Build << '.' << version->Revision;

            return LocIndString{ strstr.str() };
        }
        else
        {
            // Calling code should avoid calling in when this is the case.
            return LocIndString{ "none" };
        }
    }

    LocIndString GetOSVersion()
    {
        winrt::Windows::System::Profile::AnalyticsInfo analyticsInfo{};
        auto versionInfo = analyticsInfo.VersionInfo();

        uint64_t version = std::stoull(Utility::ConvertToUTF8(versionInfo.DeviceFamilyVersion()));
        uint16_t parts[4];

        for (size_t i = 0; i < ARRAYSIZE(parts); ++i)
        {
            parts[i] = version & 0xFFFF;
            version = version >> 16;
        }

        std::ostringstream strstr;
        strstr << Utility::ConvertToUTF8(versionInfo.DeviceFamily()) << " v" << parts[3] << '.' << parts[2] << '.' << parts[1] << '.' << parts[0];

        return LocIndString{ strstr.str() };
    }

    std::filesystem::path GetPathToTemp()
    {
        std::filesystem::path result;

        if (IsRunningInPackagedContext())
        {
            result.assign(winrt::Windows::Storage::ApplicationData::Current().TemporaryFolder().Path().c_str());
        }
        else
        {
            wchar_t tempPath[MAX_PATH + 1];
            DWORD tempChars = GetTempPathW(ARRAYSIZE(tempPath), tempPath);
            result.assign(std::wstring_view{ tempPath, static_cast<size_t>(tempChars) });
        }

        result /= AICLI_DEFAULT_TEMP_DIRECTORY;

        std::filesystem::create_directories(result);

        return result;
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

    std::filesystem::path GetPathToDefaultLogLocation()
    {
        if (IsRunningInPackagedContext())
        {
            // To enable UIF collection through Feedback hub, we must put our logs here.
            auto result = GetPathToLocalState() / WINGET_DEFAULT_LOG_DIRECTORY;

            std::filesystem::create_directories(result);

            return result;
        }
        else
        {
            return GetPathToTemp();
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
