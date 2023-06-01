// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/SelfManagement.h"
#include "AppInstallerRuntime.h"

namespace AppInstaller::SelfManagement
{
    using namespace std::string_view_literals;
    using namespace winrt::Windows::Management::Deployment;
    using namespace winrt::Windows::Services::Store;

    // Always use AppInstaller's package family name even for wingetdev
    static constexpr std::wstring_view s_AppInstallerPfn = L"Microsoft.DesktopAppInstaller_8wekyb3d8bbwe"sv;

    bool IsStubPreferenceSupported()
    {
        if (!Runtime::IsRunningInPackagedContext())
        {
            return false;
        }

        PackageManager packageManager;
        auto packageManager9 = packageManager.try_as<IPackageManager9>();

        if (!packageManager9)
        {
            return false;
        }

        return true;
    }

    bool IsStubPreferred()
    {
        winrt::hstring packageFamilyName{ s_AppInstallerPfn };
        if (packageFamilyName.empty())
        {
            return false;
        }

        PackageManager packageManager;
        auto packageManager9 = packageManager.try_as<IPackageManager9>();

        if (!packageManager9)
        {
            // If the API isn't present, then the only option is full package.
            return false;
        }

        auto preference = packageManager9.GetPackageStubPreference(packageFamilyName);

        return preference == PackageStubPreference::Stub;
    }

    void SetStubPreferred(bool preferStub)
    {
        winrt::hstring packageFamilyName{ s_AppInstallerPfn };
        THROW_HR_IF(HRESULT_FROM_WIN32(APPMODEL_ERROR_NO_PACKAGE), packageFamilyName.empty());

        PackageManager packageManager;
        auto packageManager9 = packageManager.try_as<IPackageManager9>();

        THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_OLD_WIN_VERSION), !packageManager9);

        packageManager9.SetPackageStubPreference(packageFamilyName, preferStub ? PackageStubPreference::Stub : PackageStubPreference::Full);
    }
}
