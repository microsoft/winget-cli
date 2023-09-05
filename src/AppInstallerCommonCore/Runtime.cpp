// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <binver/version.h>
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerRuntime.h"
#include "Public/AppInstallerStrings.h"
#include "Public/winget/Filesystem.h"
#include "Public/winget/UserSettings.h"
#include "Public/winget/Registry.h"


#define WINGET_DEFAULT_LOG_DIRECTORY "DiagOutputDir"

namespace AppInstaller::Runtime
{
    using namespace Utility;
    using namespace Settings;
    using namespace Filesystem;

    namespace
    {
        using namespace std::string_view_literals;
        constexpr std::string_view s_DefaultTempDirectory = "WinGet"sv;
        constexpr std::string_view s_AppDataDir_Settings = "Settings"sv;
        constexpr std::string_view s_AppDataDir_State = "State"sv;
        constexpr std::string_view s_SecureSettings_Base = "Microsoft\\WinGet"sv;
        constexpr std::string_view s_SecureSettings_UserRelative = "settings"sv;
        constexpr std::string_view s_SecureSettings_Relative_Unpackaged = "win"sv;
        constexpr std::string_view s_PortablePackageUserRoot_Base = "Microsoft"sv;
        constexpr std::string_view s_PortablePackageRoot = "WinGet"sv;
        constexpr std::string_view s_PortablePackagesDirectory = "Packages"sv;
        constexpr std::string_view s_LinksDirectory = "Links"sv;
        constexpr std::string_view s_DevModeSubkey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock"sv;
        constexpr std::string_view s_AllowDevelopmentWithoutDevLicense = "AllowDevelopmentWithoutDevLicense"sv;
#ifndef WINGET_DISABLE_FOR_FUZZING
        constexpr std::string_view s_SecureSettings_Relative_Packaged = "pkg"sv;
#endif
        constexpr std::string_view s_RuntimePath_Unpackaged_DefaultState = "defaultState"sv;

        constexpr std::string_view s_UserProfileEnvironmentVariable = "%USERPROFILE%";
        constexpr std::string_view s_LocalAppDataEnvironmentVariable = "%LOCALAPPDATA%";

        static std::optional<std::string> s_runtimePathStateName;
        static wil::srwlock s_runtimePathStateNameLock;

        // Gets the path to the root of the package containing the current process.
        std::filesystem::path GetPackagePath()
        {
            wchar_t packageFullName[PACKAGE_FULL_NAME_MAX_LENGTH + 1];
            UINT32 nameLength = ARRAYSIZE(packageFullName);
            THROW_IF_WIN32_ERROR(GetPackageFullName(GetCurrentProcess(), &nameLength, packageFullName));

            UINT32 pathLength = 0;
            LONG result = GetPackagePathByFullName(packageFullName, &pathLength, nullptr);
            THROW_HR_IF(HRESULT_FROM_WIN32(result), result != ERROR_INSUFFICIENT_BUFFER);

            std::unique_ptr<wchar_t[]> buffer = std::make_unique<wchar_t[]>(pathLength);
            THROW_IF_WIN32_ERROR(GetPackagePathByFullName(packageFullName, &pathLength, buffer.get()));

            return { buffer.get() };
        }

        // Gets the path to the directory containing the currently executing binary file.
        std::filesystem::path GetBinaryDirectoryPath()
        {
            HMODULE moduleHandle = NULL;
            THROW_IF_WIN32_BOOL_FALSE(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCWSTR)&GetBinaryDirectoryPath, &moduleHandle));

            // Get the path for this module.
            wil::unique_process_heap_string binaryPath;
            THROW_IF_FAILED(wil::GetModuleFileNameW(moduleHandle, binaryPath));

            std::filesystem::path resultFilePath{ binaryPath.get() };
            return resultFilePath.parent_path();
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

#ifndef AICLI_DISABLE_TEST_HOOKS
        static std::map<PathName, PathDetails> s_Path_TestHook_Overrides;
#endif

        // Gets the user's temp path
        std::filesystem::path GetPathToUserTemp(bool forDisplay)
        {
            if (forDisplay && Settings::User().Get<Setting::AnonymizePathForDisplay>())
            {
                return "%TEMP%";
            }
            else
            {
                wchar_t tempPath[MAX_PATH + 1];
                DWORD tempChars = GetTempPathW(ARRAYSIZE(tempPath), tempPath);
                THROW_LAST_ERROR_IF(!tempChars);
                THROW_HR_IF(E_UNEXPECTED, tempChars > ARRAYSIZE(tempPath));
                return { std::wstring_view{ tempPath, static_cast<size_t>(tempChars) } };
            }
        }

