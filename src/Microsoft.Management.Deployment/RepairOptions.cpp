// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
// 6388 Allow CreateInstance.
#include <wil\cppwinrt_wrl.h>
// 4467 Allow use of uuid attribute for com object creation.
#include "RepairOptions.h"
#pragma warning( pop )
#include "RepairOptions.g.cpp"
#include "Converters.h"
#include "Helpers.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    RepairOptions::RepairOptions()
    {
    }

    winrt::Microsoft::Management::Deployment::PackageVersionId RepairOptions::PackageVersionId()
    {
        return m_packageVersionId;
    }
    void RepairOptions::PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const& value)
    {
        m_packageVersionId = value;
    }

    winrt::Microsoft::Management::Deployment::PackageRepairScope RepairOptions::PackageRepairScope()
    {
        return m_packageRepairScope;
    }

    void RepairOptions::PackageRepairScope(winrt::Microsoft::Management::Deployment::PackageRepairScope const& value)
    {
        m_packageRepairScope = value;
    }

    winrt::Microsoft::Management::Deployment::PackageRepairMode RepairOptions::PackageRepairMode()
    {
        return m_packageRepairMode;
    }

    void RepairOptions::PackageRepairMode(winrt::Microsoft::Management::Deployment::PackageRepairMode const& value)
    {
        m_packageRepairMode = value;
    }

    bool RepairOptions::AcceptPackageAgreements()
    {
        return m_acceptPackageAgreements;
    }

    void RepairOptions::AcceptPackageAgreements(bool value)
    {
        m_acceptPackageAgreements = value;
    }

    hstring RepairOptions::LogOutputPath()
    {
        return hstring(m_logOutputPath);
    }

    void RepairOptions::LogOutputPath(hstring const& value)
    {
        m_logOutputPath = value;
    }

    hstring RepairOptions::CorrelationData()
    {
        return hstring(m_correlationData);
    }

    void RepairOptions::CorrelationData(hstring const& value)
    {
        m_correlationData = value;
    }

    bool RepairOptions::AllowHashMismatch()
    {
        return m_allowHashMismatch;
    }

    void RepairOptions::AllowHashMismatch(bool value)
    {
        m_allowHashMismatch = value;
    }

    bool RepairOptions::BypassIsStoreClientBlockedPolicyCheck()
    {
        return m_bypassIsStoreClientBlockedPolicyCheck;
    }

    void RepairOptions::BypassIsStoreClientBlockedPolicyCheck(bool value)
    {
        m_bypassIsStoreClientBlockedPolicyCheck = value;
    }

    bool RepairOptions::Force()
    {
        return m_force;
    }

    void RepairOptions::Force(bool value)
    {
        m_force = value;
    }

    winrt::Microsoft::Management::Deployment::AuthenticationArguments RepairOptions::AuthenticationArguments()
    {
        return m_authenticationArguments;
    }

    void RepairOptions::AuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments const& value)
    {
        m_authenticationArguments = value;
    }

    CoCreatableMicrosoftManagementDeploymentClass(RepairOptions);
}
