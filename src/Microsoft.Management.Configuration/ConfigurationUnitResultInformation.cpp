// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationUnitResultInformation.h"
#include "ConfigurationUnitResultInformation.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void ConfigurationUnitResultInformation::Initialize(hresult resultCode, std::wstring_view description)
    {
        m_resultCode = resultCode;
        m_description = description;
    }

    hresult ConfigurationUnitResultInformation::ResultCode()
    {
        return m_resultCode;
    }

    hstring ConfigurationUnitResultInformation::Description()
    {
        return m_description;
    }
}