        // Gets the path to the appdata root.
        // *Only used by non packaged version!*
        std::filesystem::path GetPathToAppDataRoot(bool forDisplay)
        {
            THROW_HR_IF(E_NOT_VALID_STATE, IsRunningInPackagedContext());

            std::filesystem::path result = (forDisplay && Settings::User().Get<Setting::AnonymizePathForDisplay>()) ? s_LocalAppDataEnvironmentVariable : GetKnownFolderPath(FOLDERID_LocalAppData);
            result /= "Microsoft/WinGet";

            return result;
        }

        // Gets the path to the app data relative directory.
        std::filesystem::path GetPathToAppDataDir(const std::filesystem::path& relative, bool forDisplay)
        {
            THROW_HR_IF(E_INVALIDARG, !relative.has_relative_path());
            THROW_HR_IF(E_INVALIDARG, relative.has_root_path());
            THROW_HR_IF(E_INVALIDARG, !relative.has_filename());

            std::filesystem::path result = GetPathToAppDataRoot(forDisplay);
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

        DWORD AccessPermissionsFrom(ACEPermissions permissions)
        {
            DWORD result = 0;

            if (permissions == ACEPermissions::All)
            {
                result |= GENERIC_ALL;
            }
            else
            {
                if (WI_IsFlagSet(permissions, ACEPermissions::Read))
                {
                    result |= GENERIC_READ;
                }

                if (WI_IsFlagSet(permissions, ACEPermissions::Write))
                {
                    result |= GENERIC_WRITE | FILE_DELETE_CHILD;
                }

                if (WI_IsFlagSet(permissions, ACEPermissions::Execute))
                {
                    result |= GENERIC_EXECUTE;
                }
            }

            return result;
        }

        // Contains the information about an ACE entry for a given principal.
        struct ACEDetails
        {
            ACEPrincipal Principal;
            PSID SID;
            TRUSTEE_TYPE TrusteeType;
        };

        // Try to replace LOCALAPPDATA first as it is the likely location, fall back to trying USERPROFILE.
        void ReplaceProfilePathsWithEnvironmentVariable(std::filesystem::path& path)
        {
            if (!ReplaceCommonPathPrefix(path, GetKnownFolderPath(FOLDERID_LocalAppData), s_LocalAppDataEnvironmentVariable))
            {
                ReplaceCommonPathPrefix(path, GetKnownFolderPath(FOLDERID_Profile), s_UserProfileEnvironmentVariable);
            }
        }
    }

    void SetRuntimePathStateName(std::string name)
    {
        auto suitablePathPart = MakeSuitablePathPart(name);
        auto lock = s_runtimePathStateNameLock.lock_exclusive();
        s_runtimePathStateName.emplace(std::move(suitablePathPart));
    }

    void PathDetails::SetOwner(ACEPrincipal owner)
    {
        Owner = owner;
        ACL[owner] = ACEPermissions::All;
    }

    bool PathDetails::ShouldApplyACL() const
    {
        // Could be expanded to actually check the current owner/ACL on the path, but isn't worth it currently
        return !ACL.empty();
    }

    void PathDetails::ApplyACL() const
    {
        bool hasCurrentUser = ACL.count(ACEPrincipal::CurrentUser) != 0;
        bool hasSystem = ACL.count(ACEPrincipal::System) != 0;

        // Configuring permissions for both CurrentUser and SYSTEM while not having owner set as one of them is not valid because
        // below we use only the owner permissions in the case of running as SYSTEM.
        if ((hasCurrentUser && hasSystem) &&
            IsRunningAsSystem() &&
            (!Owner || (Owner.value() != ACEPrincipal::CurrentUser && Owner.value() != ACEPrincipal::System)))
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
        }

