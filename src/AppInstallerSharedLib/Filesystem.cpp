// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Filesystem.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerLogging.h"
#include "Public/winget/Runtime.h"

using namespace std::chrono_literals;
using namespace std::string_view_literals;
using namespace AppInstaller::Runtime;

namespace AppInstaller::Filesystem
{
    namespace anon
    {
        constexpr std::string_view s_AppDataDir_Settings = "Settings"sv;
        constexpr std::string_view s_AppDataDir_State = "State"sv;

        constexpr std::string_view s_LocalAppDataEnvironmentVariable = "%LOCALAPPDATA%";

        // Contains the information about an ACE entry for a given principal.
        struct ACEDetails
        {
            ACEPrincipal Principal;
            PSID SID;
            TRUSTEE_TYPE TrusteeType;
        };

        constexpr BYTE s_InheritableAceFlags = CONTAINER_INHERIT_ACE | OBJECT_INHERIT_ACE;

        struct PrincipalPermissions
        {
            DWORD DirectAccessMask = 0;
            DWORD ObjectChildAccessMask = 0;
            DWORD ContainerChildAccessMask = 0;

            bool operator==(const PrincipalPermissions& other) const
            {
                return DirectAccessMask == other.DirectAccessMask &&
                    ObjectChildAccessMask == other.ObjectChildAccessMask &&
                    ContainerChildAccessMask == other.ContainerChildAccessMask;
            }
        };

        struct ExpectedACE
        {
            ACEPrincipal Principal;
            PSID SID;
        };

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

        DWORD NormalizeAccessMask(DWORD accessMask)
        {
            GENERIC_MAPPING genericMapping
            {
                FILE_GENERIC_READ,
                FILE_GENERIC_WRITE,
                FILE_GENERIC_EXECUTE,
                FILE_ALL_ACCESS,
            };

            MapGenericMask(&accessMask, &genericMapping);
            return accessMask;
        }

        std::map<ACEPrincipal, PrincipalPermissions> GetExpectedPermissions(
            const PathDetails& details,
            const ACEDetails(&aceDetails)[3],
            const std::optional<ACEPrincipal>& principalToIgnore,
            PSID& expectedOwnerSID)
        {
            std::map<ACEPrincipal, PrincipalPermissions> result;

            for (const auto& ace : aceDetails)
            {
                if (principalToIgnore && principalToIgnore.value() == ace.Principal)
                {
                    continue;
                }

                if (details.Owner && details.Owner.value() == ace.Principal)
                {
                    expectedOwnerSID = ace.SID;
                }

                auto itr = details.ACL.find(ace.Principal);
                if (itr != details.ACL.end())
                {
                    DWORD normalizedAccessMask = NormalizeAccessMask(AccessPermissionsFrom(itr->second));
                    result.emplace(ace.Principal, PrincipalPermissions
                    {
                        normalizedAccessMask,
                        normalizedAccessMask,
                        normalizedAccessMask,
                    });
                }
            }

            return result;
        }

