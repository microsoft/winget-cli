// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <binver/version.h>
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"
#include "Public/winget/UserSettings.h"

#include <optional>

#define WINGET_DEFAULT_LOG_DIRECTORY "DiagOutputDir"

namespace AppInstaller::Runtime
{
    using namespace Utility;
    using namespace Settings;

    namespace
    {
        using namespace std::string_view_literals;
        constexpr std::string_view s_DefaultTempDirectory = "WinGet"sv;
        constexpr std::string_view s_AppDataDir_Settings = "Settings"sv;
        constexpr std::string_view s_AppDataDir_State = "State"sv;
        constexpr std::string_view s_SecureSettings_Base = "Microsoft/WinGet"sv;
        constexpr std::string_view s_SecureSettings_UserRelative = "settings"sv;
        constexpr std::string_view s_SecureSettings_Relative_Unpackaged = "win"sv;
        constexpr std::string_view s_PortablePackageUserRoot_Base = "Microsoft"sv;
        constexpr std::string_view s_PortablePackageRoot = "WinGet"sv;
        constexpr std::string_view s_PortablePackagesDirectory = "Packages"sv;
        constexpr std::string_view s_LinksDirectory = "Links"sv;
#ifndef WINGET_DISABLE_FOR_FUZZING
        constexpr std::string_view s_SecureSettings_Relative_Packaged = "pkg"sv;
#endif
        constexpr std::string_view s_PreviewBuildSuffix = "-preview"sv;
        constexpr std::string_view s_RuntimePath_Unpackaged_DefaultState = "defaultState"sv;

        static std::optional<std::string> s_runtimePathStateName;
        static wil::srwlock s_runtimePathStateNameLock;

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

