// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/RepositorySource.h>
#include "CatalogPackageMetadata.h"
#include "CatalogPackageMetadata.g.cpp"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    using Localization = ::AppInstaller::Manifest::Localization;

    void CatalogPackageMetadata::Initialize(::AppInstaller::Manifest::ManifestLocalization manifestLocalization)
    {
        m_manifestLocalization = std::move(manifestLocalization);
    }

    hstring CatalogPackageMetadata::Locale()
    {
        return winrt::to_hstring(m_manifestLocalization.Locale);
    }
    hstring CatalogPackageMetadata::Publisher()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::Publisher>());
    }
    hstring CatalogPackageMetadata::PublisherUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PublisherUrl>());
    }
    hstring CatalogPackageMetadata::PublisherSupportUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PublisherSupportUrl>());
    }
    hstring CatalogPackageMetadata::PrivacyUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PrivacyUrl>());
    }
    hstring CatalogPackageMetadata::Author()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::Author>());
    }
    hstring CatalogPackageMetadata::PackageName()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PackageName>());
    }
    hstring CatalogPackageMetadata::PackageUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PackageUrl>());
    }
    hstring CatalogPackageMetadata::License()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::License>());
    }
    hstring CatalogPackageMetadata::LicenseUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::LicenseUrl>());
    }
    hstring CatalogPackageMetadata::Copyright()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::Copyright>());
    }
    hstring CatalogPackageMetadata::CopyrightUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::CopyrightUrl>());
    }
    hstring CatalogPackageMetadata::ShortDescription()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::ShortDescription>());
    }
    hstring CatalogPackageMetadata::Description()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::Description>());
    }
    hstring CatalogPackageMetadata::ReleaseNotes()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::ReleaseNotes>());
    }
    hstring CatalogPackageMetadata::ReleaseNotesUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::ReleaseNotesUrl>());
    }
    hstring CatalogPackageMetadata::PurchaseUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PurchaseUrl>());
    }
    hstring CatalogPackageMetadata::InstallationNotes()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::InstallationNotes>());
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageAgreement> CatalogPackageMetadata::Agreements()
    {
        if (!m_packageAgreements)
        {
            auto agreements = winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::PackageAgreement>();
            for (auto const& agreement : m_manifestLocalization.Get<AppInstaller::Manifest::Localization::Agreements>())
            {
                auto packageAgreement = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageAgreement>>();
                packageAgreement->Initialize(agreement);
                agreements.Append(*packageAgreement);
            }
            m_packageAgreements = agreements;
        }

        return m_packageAgreements.GetView();
    }
    winrt::Windows::Foundation::Collections::IVectorView<hstring> CatalogPackageMetadata::Tags()
    {
        if (!m_tags)
        {
            // Vector hasn't been created yet, create and populate it.
            auto tags = winrt::single_threaded_vector<hstring>();
            for (auto&& tag : m_manifestLocalization.Get<Localization::Tags>())
            {
                tags.Append(winrt::to_hstring(tag));
            }
            m_tags = tags;
        }

        return m_tags.GetView();
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::Documentation> CatalogPackageMetadata::Documentations()
    {
        if (!m_documentations)
        {
            auto documentations = winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::Documentation>();
            for (auto const& documentation : m_manifestLocalization.Get<AppInstaller::Manifest::Localization::Documentations>())
            {
                auto documentationImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::Documentation>>();
                documentationImpl->Initialize(documentation);
                documentations.Append(*documentationImpl);
            }
            m_documentations = documentations;
        }

        return m_documentations.GetView();
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::Icon> CatalogPackageMetadata::Icons()
    {
        if (!m_icons)
        {
            auto icons = winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::Icon>();
            for (auto const& icon : m_manifestLocalization.Get<AppInstaller::Manifest::Localization::Icons>())
            {
                auto iconImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::Icon>>();
                iconImpl->Initialize(icon);
                icons.Append(*iconImpl);
            }
            m_icons = icons;
        }

        return m_icons.GetView();
    }
}