        std::optional<std::map<ACEPrincipal, PrincipalPermissions>> GetActualPermissions(
            PACL acl,
            const std::vector<ExpectedACE>& expectedAces)
        {
            constexpr BYTE s_AllowedAceFlags = OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE | INHERIT_ONLY_ACE;

            ACL_SIZE_INFORMATION sizeInformation{};
            THROW_IF_WIN32_BOOL_FALSE(GetAclInformation(acl, &sizeInformation, sizeof(sizeInformation), AclSizeInformation));

            std::map<ACEPrincipal, PrincipalPermissions> result;

            for (DWORD i = 0; i < sizeInformation.AceCount; ++i)
            {
                void* ace = nullptr;
                THROW_IF_WIN32_BOOL_FALSE(GetAce(acl, i, &ace));

                const ACE_HEADER* aceHeader = static_cast<ACE_HEADER*>(ace);
                if (aceHeader->AceType != ACCESS_ALLOWED_ACE_TYPE || (aceHeader->AceFlags & ~s_AllowedAceFlags) != 0)
                {
                    return std::nullopt;
                }

                const ACCESS_ALLOWED_ACE* accessAllowedAce = static_cast<ACCESS_ALLOWED_ACE*>(ace);
                PSID sid = reinterpret_cast<PSID>(const_cast<DWORD*>(&accessAllowedAce->SidStart));
                auto expectedAceItr = std::find_if(expectedAces.begin(), expectedAces.end(),
                    [&](const auto& expectedAce)
                    {
                        return EqualSid(expectedAce.SID, sid);
                    });
                if (expectedAceItr == expectedAces.end())
                {
                    return std::nullopt;
                }

                if (WI_IsFlagSet(aceHeader->AceFlags, INHERIT_ONLY_ACE) &&
                    (aceHeader->AceFlags & (OBJECT_INHERIT_ACE | CONTAINER_INHERIT_ACE)) == 0)
                {
                    return std::nullopt;
                }

                PrincipalPermissions& principalPermissions = result[expectedAceItr->Principal];
                DWORD normalizedAccessMask = NormalizeAccessMask(accessAllowedAce->Mask);

                if (!WI_IsFlagSet(aceHeader->AceFlags, INHERIT_ONLY_ACE))
                {
                    principalPermissions.DirectAccessMask |= normalizedAccessMask;
                }

                if (WI_IsFlagSet(aceHeader->AceFlags, OBJECT_INHERIT_ACE))
                {
                    principalPermissions.ObjectChildAccessMask |= normalizedAccessMask;
                }

                if (WI_IsFlagSet(aceHeader->AceFlags, CONTAINER_INHERIT_ACE))
                {
                    principalPermissions.ContainerChildAccessMask |= normalizedAccessMask;
                }
            }

            return result;
        }

        std::optional<ACEPrincipal> GetPrincipalToIgnore(const PathDetails& details, const TOKEN_USER* userToken, PSID systemSID)
        {
            bool hasCurrentUser = details.ACL.count(ACEPrincipal::CurrentUser) != 0;
            bool hasSystem = details.ACL.count(ACEPrincipal::System) != 0;

            if ((hasCurrentUser && hasSystem) &&
                IsRunningAsSystem() &&
                (!details.Owner || (details.Owner.value() != ACEPrincipal::CurrentUser && details.Owner.value() != ACEPrincipal::System)))
            {
                THROW_HR(HRESULT_FROM_WIN32(ERROR_INVALID_STATE));
            }

            if (hasCurrentUser && hasSystem && EqualSid(userToken->User.Sid, systemSID))
            {
                return (details.Owner.value() == ACEPrincipal::CurrentUser ? ACEPrincipal::System : ACEPrincipal::CurrentUser);
            }

            return std::nullopt;
        }

        bool PathHasExpectedOwnerAndACLs(const PathDetails& details)
        {
            auto userToken = wil::get_token_information<TOKEN_USER>();
            auto adminSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
            auto systemSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID);
            auto principalToIgnore = GetPrincipalToIgnore(details, userToken.get(), systemSID.get());

            ACEDetails aceDetails[] =
            {
                { ACEPrincipal::CurrentUser, userToken->User.Sid, TRUSTEE_IS_USER },
                { ACEPrincipal::Admins, adminSID.get(), TRUSTEE_IS_WELL_KNOWN_GROUP },
                { ACEPrincipal::System, systemSID.get(), TRUSTEE_IS_USER },
            };

            PSID expectedOwnerSID = nullptr;
            std::map<ACEPrincipal, PrincipalPermissions> expectedPermissions = GetExpectedPermissions(details, aceDetails, principalToIgnore, expectedOwnerSID);

            SECURITY_INFORMATION securityInformation = DACL_SECURITY_INFORMATION;
            PSID ownerSID = nullptr;

            if (details.Owner)
            {
                securityInformation |= OWNER_SECURITY_INFORMATION;
                ownerSID = expectedOwnerSID;
            }

            std::wstring path = details.Path.wstring();
            wil::unique_hlocal_security_descriptor securityDescriptor;
            PSID actualOwnerSID = nullptr;
            DWORD result = GetNamedSecurityInfoW(
                &path[0],
                SE_FILE_OBJECT,
                securityInformation,
                details.Owner ? &actualOwnerSID : nullptr,
                nullptr,
                nullptr,
                nullptr,
                &securityDescriptor);

            if (result != ERROR_SUCCESS)
            {
                return false;
            }

            SECURITY_DESCRIPTOR_CONTROL control = 0;
            DWORD revision = 0;
            if (!GetSecurityDescriptorControl(securityDescriptor.get(), &control, &revision) || !WI_IsFlagSet(control, SE_DACL_PROTECTED))
            {
                return false;
            }

