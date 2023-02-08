// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationUnitResultInformation.h"
#include "ConfigurationUnitResultInformation.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    void ConfigurationUnitResultInformation::Initialize(const Configuration::ConfigurationUnitResultInformation& other)
    {
        m_resultCode = other.ResultCode();
        m_description = other.Description();
    }

    void ConfigurationUnitResultInformation::Initialize(hresult resultCode, std::wstring_view description)
    {
        m_resultCode = resultCode;
        m_description = description;
    }

    void ConfigurationUnitResultInformation::Initialize(hresult resultCode, hstring description)
    {
        m_resultCode = resultCode;
        m_description = description;
    }

    hresult ConfigurationUnitResultInformation::ResultCode()
    {
        return m_resultCode;
    }

    void ConfigurationUnitResultInformation::ResultCode(hresult resultCode)
    {
        m_resultCode = resultCode;
    }

    hstring ConfigurationUnitResultInformation::Description()
    {
        return m_description;
    }

    void ConfigurationUnitResultInformation::Description(hstring value)
    {
        m_description = value;
    }
}