        // Gets the user's temp path
        std::filesystem::path GetPathToUserTemp()
        {
            wchar_t tempPath[MAX_PATH + 1];
            DWORD tempChars = GetTempPathW(ARRAYSIZE(tempPath), tempPath);
            THROW_LAST_ERROR_IF(!tempChars);
            THROW_HR_IF(E_UNEXPECTED, tempChars > ARRAYSIZE(tempPath));
            return { std::wstring_view{ tempPath, static_cast<size_t>(tempChars) } };
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

        std::string GetRuntimePathStateName()
        {
            std::string result;
            auto lock = s_runtimePathStateNameLock.lock_shared();

            if (s_runtimePathStateName.has_value())
            {
                result = s_runtimePathStateName.value();
            }

            if (Utility::IsEmptyOrWhitespace(result))
            {
                result = s_RuntimePath_Unpackaged_DefaultState;
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
                return LocIndString{ "error"sv };
            }

            strstr << version->Build;
        }
        else
        {
            strstr << VERSION_BUILD;
        }

        if (!IsReleaseBuild())
        {
            strstr << s_PreviewBuildSuffix;
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

#ifndef WINGET_DISABLE_FOR_FUZZING
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

    std::string GetOSRegion()
    {
        winrt::Windows::Globalization::GeographicRegion region;
        return Utility::ConvertToUTF8(region.CodeTwoLetter());
    }
#endif

    void SetRuntimePathStateName(std::string name)
    {
        auto suitablePathPart = MakeSuitablePathPart(name);
        auto lock = s_runtimePathStateNameLock.lock_exclusive();
        s_runtimePathStateName.emplace(std::move(suitablePathPart));
    }

    // The permissions granted to a specific ACE
    enum class ACEPermissions
    {
        // This is not "Deny All", but rather, "Not mentioned"
        None = 0x0,
        Read = 0x1,
        Write = 0x2,
        Execute = 0x4,
        ReadWrite = Read | Write,
        ReadWriteExecute = Read | Write | Execute,
    };

    DEFINE_ENUM_FLAG_OPERATORS(ACEPermissions);

    // Information about one of the paths that we use and how to set it up.
    struct PathDetails
    {
        std::filesystem::path Path;
        // Default to creating the directory with read/write permissions for the current effective user
        bool Create = true;
        ACEPermissions CurrentUser = ACEPermissions::ReadWrite;
        ACEPermissions Admins = ACEPermissions::None;
        ACEPermissions Users = ACEPermissions::None;
    };

#ifndef WINGET_DISABLE_FOR_FUZZING
    PathDetails GetPathDetailsForPackagedContext(PathName path)
    {
        PathDetails result;

        auto appStorage = winrt::Windows::Storage::ApplicationData::Current();

        switch (path)
        {
        case PathName::Temp:
        {
            result.Path = GetPathToUserTemp() / s_DefaultTempDirectory;
        }
        break;
        case PathName::LocalState:
        case PathName::UserFileSettings:
            result.Path.assign(appStorage.LocalFolder().Path().c_str());
            break;
        case PathName::DefaultLogLocation:
        case PathName::DefaultLogLocationForDisplay:
            // To enable UIF collection through Feedback hub, we must put our logs here.
            result.Path.assign(appStorage.LocalFolder().Path().c_str());
            result.Path /= WINGET_DEFAULT_LOG_DIRECTORY;

            if (path == PathName::DefaultLogLocationForDisplay)
            {
                std::filesystem::path localAppData = GetKnownFolderPath(FOLDERID_LocalAppData);

                auto ladItr = localAppData.begin();
                auto resultItr = result.Path.begin();

                while (ladItr != localAppData.end() && resultItr != result.Path.end())
                {
                    if (*ladItr != *resultItr)
                    {
                        break;
                    }

                    ++ladItr;
                    ++resultItr;
                }

                if (ladItr == localAppData.end())
                {
                    localAppData.assign("%LOCALAPPDATA%");

                    for (; resultItr != result.Path.end(); ++resultItr)
                    {
                        localAppData /= *resultItr;
                    }

                    result.Path = std::move(localAppData);
                }
            }
            break;
        case PathName::StandardSettings:
            result.Create = false;
            break;
        case PathName::SecureSettings:
            result.Path = GetKnownFolderPath(FOLDERID_ProgramData);
            result.Path /= s_SecureSettings_Base;
            result.Path /= GetUserSID();
            result.Path /= s_SecureSettings_UserRelative;
            result.Path /= s_SecureSettings_Relative_Packaged;
            result.Path /= GetPackageName();
            result.Create = false;
            break;
        case PathName::UserProfile:
            result.Path = GetKnownFolderPath(FOLDERID_Profile);
            result.Create = false;
            break;
        case PathName::PortablePackageUserRoot:
            result.Path = Settings::User().Get<Setting::PortableAppUserRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_LocalAppData);
                result.Path /= s_PortablePackageUserRoot_Base;
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            result.Create = true;
            break;
        case PathName::PortablePackageMachineRootX64:
            result.Path = Settings::User().Get<Setting::PortableAppMachineRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_ProgramFilesX64);
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            result.Create = true;
            break;
        case PathName::PortablePackageMachineRootX86:
            result.Path = Settings::User().Get<Setting::PortableAppMachineRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_ProgramFilesX86);
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            result.Create = true;
            break;
        case PathName::PortableLinksUserLocation:
            result.Path = GetKnownFolderPath(FOLDERID_LocalAppData);
            result.Path /= s_PortablePackageUserRoot_Base;
            result.Path /= s_PortablePackageRoot;
            result.Path /= s_LinksDirectory;
            result.Create = true;
            break;
        case PathName::PortableLinksMachineLocation:
            result.Path = GetKnownFolderPath(FOLDERID_ProgramFilesX64);
            result.Path /= s_PortablePackageRoot;
            result.Path /= s_LinksDirectory;
            result.Create = true;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }
#endif

