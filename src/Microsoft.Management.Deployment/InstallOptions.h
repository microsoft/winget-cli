#pragma once
#include "InstallOptions.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions>
    {
        InstallOptions() = default;

        Microsoft::Management::Deployment::Package Package();
        void Package(Microsoft::Management::Deployment::Package const& value);
        Microsoft::Management::Deployment::PackageVersionId PackageVersionId();
        void PackageVersionId(Microsoft::Management::Deployment::PackageVersionId const& value);
        hstring PreferredInstallLocation();
        void PreferredInstallLocation(hstring const& value);
        Microsoft::Management::Deployment::InstallScope InstallScope();
        void InstallScope(Microsoft::Management::Deployment::InstallScope const& value);
        Microsoft::Management::Deployment::InstallMode InstallMode();
        void InstallMode(Microsoft::Management::Deployment::InstallMode const& value);
        hstring LogOutputPath();
        void LogOutputPath(hstring const& value);
        bool AllowHashMismatch();
        void AllowHashMismatch(bool value);
        hstring ReplacementInstallerArguments();
        void ReplacementInstallerArguments(hstring const& value);
        hstring AdditionalTelemetryArguments();
        void AdditionalTelemetryArguments(hstring const& value);
        hstring AdditionalAppCatalogArguments();
        void AdditionalAppCatalogArguments(hstring const& value);
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions, implementation::InstallOptions>
    {
    };
}