            BOOL daclPresent = FALSE;
            BOOL daclDefaulted = FALSE;
            PACL currentDacl = nullptr;
            if (!GetSecurityDescriptorDacl(securityDescriptor.get(), &daclPresent, &currentDacl, &daclDefaulted) || !daclPresent || !currentDacl)
            {
                return false;
            }

            if (ownerSID && (!actualOwnerSID || !EqualSid(actualOwnerSID, ownerSID)))
            {
                return false;
            }

            std::vector<ExpectedACE> expectedAces;
            for (const auto& ace : aceDetails)
            {
                if (principalToIgnore && principalToIgnore.value() == ace.Principal)
                {
                    continue;
                }

                if (details.ACL.count(ace.Principal) != 0)
                {
                    expectedAces.push_back({ ace.Principal, ace.SID });
                }
            }

            auto actualPermissions = GetActualPermissions(currentDacl, expectedAces);
            return actualPermissions && actualPermissions.value() == expectedPermissions;
        }

        // Gets the path to the appdata root.
        // *Only used by non packaged version!*
        std::filesystem::path GetPathToAppDataRoot(bool anonymize)
        {
            std::filesystem::path result = anonymize ? s_LocalAppDataEnvironmentVariable : GetKnownFolderPath(FOLDERID_LocalAppData);
            result /= "Microsoft/WinGet";

            return result;
        }

        // Gets the path to the app data relative directory.
        std::filesystem::path GetPathToAppDataDir(const std::filesystem::path& relative, bool anonymize)
        {
            THROW_HR_IF(E_INVALIDARG, !relative.has_relative_path());
            THROW_HR_IF(E_INVALIDARG, relative.has_root_path());
            THROW_HR_IF(E_INVALIDARG, !relative.has_filename());

            std::filesystem::path result = GetPathToAppDataRoot(anonymize);
            result /= relative;

            return result;
        }
    }

    DWORD GetVolumeInformationFlagsByHandle(HANDLE anyFileHandle)
    {
        DWORD flags = 0;
        wchar_t fileSystemName[MAX_PATH];
        THROW_LAST_ERROR_IF(!GetVolumeInformationByHandleW(
            anyFileHandle, /*hFile*/
            NULL, /*lpVolumeNameBuffer*/
            0, /*nVolumeNameSize*/
            NULL, /*lpVolumeSerialNumber*/
            NULL, /*lpMaximumComponentLength*/
            &flags, /*lpFileSystemFlags*/
            fileSystemName, /*lpFileSystemNameBuffer*/
            MAX_PATH /*nFileSystemNameSize*/));

        // Vista and older does not report all flags, fix them up here
        if (!(flags & FILE_SUPPORTS_HARD_LINKS) && !_wcsicmp(fileSystemName, L"NTFS"))
        {
            flags |= FILE_SUPPORTS_HARD_LINKS | FILE_SUPPORTS_EXTENDED_ATTRIBUTES | FILE_SUPPORTS_OPEN_BY_FILE_ID | FILE_SUPPORTS_USN_JOURNAL;
        }

        return flags;
    }

    DWORD GetVolumeInformationFlags(const std::filesystem::path& anyPath)
    {
        wil::unique_hfile fileHandle{ CreateFileW(
            anyPath.c_str(), /*lpFileName*/
            0, /*dwDesiredAccess*/
            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, /*dwShareMode*/
            NULL, /*lpSecurityAttributes*/
            OPEN_EXISTING, /*dwCreationDisposition*/
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_BACKUP_SEMANTICS, /*dwFlagsAndAttributes*/
            NULL /*hTemplateFile*/) };

        THROW_LAST_ERROR_IF(fileHandle.get() == INVALID_HANDLE_VALUE);

        return GetVolumeInformationFlagsByHandle(fileHandle.get());
    }

    bool SupportsNamedStreams(const std::filesystem::path& path)
    {
        return (GetVolumeInformationFlags(path) & FILE_NAMED_STREAMS) != 0;
    }

    bool SupportsHardLinks(const std::filesystem::path& path)
    {
        return (GetVolumeInformationFlags(path) & FILE_SUPPORTS_HARD_LINKS) != 0;
    }

    bool SupportsReparsePoints(const std::filesystem::path& path)
    {
        return (GetVolumeInformationFlags(path) & FILE_SUPPORTS_REPARSE_POINTS) != 0;
    }

    bool PathEscapesBaseDirectory(const std::filesystem::path& target, const std::filesystem::path& base)
    {
        const auto& targetPath = std::filesystem::weakly_canonical(target);
        const auto& basePath = std::filesystem::weakly_canonical(base);
        auto [a, b] = std::mismatch(targetPath.begin(), targetPath.end(), basePath.begin(), basePath.end());
        return (b != basePath.end());
    }

    // Complicated rename algorithm due to somewhat arbitrary failures.
    // 1. First, try to rename.
    // 2. Then, create an empty file for the target, and attempt to rename.
    // 3. Then, try repeatedly for 500ms in case it is a timing thing.
    // 4. Attempt to use a hard link if available.
    // 5. Copy the file if nothing else has worked so far.
    void RenameFile(const std::filesystem::path& from, const std::filesystem::path& to)
    {
        // 1. First, try to rename.
        try
        {
            // std::filesystem::rename() handles motw correctly if applicable.
            std::filesystem::rename(from, to);
            return;
        }
        CATCH_LOG();

        // 2. Then, create an empty file for the target, and attempt to rename.
        //    This seems to fix things in certain cases, so we do it.
        try
        {
            {
                std::ofstream targetFile{ to };
            }
            std::filesystem::rename(from, to);
            return;
        }
        CATCH_LOG();

        // 3. Then, try repeatedly for 500ms in case it is a timing thing.
        for (int i = 0; i < 5; ++i)
        {
            try
            {
                std::this_thread::sleep_for(100ms);
                std::filesystem::rename(from, to);
                return;
            }
            CATCH_LOG();
        }

        // 4. Attempt to use a hard link if available.
        if (SupportsHardLinks(from))
        {
            try
            {
                // Create a hard link to the file; the installer will be left in the temp directory afterward
                // but it is better to succeed the operation and leave a file around than to fail.
                // First we have to remove the target file as the function will not overwrite.
                std::filesystem::remove(to);
                std::filesystem::create_hard_link(from, to);
                return;
            }
            CATCH_LOG();
        }

        // 5. Copy the file if nothing else has worked so far.
        // Create a copy of the file; the installer will be left in the temp directory afterward
        // but it is better to succeed the operation and leave a file around than to fail.
        std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);
    }

