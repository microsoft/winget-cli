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
        void Initialize(winrt::Microsoft::Management::Deployment::PackageCatalogInfo packageCatalogInfo, ::AppInstaller::Repository::Source sourceReference);
        void Initialize(winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions options);
#endif

        bool IsComposite();
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo Info();
        winrt::Windows::Foundation::IAsyncOperation<winrt::Microsoft::Management::Deployment::ConnectResult> ConnectAsync();
        winrt::Microsoft::Management::Deployment::ConnectResult Connect();
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Deployment::SourceAgreement> SourceAgreements();
        hstring AdditionalPackageCatalogArguments();
        void AdditionalPackageCatalogArguments(hstring const& value);
        // Contract 6.0
        bool AcceptSourceAgreements();
        void AcceptSourceAgreements(bool value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        winrt::Microsoft::Management::Deployment::CreateCompositePackageCatalogOptions m_compositePackageCatalogOptions{ nullptr };
        winrt::Microsoft::Management::Deployment::PackageCatalogInfo m_info{ nullptr };
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::SourceAgreement> m_sourceAgreements{ winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::SourceAgreement>() };
        ::AppInstaller::Repository::Source m_sourceReference;
        std::optional<std::string> m_additionalPackageCatalogArguments;
        bool m_acceptSourceAgreements = true;
        std::once_flag m_sourceAgreementsOnceFlag;
#endif
    };
}
