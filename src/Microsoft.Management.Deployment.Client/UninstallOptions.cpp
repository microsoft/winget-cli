// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
#include <UninstallOptions.h>
#include <Client.UninstallOptions.h>
#pragma warning( pop )
#include "UninstallOptions.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    UninstallOptions::UninstallOptions()
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageVersionId UninstallOptions::PackageVersionId()
    {
        throw hresult_not_implemented();
    }
    void UninstallOptions::PackageVersionId(winrt::Microsoft::Management::Deployment::PackageVersionId const&)
    {
        throw hresult_not_implemented();
    }
    winrt::Microsoft::Management::Deployment::PackageUninstallMode UninstallOptions::PackageUninstallMode()
    {
        throw hresult_not_implemented();
    }
    void UninstallOptions::PackageUninstallMode(winrt::Microsoft::Management::Deployment::PackageUninstallMode const&)
    {
        throw hresult_not_implemented();
    }
    hstring UninstallOptions::LogOutputPath()
    {
        throw hresult_not_implemented();
    }
    void UninstallOptions::LogOutputPath(hstring const&)
    {
        throw hresult_not_implemented();
    }
    hstring UninstallOptions::CorrelationData()
    {
        throw hresult_not_implemented();
    }
    void UninstallOptions::CorrelationData(hstring const&)
    {
        throw hresult_not_implemented();
    }
}
