// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CatalogPackage.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    // This local-only interface must not be added to WinMD or registered for marshaling.
    MIDL_INTERFACE("6ab6bdfb-29fb-48dc-8a40-f92a30e4d1dc")
    ICatalogPackageInternal : public ::IUnknown
    {
        virtual HRESULT STDMETHODCALLTYPE GetRepositoryPackage(
            std::shared_ptr<::AppInstaller::Repository::ICompositePackage>* package) noexcept = 0;
    };

    struct CatalogPackage : CatalogPackageT<CatalogPackage, winrt::cloaked<ICatalogPackageInternal>>
#else
    struct CatalogPackage : CatalogPackageT<CatalogPackage>
#endif
    {
        CatalogPackage() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(
            ::AppInstaller::Repository::Source source,
            std::shared_ptr<::AppInstaller::Repository::ICompositePackage> package);
        static std::shared_ptr<::AppInstaller::Repository::ICompositePackage> GetRepositoryPackage(
            winrt::Microsoft::Management::Deployment::CatalogPackage const& catalogPackage);
        HRESULT STDMETHODCALLTYPE GetRepositoryPackage(
            std::shared_ptr<::AppInstaller::Repository::ICompositePackage>* package) noexcept override;
#endif

        hstring Id();
        hstring Name();
        winrt::Microsoft::Management::Deployment::PackageVersionInfo InstalledVersion();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageVersionId> AvailableVersions();
        winrt::Microsoft::Management::Deployment::PackageVersionInfo DefaultInstallVersion();
        winrt::Microsoft::Management::Deployment::PackageVersionInfo GetPackageVersionInfo(winrt::Microsoft::Management::Deployment::PackageVersionId const& versionKey);
        bool IsUpdateAvailable();
        // Contract 5.0
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::CheckInstalledStatusResult> CheckInstalledStatusAsync(
            winrt::Microsoft::Management::Deployment::InstalledStatusType checkTypes);
        winrt::Microsoft::Management::Deployment::CheckInstalledStatusResult CheckInstalledStatus(
            winrt::Microsoft::Management::Deployment::InstalledStatusType checkTypes);
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::CheckInstalledStatusResult> CheckInstalledStatusAsync();
        winrt::Microsoft::Management::Deployment::CheckInstalledStatusResult CheckInstalledStatus();

        Windows::Foundation::IReference<int32_t> CatalogPriority();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Repository::Source m_source;
        std::shared_ptr<::AppInstaller::Repository::ICompositePackage> m_package;
        bool m_updateAvailable = false;
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageVersionId> m_availableVersions{ winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageVersionId>() };
        winrt::Microsoft::Management::Deployment::PackageVersionInfo m_installedVersion{ nullptr };
        winrt::Microsoft::Management::Deployment::PackageVersionInfo m_latestApplicableVersion{ nullptr };
        std::once_flag m_installedVersionOnceFlag;
        std::once_flag m_availableVersionsOnceFlag;
        std::once_flag m_latestApplicableVersionOnceFlag;

        void InitializeLatestApplicableVersion();
#endif
    };
}
