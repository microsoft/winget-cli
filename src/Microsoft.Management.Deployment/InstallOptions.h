#pragma once
#include "InstallOptions.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallOptions : InstallOptionsT<InstallOptions>
    {
        InstallOptions() = default;

        Microsoft::Management::Deployment::CatalogPackage CatalogPackage();
        void CatalogPackage(Microsoft::Management::Deployment::CatalogPackage const& value);
        Microsoft::Management::Deployment::PackageVersionId PackageVersionId();
        void PackageVersionId(Microsoft::Management::Deployment::PackageVersionId const& value);
        hstring PreferredInstallLocation();
        void PreferredInstallLocation(hstring const& value);
        Microsoft::Management::Deployment::AppInstallScope AppInstallScope();
        void AppInstallScope(Microsoft::Management::Deployment::AppInstallScope const& value);
        Microsoft::Management::Deployment::AppInstallMode AppInstallMode();
        void AppInstallMode(Microsoft::Management::Deployment::AppInstallMode const& value);
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