        auto userToken = wil::get_token_information<TOKEN_USER>();
        auto adminSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
        auto systemSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID);
        PSID ownerSID = nullptr;

        ACEDetails aceDetails[] =
        {
            { ACEPrincipal::CurrentUser, userToken->User.Sid, TRUSTEE_IS_USER },
            { ACEPrincipal::Admins, adminSID.get(), TRUSTEE_IS_WELL_KNOWN_GROUP},
            { ACEPrincipal::System, systemSID.get(), TRUSTEE_IS_USER},
        };

        ULONG entriesCount = 0;
        std::array<EXPLICIT_ACCESS_W, ARRAYSIZE(aceDetails)> explicitAccess;

        // If the current user is SYSTEM, we want to take either the owner or the only configured set of permissions.
        // The check above should prevent us from getting into situations outside of the ones below.
        std::optional<ACEPrincipal> principalToIgnore;
        if (hasCurrentUser && hasSystem && EqualSid(userToken->User.Sid, systemSID.get()))
        {
            principalToIgnore = (Owner.value() == ACEPrincipal::CurrentUser ? ACEPrincipal::System : ACEPrincipal::CurrentUser);
        }

        for (const auto& ace : aceDetails)
        {
            if (principalToIgnore && principalToIgnore.value() == ace.Principal)
            {
                continue;
            }

            if (Owner && Owner.value() == ace.Principal)
            {
                ownerSID = ace.SID;
            }

            auto itr = ACL.find(ace.Principal);
            if (itr != ACL.end())
            {
                EXPLICIT_ACCESS_W& entry = explicitAccess[entriesCount++];
                entry = {};

                entry.grfAccessPermissions = AccessPermissionsFrom(itr->second);
                entry.grfAccessMode = SET_ACCESS;
                entry.grfInheritance = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

                entry.Trustee.pMultipleTrustee = nullptr;
                entry.Trustee.MultipleTrusteeOperation = NO_MULTIPLE_TRUSTEE;
                entry.Trustee.TrusteeForm = TRUSTEE_IS_SID;
                entry.Trustee.TrusteeType = ace.TrusteeType;
                entry.Trustee.ptstrName = reinterpret_cast<LPWCH>(ace.SID);
            }
        }

        wil::unique_any<PACL, decltype(&::LocalFree), ::LocalFree> acl;
        THROW_IF_WIN32_ERROR(SetEntriesInAclW(entriesCount, explicitAccess.data(), nullptr, &acl));

        std::wstring path = Path.wstring();
        SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION;

        if (ownerSID)
        {
            securityInformation |= OWNER_SECURITY_INFORMATION;
        }

        THROW_IF_WIN32_ERROR(SetNamedSecurityInfoW(&path[0], SE_FILE_OBJECT, securityInformation, ownerSID, nullptr, acl.get(), nullptr));
    }

    // Contains all of the paths that are common between the runtime contexts.
    PathDetails GetPathDetailsCommon(PathName path, bool forDisplay)
    {
        PathDetails result;
        // We should not create directories by default when they are retrieved for display purposes.
        result.Create = !forDisplay;

        bool mayBeInProfilePath = false;

        switch (path)
        {
        case PathName::UserProfile:
            result.Path = (forDisplay && Settings::User().Get<Setting::AnonymizePathForDisplay>()) ? s_UserProfileEnvironmentVariable : GetKnownFolderPath(FOLDERID_Profile);
            result.Create = false;
            break;
        case PathName::PortablePackageUserRoot:
            result.Path = Settings::User().Get<Setting::PortablePackageUserRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_LocalAppData);
                result.Path /= s_PortablePackageUserRoot_Base;
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            mayBeInProfilePath = true;
            break;
        case PathName::PortablePackageMachineRoot:
            result.Path = Settings::User().Get<Setting::PortablePackageMachineRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_ProgramFiles);
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            break;
        case PathName::PortablePackageMachineRootX86:
            result.Path = Settings::User().Get<Setting::PortablePackageMachineRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_ProgramFilesX86);
                result.Path /= s_PortablePackageRoot;
                result.Path /= s_PortablePackagesDirectory;
            }
            break;
        case PathName::PortableLinksUserLocation:
            result.Path = GetKnownFolderPath(FOLDERID_LocalAppData);
            result.Path /= s_PortablePackageUserRoot_Base;
            result.Path /= s_PortablePackageRoot;
            result.Path /= s_LinksDirectory;
            mayBeInProfilePath = true;
            break;
        case PathName::PortableLinksMachineLocation:
            result.Path = GetKnownFolderPath(FOLDERID_ProgramFiles);
            result.Path /= s_PortablePackageRoot;
            result.Path /= s_LinksDirectory;
            break;
        case PathName::UserProfileDownloads:
            result.Path = GetKnownFolderPath(FOLDERID_Downloads);
            mayBeInProfilePath = true;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        if (mayBeInProfilePath && forDisplay && Settings::User().Get<Setting::AnonymizePathForDisplay>())
        {
            ReplaceProfilePathsWithEnvironmentVariable(result.Path);
        }

        return result;
    }

