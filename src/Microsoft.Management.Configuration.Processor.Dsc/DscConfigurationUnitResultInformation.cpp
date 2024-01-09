// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;

    winrt::hresult DscConfigurationUnitResultInformation::ResultCode() const
    {
        return m_resultCode;
    }

    void DscConfigurationUnitResultInformation::ResultCode(winrt::hresult resultCode)
    {
        m_resultCode = resultCode;
    }

    winrt::hstring DscConfigurationUnitResultInformation::Description() const
    {
        return m_description;
    }

    void DscConfigurationUnitResultInformation::Description(const winrt::hstring& description)
    {
        m_description = description;
    }

    winrt::hstring DscConfigurationUnitResultInformation::Details() const
    {
        return m_details;
    }

    void DscConfigurationUnitResultInformation::Details(const winrt::hstring& details)
    {
        m_details = details;
    }

    ConfigurationUnitResultSource DscConfigurationUnitResultInformation::ResultSource() const
    {
        return m_resultSource;
    }

    void DscConfigurationUnitResultInformation::ResultSource(ConfigurationUnitResultSource resultSource)
    {
        m_resultSource = resultSource;
    }
}
