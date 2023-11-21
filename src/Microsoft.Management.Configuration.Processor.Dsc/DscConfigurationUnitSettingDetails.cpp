// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationUnitSettingDetails.h"

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;

    winrt::hstring DscConfigurationUnitSettingDetails::Identifier() const
    {
        return m_identifier;
    }

    void DscConfigurationUnitSettingDetails::Identifier(const winrt::hstring& identifier)
    {
        m_identifier = identifier;
    }

    winrt::hstring DscConfigurationUnitSettingDetails::Title() const
    {
        return m_title;
    }

    void DscConfigurationUnitSettingDetails::Title(const winrt::hstring& title)
    {
        m_title = title;
    }

    winrt::hstring DscConfigurationUnitSettingDetails::Description() const
    {
        return m_description;
    }

    void DscConfigurationUnitSettingDetails::Description(const winrt::hstring& description)
    {
        m_description = description;
    }

    bool DscConfigurationUnitSettingDetails::IsKey() const
    {
        return m_isKey;
    }

    void DscConfigurationUnitSettingDetails::IsKey(bool isKey)
    {
        m_isKey = isKey;
    }

    bool DscConfigurationUnitSettingDetails::IsInformational() const
    {
        return m_isInformational;
    }

    void DscConfigurationUnitSettingDetails::IsInformational(bool isInformational)
    {
        m_isInformational = isInformational;
    }

    bool DscConfigurationUnitSettingDetails::IsRequired() const
    {
        return m_isRequired;
    }

    void DscConfigurationUnitSettingDetails::IsRequired(bool isRequired)
    {
        m_isRequired = isRequired;
    }

    Windows::Foundation::PropertyType DscConfigurationUnitSettingDetails::Type()
    {
        return m_type;
    }

    void DscConfigurationUnitSettingDetails::Type(Windows::Foundation::PropertyType type)
    {
        m_type = type;
    }

    winrt::hstring DscConfigurationUnitSettingDetails::Schema() const
    {
        return m_schema;
    }

    void DscConfigurationUnitSettingDetails::Schema(const winrt::hstring& schema)
    {
        m_schema = schema;
    }
}
