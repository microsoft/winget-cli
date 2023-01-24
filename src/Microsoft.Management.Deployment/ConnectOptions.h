// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConnectOptions.g.h"
#include "SourceAgreement.h"
#include "Public/ComClsids.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid(WINGET_OUTOFPROC_COM_CLSID_ConnectOptions)]
    struct ConnectOptions : ConnectOptionsT<ConnectOptions>
    {
        ConnectOptions();

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::SourceAgreement> SourceAgreements();

        void SourceAgreements(winrt::Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::SourceAgreement> const& value);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Deployment::SourceAgreement> m_sourceAgreements{ winrt::single_threaded_vector<winrt::Microsoft::Management::Deployment::SourceAgreement>() };
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct ConnectOptions : ConnectOptionsT<ConnectOptions, implementation::ConnectOptions>
    {
    };
}
#endif
