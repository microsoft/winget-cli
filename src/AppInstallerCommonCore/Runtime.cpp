// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <binver/version.h>
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"

#define WINGET_DEFAULT_LOG_DIRECTORY "DiagOutputDir"

namespace AppInstaller::Runtime
{
    using namespace Utility;

    namespace
    {
        using namespace std::string_view_literals;
        constexpr std::string_view s_DefaultTempDirectory = "WinGet"sv;
        constexpr std::string_view s_AppDataDir_Settings = "Settings"sv;
        constexpr std::string_view s_AppDataDir_State = "State"sv;
        constexpr std::string_view s_SecureSettings_Base = "Microsoft/WinGet"sv;
        constexpr std::string_view s_SecureSettings_UserRelative = "settings"sv;
        constexpr std::string_view s_SecureSettings_Relative_Packaged = "pkg"sv;
        constexpr std::string_view s_SecureSettings_Relative_Unpackaged = "win"sv;

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
        static std::map<PathName, std::filesystem::path> s_Path_TestHook_Overrides;
#endif

        std::filesystem::path GetKnownFolderPath(const KNOWNFOLDERID& id)
        {
            wil::unique_cotaskmem_string knownFolder = nullptr;
            THROW_IF_FAILED(SHGetKnownFolderPath(id, KF_FLAG_NO_ALIAS | KF_FLAG_DONT_VERIFY | KF_FLAG_NO_PACKAGE_REDIRECTION, NULL, &knownFolder));
            return knownFolder.get();
        }

        // Gets the path to the appdata root.
        // *Only used by non packaged version!*
        std::filesystem::path GetPathToAppDataRoot()
        {
            THROW_HR_IF(E_NOT_VALID_STATE, IsRunningInPackagedContext());

            std::filesystem::path result = GetKnownFolderPath(FOLDERID_LocalAppData);
            result /= "Microsoft/WinGet";

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

            return result;
        }

        // Gets the current user's SID for use in paths.
        std::filesystem::path GetUserSID()
        {
            auto userToken = wil::get_token_information<TOKEN_USER>();

            wil::unique_hlocal_string sidString;
            THROW_IF_WIN32_BOOL_FALSE(ConvertSidToStringSidW(userToken->User.Sid, &sidString));
            return { sidString.get() };
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
                return LocIndString{ "error"sv };
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
                return LocIndString{ "error"sv };
            }

            std::ostringstream strstr;
            strstr << GetPackageName() << " v" << version->Major << '.' << version->Minor << '.' << version->Build << '.' << version->Revision;

            return LocIndString{ strstr.str() };
        }
        else
        {
            // Calling code should avoid calling in when this is the case.
            return LocIndString{ "none"sv };
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

    std::filesystem::path GetPathTo(PathName path)
    {
        std::filesystem::path result;
        bool create = true;

        if (IsRunningInPackagedContext())
        {
            auto appStorage = winrt::Windows::Storage::ApplicationData::Current();

            switch (path)
            {
            case PathName::Temp:
                result.assign(appStorage.TemporaryFolder().Path().c_str());
                result /= s_DefaultTempDirectory;
                break;
            case PathName::LocalState:
            case PathName::UserFileSettings:
                result.assign(appStorage.LocalFolder().Path().c_str());
                break;
            case PathName::DefaultLogLocation:
                // To enable UIF collection through Feedback hub, we must put our logs here.
                result.assign(appStorage.LocalFolder().Path().c_str());
                result /= WINGET_DEFAULT_LOG_DIRECTORY;
                break;
            case PathName::StandardSettings:
                create = false;
                break;
            case PathName::SecureSettings:
                result = GetKnownFolderPath(FOLDERID_ProgramData);
                result /= s_SecureSettings_Base;
                result /= GetUserSID();
                result /= s_SecureSettings_UserRelative;
                result /= s_SecureSettings_Relative_Packaged;
                result /= GetPackageName();
                create = false;
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }
        else
        {
            switch (path)
            {
            case PathName::Temp:
            case PathName::DefaultLogLocation:
            {
                wchar_t tempPath[MAX_PATH + 1];
                DWORD tempChars = GetTempPathW(ARRAYSIZE(tempPath), tempPath);
                result.assign(std::wstring_view{ tempPath, static_cast<size_t>(tempChars) });

                result /= s_DefaultTempDirectory;
            }
            break;
            case PathName::LocalState:
                result = GetPathToAppDataDir(s_AppDataDir_State);
                break;
            case PathName::StandardSettings:
            case PathName::UserFileSettings:
                result = GetPathToAppDataDir(s_AppDataDir_Settings);
                break;
            case PathName::SecureSettings:
                result = GetKnownFolderPath(FOLDERID_ProgramData);
                result /= s_SecureSettings_Base;
                result /= GetUserSID();
                result /= s_SecureSettings_UserRelative;
                result /= s_SecureSettings_Relative_Unpackaged;
                create = false;
                break;
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }

#ifndef AICLI_DISABLE_TEST_HOOKS
        // Override the value after letting the normal code path run
        auto itr = s_Path_TestHook_Overrides.find(path);
        if (itr != s_Path_TestHook_Overrides.end())
        {
            result = itr->second;
        }
#endif

        if (create && result.is_absolute())
        {
            if (std::filesystem::exists(result))
            {
                if (!std::filesystem::is_directory(result))
                {
                    // STATUS_NOT_A_DIRECTORY: A requested opened file is not a directory.
                    THROW_NTSTATUS_MSG(0xC0000103, "Location is not a directory");
                }
            }
            else
            {
                std::filesystem::create_directories(result);
            }
        }

        return result;
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

    bool IsRunningAsAdmin()
    {
        return wil::test_token_membership(nullptr, SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void TestHook_SetPathOverride(PathName target, const std::filesystem::path& path)
    {
        s_Path_TestHook_Overrides[target] = path;
    }

    void TestHook_ClearPathOverrides()
    {
        s_Path_TestHook_Overrides.clear();
    }
#endif
}
