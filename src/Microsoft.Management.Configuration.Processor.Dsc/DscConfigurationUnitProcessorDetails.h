// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Microsoft.Management.Configuration.h>

namespace winrt::Microsoft::Management::Configuration::Processor::Dsc::implementation
{
    struct DscConfigurationUnitProcessorDetails : winrt::implements<DscConfigurationUnitProcessorDetails, winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails2>
    {
        DscConfigurationUnitProcessorDetails(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags);

        winrt::hstring UnitType() const;

        winrt::hstring UnitDescription() const;

        winrt::Windows::Foundation::Uri UnitDocumentationUri() const;

        winrt::Windows::Foundation::Uri UnitIconUri() const;

        winrt::hstring ModuleName() const;

        winrt::hstring ModuleType() const;

        winrt::hstring ModuleSource() const;

        winrt::hstring ModuleDescription() const;

        winrt::Windows::Foundation::Uri ModuleDocumentationUri() const;

        winrt::Windows::Foundation::Uri PublishedModuleUri() const;

        winrt::hstring Version() const;

        winrt::Windows::Foundation::DateTime PublishedDate() const;

        bool IsLocal() const;

        winrt::hstring Author() const;

        winrt::hstring Publisher() const;

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable> SigningInformation() const;

        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Configuration::IConfigurationUnitSettingDetails> Settings() const;

        bool IsPublic() const;

        bool IsGroup() const;

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        winrt::hstring m_unitType;
        winrt::hstring m_unitDescription;
        winrt::Windows::Foundation::Uri m_unitDocumentationUri = nullptr;
        winrt::Windows::Foundation::Uri m_unitIconUr = nullptr;
        winrt::hstring m_moduleName;
        winrt::hstring m_moduleType;
        winrt::hstring m_moduleSource;
        winrt::hstring m_moduleDescription;
        winrt::Windows::Foundation::Uri m_moduleDocumentationUri = nullptr;
        winrt::Windows::Foundation::Uri m_publishedModuleUri = nullptr;
        winrt::hstring m_version;
        winrt::Windows::Foundation::DateTime m_publishedDate;
        bool m_isLocal = false;
        winrt::hstring m_author;
        winrt::hstring m_publisher;
        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable> m_signingInformation = nullptr;
        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Configuration::IConfigurationUnitSettingDetails> m_settings;
        bool m_isPublic = false;
        bool m_isGroup = false;
    };
}
