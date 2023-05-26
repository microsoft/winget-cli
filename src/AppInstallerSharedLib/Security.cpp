// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Security.h"
#include "AppInstallerLogging.h"
#include "AppInstallerLanguageUtilities.h"

namespace AppInstaller::Security
{
    namespace
    {
        bool IsSameAuthority(const SID_IDENTIFIER_AUTHORITY& a, const SID_IDENTIFIER_AUTHORITY& b)
        {
            for (size_t i = 0; i < ARRAYSIZE(a.Value); ++i)
            {
                if (a.Value[i] != b.Value[i])
                {
                    return false;
                }
            }

            return true;
        }
    }

    IntegrityLevel GetEffectiveIntegrityLevel()
    {
        auto currentIntegrityLevel = wil::get_token_information<TOKEN_MANDATORY_LABEL>();
        PSID sid = currentIntegrityLevel->Label.Sid;
        THROW_HR_IF(CO_E_INVALIDSID, !IsValidSid(sid));

        auto identifierAuthority = GetSidIdentifierAuthority(sid);
        THROW_HR_IF(E_UNEXPECTED, !IsSameAuthority(*identifierAuthority, SECURITY_MANDATORY_LABEL_AUTHORITY));

        PUCHAR subAuthorityCount = GetSidSubAuthorityCount(sid);
        THROW_HR_IF(E_UNEXPECTED, *subAuthorityCount != 1);

        PDWORD subAuthority = GetSidSubAuthority(sid, 0);

        switch (*subAuthority)
        {
        case SECURITY_MANDATORY_UNTRUSTED_RID: return IntegrityLevel::Untrusted;
        case SECURITY_MANDATORY_LOW_RID: return IntegrityLevel::Low;
        case SECURITY_MANDATORY_MEDIUM_RID: return IntegrityLevel::Medium;
        case SECURITY_MANDATORY_HIGH_RID: return IntegrityLevel::High;
        case SECURITY_MANDATORY_SYSTEM_RID: return IntegrityLevel::System;
        case SECURITY_MANDATORY_PROTECTED_PROCESS_RID: return IntegrityLevel::ProtectedProcess;
        }

        THROW_HR(E_UNEXPECTED);
    }

    wil::unique_hlocal_security_descriptor CreateSecurityDescriptorForCurrentUserWithMandatoryLabel(
        std::wstring_view mandatoryLabelRight,
        IntegrityLevel minimumIntegrityLevel)
    {
        auto currentUser = wil::get_token_information<TOKEN_USER>();
        std::wstring currentUserSID = ToWString(currentUser->User.Sid);

        // (ML;;NW;;;HI) specifies a high mandatory integrity level (requires admin to write).
        // (A;;GA;;;UserSID) specifies access only for the user with the user SID (i.e. self).
        std::wostringstream stream;
        stream << L"S:(" SDDL_MANDATORY_LABEL L";;" << mandatoryLabelRight << L";;;" << ToWString(minimumIntegrityLevel) << L")"
            L"D:(A;;GA;;;" << currentUserSID << L")";

        wil::unique_hlocal_security_descriptor securityDescriptor;
        THROW_IF_WIN32_BOOL_FALSE(ConvertStringSecurityDescriptorToSecurityDescriptorW(stream.str().c_str(), SDDL_REVISION_1, &securityDescriptor, nullptr));

        return securityDescriptor;
    }

    bool IsCOMCallerSameUserAndIntegrityLevel()
    {
        auto serverUser = wil::get_token_information<TOKEN_USER>();
        IntegrityLevel serverIntegrityLevel = GetEffectiveIntegrityLevel();

        // Impersonate caller token
        // TODO: Handle RPC only caller as well
        wil::com_ptr<IServerSecurity> serverSecurity;
        THROW_IF_FAILED(CoGetCallContext(IID_IServerSecurity, serverSecurity.put_void()));
        THROW_IF_FAILED(serverSecurity->ImpersonateClient());
        auto stopImpersonation = wil::scope_exit([&]() { FAIL_FAST_IF_FAILED(serverSecurity->RevertToSelf()); });

        auto callingUser = wil::get_token_information<TOKEN_USER>();
        IntegrityLevel callingIntegrityLevel = GetEffectiveIntegrityLevel();

        if (!EqualSid(serverUser->User.Sid, callingUser->User.Sid))
        {
            AICLI_LOG(Core, Crit, << "Attempt to access by another user: " << ToString(callingUser->User.Sid));
            return false;
        }

        if (ToIntegral(callingIntegrityLevel) < ToIntegral(serverIntegrityLevel))
        {
            AICLI_LOG(Core, Crit, << "Attempt to access by a lower integrity process: " << callingIntegrityLevel);
            return false;
        }

        return true;
    }

    // TODO: Figure out how to make AccessCheck happy or remove.
    //bool IsCOMCallerSameUserAndIntegrityLevel()
    //{
    //    auto securityDescriptor = CreateSecurityDescriptorForCurrentUserWithMandatoryLabel(SDDL_NO_EXECUTE_UP, GetEffectiveIntegrityLevel());

    //    // Impersonate caller token
    //    wil::com_ptr<IServerSecurity> serverSecurity;
    //    THROW_IF_FAILED(CoGetCallContext(IID_IServerSecurity, serverSecurity.put_void()));
    //    THROW_IF_FAILED(serverSecurity->ImpersonateClient());

    //    // Make up our own generic access bits
    //    GENERIC_MAPPING genericMapping{};
    //    genericMapping.GenericExecute = 0x1;
    //    genericMapping.GenericRead = 0x2;
    //    genericMapping.GenericWrite = 0x4;
    //    genericMapping.GenericAll = genericMapping.GenericExecute | genericMapping.GenericRead | genericMapping.GenericWrite;

    //    PRIVILEGE_SET privilegeSet{};
    //    DWORD privilegeSetSize = sizeof(privilegeSet);

    //    DWORD grantedAccess = 0;
    //    BOOL checkResult = FALSE;

    //    THROW_IF_WIN32_BOOL_FALSE(AccessCheck(securityDescriptor.get(), GetCurrentThreadEffectiveToken(), genericMapping.GenericExecute, &genericMapping, &privilegeSet, &privilegeSetSize, &grantedAccess, &checkResult));

    //    return checkResult;
    //}

    std::string ToString(PSID sid)
    {
        wil::unique_hlocal_ansistring result;
        THROW_IF_WIN32_BOOL_FALSE(ConvertSidToStringSidA(sid, &result));
        return result.get();
    }

    std::wstring ToWString(PSID sid)
    {
        wil::unique_hlocal_string result;
        THROW_IF_WIN32_BOOL_FALSE(ConvertSidToStringSidW(sid, &result));
        return result.get();
    }

    std::wstring_view ToWString(IntegrityLevel integrityLevel)
    {
        switch (integrityLevel)
        {
        case IntegrityLevel::Low:
            return SDDL_ML_LOW;
        case IntegrityLevel::Medium:
            return SDDL_ML_MEDIUM;
        case IntegrityLevel::MediumPlus:
            return SDDL_ML_MEDIUM_PLUS;
        case IntegrityLevel::High:
            return SDDL_ML_HIGH;
        case IntegrityLevel::System:
            return SDDL_ML_SYSTEM;
        }

        THROW_HR(E_UNEXPECTED);
    }
}
