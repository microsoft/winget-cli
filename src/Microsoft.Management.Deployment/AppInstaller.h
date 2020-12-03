#pragma once
#include "AppInstaller.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct AppInstaller : AppInstallerT<AppInstaller>
    {
        AppInstaller() = default;

        Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> InstallPackageAsync(Microsoft::Management::Deployment::InstallOptions options);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct AppInstaller : AppInstallerT<AppInstaller, implementation::AppInstaller>
    {
    };
}
