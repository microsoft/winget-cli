// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct DscConfigurationUnitSettingDetails : winrt::implements<DscConfigurationUnitSettingDetails, winrt::Microsoft::Management::Configuration::IConfigurationUnitSettingDetails>
    {
        winrt::hstring Identifier() const;
        void Identifier(const winrt::hstring& identifier);

        winrt::hstring Title() const;
        void Title(const winrt::hstring& title);

        winrt::hstring Description() const;
        void Description(const winrt::hstring& description);

        bool IsKey() const;
        void IsKey(bool isKey);

        bool IsInformational() const;
        void IsInformational(bool isInformational);

        bool IsRequired() const;
        void IsRequired(bool isRequired);

        Windows::Foundation::PropertyType Type();
        void Type(Windows::Foundation::PropertyType type);

        winrt::hstring Schema() const;
        void Schema(const winrt::hstring& schema);

    private:
        winrt::hstring m_identifier;
        winrt::hstring m_title;
        winrt::hstring m_description;
        bool m_isKey;
        bool m_isInformational;
        bool m_isRequired;
        Windows::Foundation::PropertyType m_type;
        winrt::hstring m_schema;
    };
}
