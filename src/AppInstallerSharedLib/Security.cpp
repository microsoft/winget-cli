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

        // Helper to impersonate the COM or RPC caller.
        struct ImpersonateCOMorRPCCaller
        {
            static ImpersonateCOMorRPCCaller BeginImpersonation()
            {
                return {};
            }

            ~ImpersonateCOMorRPCCaller()
            {
                if (m_serverSecurity)
                {
                    FAIL_FAST_IF_FAILED(m_serverSecurity->RevertToSelf());
                }
                else
                {
                    FAIL_FAST_IF(RpcRevertToSelf() != RPC_S_OK);
                }
            }

        private:
            ImpersonateCOMorRPCCaller()
            {
                if (SUCCEEDED_LOG(CoGetCallContext(IID_IServerSecurity, m_serverSecurity.put_void())))
                {
                    THROW_IF_FAILED(m_serverSecurity->ImpersonateClient());
                }
                else
                {
                    RPC_STATUS status = RpcImpersonateClient(nullptr);
                    THROW_HR_IF(MAKE_HRESULT(SEVERITY_ERROR, FACILITY_RPC, status), status != RPC_S_OK);
                }
            }

            wil::com_ptr<IServerSecurity> m_serverSecurity;
        };
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

    bool IsCOMCallerSameUserAndIntegrityLevel()
    {
        auto serverUser = wil::get_token_information<TOKEN_USER>();
        IntegrityLevel serverIntegrityLevel = GetEffectiveIntegrityLevel();

        auto impersonation = ImpersonateCOMorRPCCaller::BeginImpersonation();

        auto callingUser = wil::get_token_information<TOKEN_USER>();
        IntegrityLevel callingIntegrityLevel = GetEffectiveIntegrityLevel();

        if (!EqualSid(serverUser->User.Sid, callingUser->User.Sid))
        {
            AICLI_LOG(Core, Crit, << "Attempt to access by another user: " << ToString(callingUser->User.Sid));
            return false;
        }

        if (ToIntegral(callingIntegrityLevel) < ToIntegral(serverIntegrityLevel))
        {
            AICLI_LOG(Core, Crit, << "Attempt to access by a lower integrity process: " << callingIntegrityLevel << " < " << serverIntegrityLevel);
            return false;
        }

        return true;
    }

    bool IsCOMCallerIntegrityLevelAtLeast(IntegrityLevel minimumLevel)
    {
        auto impersonation = ImpersonateCOMorRPCCaller::BeginImpersonation();
        return IsCurrentIntegrityLevelAtLeast(minimumLevel);
    }

    bool IsCurrentIntegrityLevelAtLeast(IntegrityLevel minimumLevel)
    {
        IntegrityLevel callingIntegrityLevel = GetEffectiveIntegrityLevel();

        if (ToIntegral(callingIntegrityLevel) < ToIntegral(minimumLevel))
        {
            AICLI_LOG(Core, Crit, << "Attempt to access by a lower integrity process than required: " << callingIntegrityLevel << " < " << minimumLevel);
            return false;
        }

        return true;
    }

    std::string ToString(PSID sid)
    {
        wil::unique_hlocal_ansistring result;
        THROW_IF_WIN32_BOOL_FALSE(ConvertSidToStringSidA(sid, &result));
        return result.get();
    }
}