#ifndef WINGET_DISABLE_FOR_FUZZING
    PathDetails GetPathDetailsForPackagedContext(PathName path, bool forDisplay)
    {
        PathDetails result;
        // We should not create directories by default when they are retrieved for display purposes.
        result.Create = !forDisplay;

        auto appStorage = winrt::Windows::Storage::ApplicationData::Current();
        bool mayBeInProfilePath = false;

        switch (path)
        {
        case PathName::Temp:
            result.Path = GetPathToUserTemp(forDisplay) / s_DefaultTempDirectory;
            result.SetOwner(ACEPrincipal::CurrentUser);
            result.ACL[ACEPrincipal::System] = ACEPermissions::All;
            result.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
            break;
        case PathName::LocalState:
        case PathName::UserFileSettings:
            result.Path.assign(appStorage.LocalFolder().Path().c_str());
            mayBeInProfilePath = true;
            break;
        case PathName::DefaultLogLocation:
            // To enable UIF collection through Feedback hub, we must put our logs here.
            result.Path.assign(appStorage.LocalFolder().Path().c_str());
            result.Path /= WINGET_DEFAULT_LOG_DIRECTORY;
            mayBeInProfilePath = true;
            break;
        case PathName::StandardSettings:
            result.Create = false;
            break;
        case PathName::SecureSettingsForRead:
        case PathName::SecureSettingsForWrite:
            result.Path = GetKnownFolderPath(FOLDERID_ProgramData);
            result.Path /= s_SecureSettings_Base;
            result.Path /= GetUserSID();
            result.Path /= s_SecureSettings_UserRelative;
            result.Path /= s_SecureSettings_Relative_Packaged;
            result.Path /= GetPackageName();
            if (path == PathName::SecureSettingsForWrite)
            {
                result.SetOwner(ACEPrincipal::Admins);
                // When running as system, we do not set current user permissions to avoid permission conflicts.
                if (!IsRunningAsSystem())
                {
                    result.ACL[ACEPrincipal::CurrentUser] = ACEPermissions::ReadExecute;
                }
                result.ACL[ACEPrincipal::System] = ACEPermissions::All;
            }
            else
            {
                result.Create = false;
            }
            break;
        case PathName::UserProfile:
        case PathName::PortablePackageMachineRoot:
        case PathName::PortablePackageMachineRootX86:
        case PathName::PortableLinksMachineLocation:
        case PathName::PortableLinksUserLocation:
        case PathName::PortablePackageUserRoot:
        case PathName::UserProfileDownloads:
            result = GetPathDetailsCommon(path, forDisplay);
            break;
        case PathName::SelfPackageRoot:
            result.Path = GetPackagePath();
            result.Create = false;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        if (mayBeInProfilePath && forDisplay && Settings::User().Get<Setting::AnonymizePathForDisplay>())
        {
            ReplaceProfilePathsWithEnvironmentVariable(result.Path);
        }

        return result;
    }
#endif

    PathDetails GetPathDetailsForUnpackagedContext(PathName path, bool forDisplay)
    {
        PathDetails result;
        // We should not create directories by default when they are retrieved for display purposes.
        result.Create = !forDisplay;

        switch (path)
        {
        case PathName::Temp:
        case PathName::DefaultLogLocation:
        {
            result.Path = GetPathToUserTemp(forDisplay);
            result.Path /= s_DefaultTempDirectory;
            result.Path /= GetRuntimePathStateName();
            if (path == PathName::Temp)
            {
                result.SetOwner(ACEPrincipal::CurrentUser);
                result.ACL[ACEPrincipal::System] = ACEPermissions::All;
                result.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
            }
        }
        break;
        case PathName::LocalState:
            result.Path = GetPathToAppDataDir(s_AppDataDir_State, forDisplay);
            result.Path /= GetRuntimePathStateName();
            result.SetOwner(ACEPrincipal::CurrentUser);
            result.ACL[ACEPrincipal::System] = ACEPermissions::All;
            result.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
            break;
        case PathName::StandardSettings:
        case PathName::UserFileSettings:
            result.Path = GetPathToAppDataDir(s_AppDataDir_Settings, forDisplay);
            result.Path /= GetRuntimePathStateName();
            result.SetOwner(ACEPrincipal::CurrentUser);
            result.ACL[ACEPrincipal::System] = ACEPermissions::All;
            result.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
            break;
        case PathName::SecureSettingsForRead:
        case PathName::SecureSettingsForWrite:
            result.Path = GetKnownFolderPath(FOLDERID_ProgramData);
            result.Path /= s_SecureSettings_Base;
            result.Path /= GetUserSID();
            result.Path /= s_SecureSettings_UserRelative;
            result.Path /= s_SecureSettings_Relative_Unpackaged;
            result.Path /= GetRuntimePathStateName();
            if (path == PathName::SecureSettingsForWrite)
            {
                result.SetOwner(ACEPrincipal::Admins);
                // When running as system, we do not set current user permissions to avoid permission conflicts.
                if (!IsRunningAsSystem())
                {
                    result.ACL[ACEPrincipal::CurrentUser] = ACEPermissions::ReadExecute;
                }
                result.ACL[ACEPrincipal::System] = ACEPermissions::All;
            }
            else
            {
                result.Create = false;
            }
            break;
        case PathName::UserProfile:
        case PathName::PortablePackageMachineRoot:
        case PathName::PortablePackageMachineRootX86:
        case PathName::PortableLinksMachineLocation:
        case PathName::PortableLinksUserLocation:
        case PathName::PortablePackageUserRoot:
        case PathName::UserProfileDownloads:
            result = GetPathDetailsCommon(path, forDisplay);
            break;
        case PathName::SelfPackageRoot:
            result.Path = GetBinaryDirectoryPath();
            result.Create = false;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

    PathDetails GetPathDetailsFor(PathName path, bool forDisplay)
    {
        PathDetails result;

#ifndef WINGET_DISABLE_FOR_FUZZING
        if (IsRunningInPackagedContext())
        {
            result = GetPathDetailsForPackagedContext(path, forDisplay);
        }
        else
#endif
        {
            result = GetPathDetailsForUnpackagedContext(path, forDisplay);
        }

#ifndef AICLI_DISABLE_TEST_HOOKS
        // Override the value after letting the normal code path run
        auto itr = s_Path_TestHook_Overrides.find(path);
        if (itr != s_Path_TestHook_Overrides.end())
        {
            result = itr->second;
        }
#endif

        return result;
    }

    std::filesystem::path GetPathTo(PathName path, bool forDisplay)
    {
        PathDetails details = GetPathDetailsFor(path, forDisplay);

        if (details.Create)
        {
            if (details.Path.is_absolute())
            {
                if (std::filesystem::exists(details.Path) && !std::filesystem::is_directory(details.Path))
                {
                    std::filesystem::remove(details.Path);
                }

                std::filesystem::create_directories(details.Path);

                // Set the ACLs on the directory if needed. We do this after creating the directory because an attacker could
                // have created the directory beforehand so we must be able to place the correct ACL on any directory or fail
                // to operate.
                if (details.ShouldApplyACL())
                {
                    details.ApplyACL();
                }
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

    // Determines whether developer mode is enabled.
    // Does not account for the group policy value which takes precedence over this registry value.
    bool IsDevModeEnabled()
    {
        const auto& devModeSubKey = Registry::Key::OpenIfExists(HKEY_LOCAL_MACHINE, s_DevModeSubkey, 0, KEY_READ|KEY_WOW64_64KEY);
        const auto& devModeEnabled = devModeSubKey[s_AllowDevelopmentWithoutDevLicense];
        if (devModeEnabled.has_value())
        {
            return devModeEnabled->GetValue<Registry::Value::Type::DWord>() == 1;
        }
        else
        {
            return false;
        }
    }

    // Using "standard" user agent format
    // Keeping `winget-cli` for historical reasons
    Utility::LocIndString GetDefaultUserAgent()
    {
        std::ostringstream strstr;
        strstr <<
            "winget-cli" <<
            " WindowsPackageManager/" << GetClientVersion() <<
            " DesktopAppInstaller/" << GetPackageVersion();
        return Utility::LocIndString{ strstr.str() };
    }

    Utility::LocIndString GetUserAgent(std::string_view caller)
    {
        std::ostringstream strstr;
        strstr <<
            caller <<
            " WindowsPackageManager/" << GetClientVersion() <<
            " DesktopAppInstaller/" << GetPackageVersion();
        return Utility::LocIndString{ strstr.str() };
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    void TestHook_SetPathOverride(PathName target, const std::filesystem::path& path)
    {
        if (s_Path_TestHook_Overrides.count(target))
        {
            s_Path_TestHook_Overrides[target].Path = path;
        }
        else
        {
            PathDetails details = GetPathDetailsFor(target);
            details.Path = path;
            s_Path_TestHook_Overrides[target] = std::move(details);
        }
    }

    void TestHook_SetPathOverride(PathName target, const PathDetails& details)
    {
        s_Path_TestHook_Overrides[target] = details;
    }

    void TestHook_ClearPathOverrides()
    {
        s_Path_TestHook_Overrides.clear();
    }
#endif
}
