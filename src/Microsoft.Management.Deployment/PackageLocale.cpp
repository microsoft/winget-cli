// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <winget/RepositorySource.h>
#include "PackageLocale.h"
#include "PackageLocale.g.cpp"
#include "Documentation.h"
#include <wil\cppwinrt_wrl.h>

namespace winrt::Microsoft::Management::Deployment::implementation
{
    using Localization = ::AppInstaller::Manifest::Localization;

    void PackageLocale::Initialize(::AppInstaller::Manifest::ManifestLocalization manifestLocalization)
    {
        m_manifestLocalization = manifestLocalization;
    }

    hstring PackageLocale::Publisher()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::Publisher>());
    }
    hstring PackageLocale::PublisherSupportUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PublisherSupportUrl>());
    }
    hstring PackageLocale::PrivacyUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PrivacyUrl>());
    }
    hstring PackageLocale::Author()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::Author>());
    }
    hstring PackageLocale::PackageName()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PackageName>());
    }
    hstring PackageLocale::PackageUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::PackageUrl>());
    }
    hstring PackageLocale::License()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::License>());
    }
    hstring PackageLocale::LicenseUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::LicenseUrl>());
    }
    hstring PackageLocale::Copyright()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::Copyright>());
    }
    hstring PackageLocale::CopyrightUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::CopyrightUrl>());
    }
    hstring PackageLocale::ShortDescription()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::ShortDescription>());
    }
    hstring PackageLocale::Description()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::Description>());
    }
    hstring PackageLocale::ReleaseNotes()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::ReleaseNotes>());
    }
    hstring PackageLocale::ReleaseNotesUrl()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::ReleaseNotesUrl>());
    }
    hstring PackageLocale::InstallationNotes()
    {
        return winrt::to_hstring(m_manifestLocalization.Get<Localization::InstallationNotes>());
    }
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::PackageAgreement> PackageLocale::Agreements()
    {
        for (auto const& agreement : m_manifestLocalization.Get<AppInstaller::Manifest::Localization::Agreements>())
        {
            auto packageAgreement = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::PackageAgreement>>();
            packageAgreement->Initialize(agreement);
            m_packageAgreements.Append(*packageAgreement);
        }

        return m_packageAgreements.GetView();
    }
    winrt::Windows::Foundation::Collections::IVectorView<hstring> PackageLocale::Tags()
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
    winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::Documentation> PackageLocale::Documentations()
    {
        for (auto const& documentation : m_manifestLocalization.Get<AppInstaller::Manifest::Localization::Documentations>())
        {
            auto documentationImpl = winrt::make_self<wil::details::module_count_wrapper<winrt::Microsoft::Management::Deployment::implementation::Documentation>>();
            documentationImpl->Initialize(documentation);
            m_documentations.Append(*documentationImpl);
        }

        return m_documentations.GetView();
    }
}
