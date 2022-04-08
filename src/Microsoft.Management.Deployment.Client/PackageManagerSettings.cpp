// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#pragma warning( push )
#pragma warning ( disable : 4467 6388)
#include <PackageManagerSettings.h>
#include <Client.PackageManagerSettings.h>
#pragma warning( pop )
#include "PackageManagerSettings.g.cpp"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    bool PackageManagerSettings::SetCallerIdentifier(hstring const&)
    {
        throw hresult_not_implemented();
    }
    bool PackageManagerSettings::SetStateIdentifier(hstring const&)
    {
        throw hresult_not_implemented();
    }
    bool PackageManagerSettings::SetUserSettings(hstring const&)
    {
        throw hresult_not_implemented();
    }
}