    PathDetails GetPathDetailsForUnpackagedContext(PathName path)
    {
        PathDetails result;

        switch (path)
        {
        case PathName::Temp:
        case PathName::DefaultLogLocation:
        {
            result.Path = GetPathToUserTemp();
            result.Path /= s_DefaultTempDirectory;
            result.Path /= GetRuntimePathStateName();
        }
        break;
        case PathName::DefaultLogLocationForDisplay:
            result.Path.assign("%TEMP%");
            result.Path /= s_DefaultTempDirectory;
            result.Path /= GetRuntimePathStateName();
            result.Create = false;
            break;
        case PathName::LocalState:
            result.Path = GetPathToAppDataDir(s_AppDataDir_State);
            result.Path /= GetRuntimePathStateName();
            break;
        case PathName::StandardSettings:
        case PathName::UserFileSettings:
            result.Path = GetPathToAppDataDir(s_AppDataDir_Settings);
            result.Path /= GetRuntimePathStateName();
            break;
        case PathName::SecureSettings:
            result.Path = GetKnownFolderPath(FOLDERID_ProgramData);
            result.Path /= s_SecureSettings_Base;
            result.Path /= GetUserSID();
            result.Path /= s_SecureSettings_UserRelative;
            result.Path /= s_SecureSettings_Relative_Unpackaged;
            result.Path /= GetRuntimePathStateName();
            result.Create = false;
            break;
        case PathName::UserProfile:
            result.Path = GetKnownFolderPath(FOLDERID_Profile);
            result.Create = false;
            break;
        case PathName::PortablePackageUserRoot:
            result.Path = Settings::User().Get<Setting::PortableAppUserRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_LocalAppData);
                result.Path /= s_PortablePackageUserRoot_Base;
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            result.Create = true;
            break;
        case PathName::PortablePackageMachineRootX64:
            result.Path = Settings::User().Get<Setting::PortableAppMachineRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_ProgramFilesX64);
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            result.Create = true;
            break;
        case PathName::PortablePackageMachineRootX86:
            result.Path = Settings::User().Get<Setting::PortableAppMachineRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_ProgramFilesX86);
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            result.Create = true;
            break;
        case PathName::PortableLinksUserLocation:
            result.Path = GetKnownFolderPath(FOLDERID_LocalAppData);
            result.Path /= s_PortablePackageUserRoot_Base;
            result.Path /= s_PortablePackageRoot;
            result.Path /= s_LinksDirectory;
            result.Create = true;
            break;
        case PathName::PortableLinksMachineLocation:
            result.Path = GetKnownFolderPath(FOLDERID_ProgramFilesX64);
            result.Path /= s_PortablePackageRoot;
            result.Path /= s_LinksDirectory;
            result.Create = true;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

    std::filesystem::path GetPathTo(PathName path)
    {
        PathDetails details;

#ifndef WINGET_DISABLE_FOR_FUZZING
        if (IsRunningInPackagedContext())
        {
            details = GetPathDetailsForPackagedContext(path);
        }
        else
#endif
        {
            details = GetPathDetailsForUnpackagedContext(path);
        }

#ifndef AICLI_DISABLE_TEST_HOOKS
        // Override the value after letting the normal code path run
        auto itr = s_Path_TestHook_Overrides.find(path);
        if (itr != s_Path_TestHook_Overrides.end())
        {
            details.Path = itr->second;
        }
#endif

        if (details.Create)
        {
            if (details.Path.is_absolute())
            {
                if (std::filesystem::exists(details.Path) && !std::filesystem::is_directory(details.Path))
                {
                    std::filesystem::remove(details.Path);
                }

                std::filesystem::create_directories(details.Path);
            }
            else
            {
                AICLI_LOG(Core, Warning, << "GetPathTo directory creation requested for [" << path << "], but path was not absolute: " << details.Path);
            }
        }

        return std::move(details.Path);
    }

    std::filesystem::path GetNewTempFilePath()
    {
        GUID guid;
        THROW_IF_FAILED(CoCreateGuid(&guid));
        WCHAR tempFileName[256];
        THROW_HR_IF(E_UNEXPECTED, StringFromGUID2(guid, tempFileName, ARRAYSIZE(tempFileName)) == 0);
        auto tempFilePath = Runtime::GetPathTo(Runtime::PathName::Temp);
        tempFilePath /= tempFileName;

        return tempFilePath;
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

    constexpr bool IsReleaseBuild()
    {
#ifdef WINGET_ENABLE_RELEASE_BUILD
        return true;
#else
        return false;
#endif
    }

    // Using "standard" user agent format
    // Keeping `winget-cli` for historical reasons
    Utility::LocIndString GetDefaultUserAgent()
    {
        std::ostringstream strstr;
        strstr <<
            "winget-cli"
            " WindowsPackageManager/" << GetClientVersion() <<
            " DesktopAppInstaller/" << GetPackageVersion();
        return Utility::LocIndString{ strstr.str() };
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