#ifndef AICLI_DISABLE_TEST_HOOKS
    static bool* s_CreateSymlinkResult_TestHook_Override = nullptr;

    void TestHook_SetCreateSymlinkResult_Override(bool* status)
    {
        s_CreateSymlinkResult_TestHook_Override = status;
    }
#endif

    bool CreateSymlink(const std::filesystem::path& target, const std::filesystem::path& link)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        if (s_CreateSymlinkResult_TestHook_Override)
        {
            return *s_CreateSymlinkResult_TestHook_Override;
        }
#endif
        try
        {
            std::filesystem::create_symlink(target, link);
            return true;
        }
        catch (std::filesystem::filesystem_error& error)
        {
            if (error.code().value() == ERROR_PRIVILEGE_NOT_HELD)
            {
                return false;
            }
            else
            {
                throw;
            }
        }
    }

    bool VerifySymlink(const std::filesystem::path& symlink, const std::filesystem::path& target)
    {
        const std::filesystem::path& symlinkTargetPath = std::filesystem::weakly_canonical(symlink);
        return symlinkTargetPath == std::filesystem::weakly_canonical(target);
    }

    void AppendExtension(std::filesystem::path& target, const std::string& value)
    {
        if (target.extension() != value)
        {
            target += value;
        }
    }

    bool SymlinkExists(const std::filesystem::path& symlinkPath)
    {
        return std::filesystem::is_symlink(std::filesystem::symlink_status(symlinkPath));
    }

    std::filesystem::path GetExpandedPath(const std::string& path)
    {
        std::string trimPath = path;
        Utility::Trim(trimPath);

        try
        {
            return std::filesystem::weakly_canonical(Utility::ExpandEnvironmentVariables(Utility::ConvertToUTF16(trimPath)));
        }
        catch (...)
        {
            return Utility::ConvertToUTF16(path);
        }
    }

    bool ReplaceCommonPathPrefix(std::filesystem::path& source, const std::filesystem::path& prefix, std::string_view replacement)
    {
        auto prefixItr = prefix.begin();
        auto sourceItr = source.begin();

        while (prefixItr != prefix.end() && sourceItr != source.end())
        {
            if (!Utility::ICUCaseInsensitiveEquals(prefixItr->u8string(), sourceItr->u8string()))
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

            return true;
        }

        return false;
    }

    std::filesystem::path GetKnownFolderPath(const KNOWNFOLDERID& id)
    {
        wil::unique_cotaskmem_string knownFolder = nullptr;
        THROW_IF_FAILED(SHGetKnownFolderPath(id, KF_FLAG_NO_ALIAS | KF_FLAG_DONT_VERIFY | KF_FLAG_NO_PACKAGE_REDIRECTION, NULL, &knownFolder));
        return knownFolder.get();
    }

    bool IsSameVolume(const std::filesystem::path& path1, const std::filesystem::path& path2)
    {
        WCHAR volumeName1[MAX_PATH];
        WCHAR volumeName2[MAX_PATH];

        // Note: GetVolumePathNameW will return false if the volume drive does not exist.
        if (!GetVolumePathNameW(path1.c_str(), volumeName1, MAX_PATH) || !GetVolumePathNameW(path2.c_str(), volumeName2, MAX_PATH))
        {
            return false;
        }
        return Utility::ICUCaseInsensitiveEquals(Utility::ConvertToUTF8(volumeName1), Utility::ConvertToUTF8(volumeName2));
    }

    bool IsParentPath(const std::filesystem::path& path, const std::filesystem::path& parentPath)
    {
        return std::filesystem::weakly_canonical(path.parent_path()) == std::filesystem::weakly_canonical(parentPath);
    }

    void PathDetails::SetOwner(ACEPrincipal owner)
    {
        Owner = owner;
        ACL[owner] = ACEPermissions::All;
    }

    bool PathDetails::ShouldApplyACL() const
    {
        if (ACL.empty())
        {
            return false;
        }

        try
        {
            return !anon::PathHasExpectedOwnerAndACLs(*this);
        }
        catch (...)
        {
            LOG_CAUGHT_EXCEPTION_MSG("Failed to inspect ACL state; reapplying ACLs to preserve security");
            return true;
        }
    }

    void PathDetails::ApplyACL() const
    {
        auto userToken = wil::get_token_information<TOKEN_USER>();
        auto adminSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS);
        auto systemSID = wil::make_static_sid(SECURITY_NT_AUTHORITY, SECURITY_LOCAL_SYSTEM_RID);
        PSID ownerSID = nullptr;

        anon::ACEDetails aceDetails[] =
        {
            { ACEPrincipal::CurrentUser, userToken->User.Sid, TRUSTEE_IS_USER },
            { ACEPrincipal::Admins, adminSID.get(), TRUSTEE_IS_WELL_KNOWN_GROUP},
            { ACEPrincipal::System, systemSID.get(), TRUSTEE_IS_USER},
        };

        ULONG entriesCount = 0;
        std::array<EXPLICIT_ACCESS_W, ARRAYSIZE(aceDetails)> explicitAccess;

        // If the current user is SYSTEM, we want to take either the owner or the only configured set of permissions.
        // The check above should prevent us from getting into situations outside of the ones below.
        std::optional<ACEPrincipal> principalToIgnore = anon::GetPrincipalToIgnore(*this, userToken.get(), systemSID.get());

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

                entry.grfAccessPermissions = anon::AccessPermissionsFrom(itr->second);
                entry.grfAccessMode = SET_ACCESS;
                entry.grfInheritance = anon::s_InheritableAceFlags;

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

        DWORD result = SetNamedSecurityInfoW(&path[0], SE_FILE_OBJECT, securityInformation, ownerSID, nullptr, acl.get(), nullptr);

        // We can be denied access attempting to set the owner when the owner is already correct.
        // Determine if the owner is correct; if so, try again without attempting to set the owner.
        if (result == ERROR_ACCESS_DENIED && ownerSID)
        {
            wil::unique_hlocal_security_descriptor securityDescriptor;
            PSID currentOwnerSID = nullptr;
            DWORD getResult = GetNamedSecurityInfoW(&path[0], SE_FILE_OBJECT, OWNER_SECURITY_INFORMATION, &currentOwnerSID, nullptr, nullptr, nullptr, &securityDescriptor);

            if (SUCCEEDED_WIN32_LOG(getResult) && currentOwnerSID && EqualSid(currentOwnerSID, ownerSID))
            {
                result = SetNamedSecurityInfoW(&path[0], SE_FILE_OBJECT, securityInformation & ~OWNER_SECURITY_INFORMATION, nullptr, nullptr, acl.get(), nullptr);
            }
        }

        THROW_IF_WIN32_ERROR(result);
    }

    std::filesystem::path InitializeAndGetPathTo(PathDetails&& details)
    {
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
                AICLI_LOG(Core, Warning, << "InitializeAndGetPathTo directory creation requested for path that was not absolute: " << details.Path);
            }
        }

        return std::move(details.Path);
    }

    PathDetails GetPathDetailsFor(PathName path, bool forDisplay)
    {
        PathDetails result;
        // We should not create directories by default when they are retrieved for display purposes.
        result.Create = !forDisplay;

        switch (path)
        {
        case PathName::UnpackagedLocalStateRoot:
            result.Path = anon::GetPathToAppDataDir(anon::s_AppDataDir_State, forDisplay);
            result.SetOwner(ACEPrincipal::CurrentUser);
            result.ACL[ACEPrincipal::System] = ACEPermissions::All;
            result.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
            break;
        case PathName::UnpackagedSettingsRoot:
            result.Path = anon::GetPathToAppDataDir(anon::s_AppDataDir_Settings, forDisplay);
            result.SetOwner(ACEPrincipal::CurrentUser);
            result.ACL[ACEPrincipal::System] = ACEPermissions::All;
            result.ACL[ACEPrincipal::Admins] = ACEPermissions::All;
            break;
        default:
            THROW_HR(E_UNEXPECTED);
        }

        return result;
    }

    std::filesystem::path GetExecutablePathForProcess(HANDLE process)
    {
        wil::unique_cotaskmem_string imageName = nullptr;
        if (SUCCEEDED(wil::QueryFullProcessImageNameW(process, 0, imageName)) &&
            (imageName.get() != nullptr))
        {
            return imageName.get();
        }

        return {};
    }

    std::vector<FileInfo> GetFileInfoFor(const std::filesystem::path& directory)
    {
        std::vector<FileInfo> result;

        for (const auto& file : std::filesystem::directory_iterator{ directory })
        {
            if (file.is_regular_file())
            {
                result.emplace_back(FileInfo{ file.path(), file.last_write_time(), file.file_size() });
            }
        }

        return result;
    }

    void FilterToFilesExceedingLimits(std::vector<FileInfo>& files, const FileLimits& limits)
    {
        auto now = std::filesystem::file_time_type::clock::now();
        std::chrono::hours ageLimit = limits.Age;
        static_assert(sizeof(uintmax_t) >= 8);
        uintmax_t totalSizeLimit = static_cast<uintmax_t>(limits.TotalSizeInMB) << 20;
        size_t countLimit = limits.Count;

        // Sort with oldest first so that we can work backward to find the cutoff
        std::sort(files.begin(), files.end(), [](const FileInfo& a, const FileInfo& b) { return a.LastWriteTime < b.LastWriteTime; });

        // Walk the list backward until we find the first entry that goes over one of the limits
        size_t i = files.size();
        uintmax_t totalSize = 0;
        for (; i > 0; --i)
        {
            const FileInfo& current = files[i - 1];

            if (totalSizeLimit != 0)
            {
                totalSize += current.Size;
                if (totalSize > totalSizeLimit)
                {
                    break;
                }
            }

            if (countLimit != 0 && (files.size() - i + 1) > countLimit)
            {
                break;
            }

            if (ageLimit != 0h && now - current.LastWriteTime > ageLimit)
            {
                break;
            }
        }

        files.resize(i);
    }

    void WriteStringToFile(HANDLE fileHandle, std::string_view content)
    {
        size_t totalBytesWritten = 0;
        while (totalBytesWritten < content.size())
        {
            DWORD bytesWritten = 0;
            THROW_LAST_ERROR_IF(!WriteFile(
                fileHandle,
                content.data() + totalBytesWritten,
                static_cast<DWORD>(content.size() - totalBytesWritten),
                &bytesWritten,
                nullptr));
            totalBytesWritten += bytesWritten;
        }
    }
}
