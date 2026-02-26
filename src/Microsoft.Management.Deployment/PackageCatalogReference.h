// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageCatalogReference.g.h"
#include <optional>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageCatalogReference : PackageCatalogReferenceT<PackageCatalogReference>
    {
        PackageCatalogReference() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(Deployment::PackageCatalogInfo packageCatalogInfo, ::AppInstaller::Repository::Source sourceReference);
        void Initialize(Deployment::CreateCompositePackageCatalogOptions options);
#endif

        bool IsComposite();
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo Info();
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> ConnectAsync();
        winrt::Microsoft::Management::Deployment::ConnectResult Connect();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::SourceAgreement> SourceAgreements();
        hstring AdditionalPackageCatalogArguments();
        void AdditionalPackageCatalogArguments(hstring const& value);
        // Contract 6
        bool AcceptSourceAgreements();
        void AcceptSourceAgreements(bool value);
        // Contract 8.0
        winrt::Windows::Foundation::TimeSpan PackageCatalogBackgroundUpdateInterval();
        void PackageCatalogBackgroundUpdateInterval(winrt::Windows::Foundation::TimeSpan const& value);
        bool InstalledPackageInformationOnly();
        void InstalledPackageInformationOnly(bool value);
        winrt::Microsoft::Management::Deployment::AuthenticationArguments AuthenticationArguments();
        void AuthenticationArguments(winrt::Microsoft::Management::Deployment::AuthenticationArguments const& value);
        winrt::Microsoft::Management::Deployment::AuthenticationInfo AuthenticationInfo();
        // Contract 12.0
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::RefreshPackageCatalogResult, double> RefreshPackageCatalogAsync();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions m_compositePackageCatalogOptions{ nullptr };
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo m_info{ nullptr };
        std::once_flag m_sourceAgreementsOnceFlag;
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::SourceAgreement> m_sourceAgreements{ winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::SourceAgreement>() };
        ::AppInstaller::Repository::Source m_sourceReference;
        std::optional<std::string> m_additionalPackageCatalogArguments;
        bool m_acceptSourceAgreements = true;
        bool m_installedPackageInformationOnly = false;
        winrt::Windows::Foundation::TimeSpan m_packageCatalogBackgroundUpdateInterval = winrt::Windows::Foundation::TimeSpan::zero();
        winrt::Microsoft::Management::Deployment::AuthenticationArguments m_authenticationArguments{ nullptr };
        std::once_flag m_authenticationInfoOnceFlag;
        winrt::Microsoft::Management::Deployment::AuthenticationInfo m_authenticationInfo{ nullptr };
#endif
    };
}
