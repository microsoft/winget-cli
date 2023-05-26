// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Security.h"

namespace AppInstaller::Security
{
    IntegrityLevel GetEffectiveIntegrityLevel()
    {
        auto currentIntegrityLevel = wil::get_token_information<TOKEN_MANDATORY_LABEL>();
        PSID sid = currentIntegrityLevel->Label.Sid;
        THROW_HR_IF(CO_E_INVALIDSID, !IsValidSid(sid));

        // TODO: Check that sid has SECURITY_MANDATORY_LABEL_AUTHORITY
        auto identifierAuthority = GetSidIdentifierAuthority(sid);

        // TODO: Check that there is 1 subauthority

        // TODO: Compare subauthority against
        // SECURITY_MANDATORY_UNTRUSTED_RID        
        // SECURITY_MANDATORY_LOW_RID              
        // SECURITY_MANDATORY_MEDIUM_RID           
        // SECURITY_MANDATORY_HIGH_RID             
        // SECURITY_MANDATORY_SYSTEM_RID           
        // SECURITY_MANDATORY_PROTECTED_PROCESS_RID

        std::string sid = ToString(currentIntegrityLevel->Label.Sid);
        THROW_HR(E_NOTIMPL);
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
        stream << L"S:(" SDDL_MANDATORY_LABEL ";;" << mandatoryLabelRight << ";;;" << ToWString(minimumIntegrityLevel) << ")"
            "D:(A;;GA;;;" << currentUserSID << ")";

        wil::unique_hlocal_security_descriptor securityDescriptor;
        THROW_IF_WIN32_BOOL_FALSE(ConvertStringSecurityDescriptorToSecurityDescriptorW(stream.str().c_str(), SDDL_REVISION_1, &securityDescriptor, nullptr));

        return securityDescriptor;
    }

    bool IsCOMCallerSameUserAndIntegrityLevel()
    {
        auto securityDescriptor = CreateSecurityDescriptorForCurrentUserWithMandatoryLabel(SDDL_NO_EXECUTE_UP, GetEffectiveIntegrityLevel());

        // Impersonate caller token
        wil::com_ptr<IServerSecurity> serverSecurity;
        THROW_IF_FAILED(CoGetCallContext(IID_IServerSecurity, serverSecurity.put_void()));
        THROW_IF_FAILED(serverSecurity->ImpersonateClient());

        // Make up our own generic access bits
        GENERIC_MAPPING genericMapping{};
        genericMapping.GenericExecute = 0x1;
        genericMapping.GenericRead = 0x2;
        genericMapping.GenericWrite = 0x4;
        genericMapping.GenericAll = genericMapping.GenericExecute | genericMapping.GenericRead | genericMapping.GenericWrite;

        PRIVILEGE_SET privilegeSet{};
        DWORD privilegeSetSize = sizeof(privilegeSet);

        DWORD grantedAccess = 0;
        BOOL checkResult = FALSE;

        THROW_IF_WIN32_BOOL_FALSE(AccessCheck(securityDescriptor.get(), GetCurrentThreadEffectiveToken(), genericMapping.GenericExecute, &genericMapping, &privilegeSet, &privilegeSetSize, &grantedAccess, &checkResult));

        return checkResult;
    }

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
