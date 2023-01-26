// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "PackageLocale.g.h"
#include "PackageAgreement.h"
#include "Documentation.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct PackageLocale : PackageLocaleT<PackageLocale>
    {
        PackageLocale() = default;

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(::AppInstaller::Manifest::ManifestLocalization manifestLocalization);
#endif
        hstring Publisher();

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

        hstring ReleaseNotes();

        hstring ReleaseNotesUrl();

        hstring InstallationNotes();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ::AppInstaller::Manifest::ManifestLocalization m_manifestLocalization;
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::PackageAgreement> m_packageAgreements{ winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageAgreement>() };
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::Documentation> m_documentations{ winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::Documentation>() };
        Windows::Foundation::Collections::IVector<hstring> m_tags{ nullptr };
#endif
    };
}
