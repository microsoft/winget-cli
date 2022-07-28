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
        constexpr std::string_view s_DevModeSubkey = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock"sv;
        constexpr std::string_view s_AllowDevelopmentWithoutDevLicense = "AllowDevelopmentWithoutDevLicense"sv;
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
        static std::map<PathName, PathDetails> s_Path_TestHook_Overrides;
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

        // If `source` begins with all of `prefix`, replace that with `replacement`.
        void ReplaceCommonPathPrefix(std::filesystem::path& source, const std::filesystem::path& prefix, std::string_view replacement)
        {
            auto prefixItr = prefix.begin();
            auto sourceItr = source.begin();

            while (prefixItr != prefix.end() && sourceItr != source.end())
            {
                if (*prefixItr != *sourceItr)
                {
                    break;
                }

                ++prefixItr;
                ++sourceItr;
            }

            // Only replace source if we found all of prefix
            if (prefixItr == prefix.end())
            {
                std::filesystem::path temp{ replacement };

                for (; sourceItr != source.end(); ++sourceItr)
                {
                    temp /= *sourceItr;
                }

                source = std::move(temp);
            }
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
    PathDetails GetPathDetailsCommon(PathName path)
    {
        PathDetails result;

        switch (path)
        {
        case PathName::UserProfile:
            result.Path = GetKnownFolderPath(FOLDERID_Profile);
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
            break;
        case PathName::PortablePackageMachineRootX64:
            result.Path = Settings::User().Get<Setting::PortablePackageMachineRoot>();
            if (result.Path.empty())
            {
                result.Path = GetKnownFolderPath(FOLDERID_ProgramFilesX64);
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
            break;
        case PathName::PortableLinksMachineLocation:
            result.Path = GetKnownFolderPath(FOLDERID_ProgramFiles);
            result.Path /= s_PortablePackageRoot;
            result.Path /= s_LinksDirectory;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

#ifndef WINGET_DISABLE_FOR_FUZZING
    PathDetails GetPathDetailsForPackagedContext(PathName path)
    {
        PathDetails result;

        auto appStorage = winrt::Windows::Storage::ApplicationData::Current();

        switch (path)
        {
        case PathName::Temp:
            result.Path = GetPathToUserTemp() / s_DefaultTempDirectory;
            result.SetOwner(ACEPrincipal::CurrentUser);
            result.ACL[ACEPrincipal::System] = ACEPermissions::All;
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
                ReplaceCommonPathPrefix(result.Path, GetKnownFolderPath(FOLDERID_LocalAppData), "%LOCALAPPDATA%");
            }
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
                result.ACL[ACEPrincipal::CurrentUser] = ACEPermissions::ReadExecute;
            }
            else
            {
                result.Create = false;
            }
            break;
        case PathName::UserProfile:
        case PathName::PortablePackageUserRoot:
        case PathName::PortablePackageMachineRootX64:
        case PathName::PortablePackageMachineRootX86:
        case PathName::PortableLinksUserLocation:
        case PathName::PortableLinksMachineLocation:
            result = GetPathDetailsCommon(path);
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
            if (path == PathName::Temp)
            {
                result.SetOwner(ACEPrincipal::CurrentUser);
                result.ACL[ACEPrincipal::System] = ACEPermissions::All;
            }
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
            result.SetOwner(ACEPrincipal::CurrentUser);
            break;
        case PathName::StandardSettings:
        case PathName::UserFileSettings:
            result.Path = GetPathToAppDataDir(s_AppDataDir_Settings);
            result.Path /= GetRuntimePathStateName();
            result.SetOwner(ACEPrincipal::CurrentUser);
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
                result.ACL[ACEPrincipal::CurrentUser] = ACEPermissions::ReadExecute;
            }
            else
            {
                result.Create = false;
            }
            break;
        case PathName::UserProfile:
        case PathName::PortablePackageUserRoot:
        case PathName::PortablePackageMachineRootX64:
        case PathName::PortablePackageMachineRootX86:
        case PathName::PortableLinksUserLocation:
        case PathName::PortableLinksMachineLocation:
            result = GetPathDetailsCommon(path);
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

    PathDetails GetPathDetailsFor(PathName path)
    {
        PathDetails result;

#ifndef WINGET_DISABLE_FOR_FUZZING
        if (IsRunningInPackagedContext())
        {
            result = GetPathDetailsForPackagedContext(path);
        }
        else
#endif
        {
            result = GetPathDetailsForUnpackagedContext(path);
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

    std::filesystem::path GetPathTo(PathName path)
    {
        PathDetails details = GetPathDetailsFor(path);

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
