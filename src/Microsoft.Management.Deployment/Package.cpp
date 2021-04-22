#include "pch.h"
#include "Package.h"
#include "Package.g.cpp"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    Package::Package(hstring const& manifestPath)
    {
        throw hresult_not_implemented();
    }
    hstring Package::Id()
    {
        throw hresult_not_implemented();
    }
    hstring Package::Name()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::PackageVersion Package::InstalledVersion()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionId> Package::AvailableVersions()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::PackageVersion Package::LatestAvailableVersion()
    {
        throw hresult_not_implemented();
    }
    Microsoft::Management::Deployment::PackageVersion Package::GetAvailableVersion(Microsoft::Management::Deployment::PackageVersionId const& versionKey)
    {
        throw hresult_not_implemented();
    }
    bool Package::IsUpdateAvailable()
    {
        throw hresult_not_implemented();
    }
    bool Package::IsInstalling()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress> Package::InstallProgress()
    {
        throw hresult_not_implemented();
    }
}
