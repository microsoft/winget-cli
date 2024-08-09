// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "CatalogPackageMetadata.g.h"
#include "PackageAgreement.h"
#include "Documentation.h"
#include "Icon.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct CatalogPackageMetadata : CatalogPackageMetadataT<CatalogPackageMetadata>
    {
        CatalogPackageMetadata() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Manifest::ManifestLocalization manifestLocalization);
#endif
        hstring Locale();

        hstring Publisher();

        hstring PublisherUrl();

        hstring PublisherSupportUrl();

        hstring PrivacyUrl();

        hstring Author();

        hstring PackageName();

        hstring PackageUrl();

        hstring License();

        hstring LicenseUrl();

        hstring Copyright();

        hstring CopyrightUrl();

        hstring ShortDescription();

        hstring Description();

        winrt::Windows::Foundation::Collections::IVectorView<hstring> Tags();

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageAgreement> Agreements();

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::Documentation> Documentations();

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::Icon> Icons();

        hstring ReleaseNotes();

        hstring ReleaseNotesUrl();

        hstring PurchaseUrl();

        hstring InstallationNotes();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Manifest::ManifestLocalization m_manifestLocalization;
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageAgreement> m_packageAgreements{ nullptr };
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::Documentation> m_documentations{ nullptr };
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::Icon> m_icons{ nullptr };
        Windows::Foundation::Collections::IVector<hstring> m_tags{ nullptr };
#endif
    };
}
