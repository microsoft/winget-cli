#pragma once
#include "Package.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct Package : PackageT<Package>
    {
        Package() = default;

        Package(hstring const& manifestPath);
        hstring Id();
        hstring Name();
        Microsoft::Management::Deployment::PackageVersion InstalledVersion();
        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionId> AvailableVersions();
        Microsoft::Management::Deployment::PackageVersion LatestAvailableVersion();
        Microsoft::Management::Deployment::PackageVersion GetAvailableVersion(Microsoft::Management::Deployment::PackageVersionId const& versionKey);
        bool IsUpdateAvailable();
        bool IsInstalling();
        Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> InstallProgress();
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct Package : PackageT<Package, implementation::Package>
    {
    };
}
