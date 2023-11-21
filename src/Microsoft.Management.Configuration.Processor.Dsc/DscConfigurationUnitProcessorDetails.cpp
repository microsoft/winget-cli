// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscConfigurationUnitProcessorDetails.h"

namespace winrt::Microsoft::Management::Configuration::Dsc::implementation
{
    using namespace winrt::Microsoft::Management::Configuration;
    using namespace winrt::Windows::Foundation;
    using namespace winrt::Windows::Foundation::Collections;

    DscConfigurationUnitProcessorDetails::DscConfigurationUnitProcessorDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags /*detailFlags*/)
        : m_unit(unit) {}

    winrt::hstring DscConfigurationUnitProcessorDetails::UnitType() const
    {
        return m_unitType;
    }

    winrt::hstring DscConfigurationUnitProcessorDetails::UnitDescription() const {
        return m_unitDescription;
    }

    Uri DscConfigurationUnitProcessorDetails::UnitDocumentationUri() const {
        return m_unitDocumentationUri;
    }

    Uri DscConfigurationUnitProcessorDetails::UnitIconUri() const {
        return m_unitDocumentationUri;
    }

    winrt::hstring DscConfigurationUnitProcessorDetails::ModuleName() const {
        return m_moduleName;
    }

    winrt::hstring DscConfigurationUnitProcessorDetails::ModuleType() const {
        return m_moduleType;
    }

    winrt::hstring DscConfigurationUnitProcessorDetails::ModuleSource() const {
        return m_moduleSource;
    }

    winrt::hstring DscConfigurationUnitProcessorDetails::ModuleDescription() const {
        return m_moduleDescription;
    }

    Uri DscConfigurationUnitProcessorDetails::ModuleDocumentationUri() const {
        return m_moduleDocumentationUri;
    }

    Uri DscConfigurationUnitProcessorDetails::PublishedModuleUri() const {
        return m_publishedModuleUri;
    }

    winrt::hstring DscConfigurationUnitProcessorDetails::Version() const {
        return m_version;
    }

    DateTime DscConfigurationUnitProcessorDetails::PublishedDate() const {
        return m_publishedDate;
    }

    bool DscConfigurationUnitProcessorDetails::IsLocal() const {
        return m_isLocal;
    }

    winrt::hstring DscConfigurationUnitProcessorDetails::Author() const {
        return m_author;
    }

    winrt::hstring DscConfigurationUnitProcessorDetails::Publisher() const {
        return m_publisher;
    }

    IVectorView<IInspectable> DscConfigurationUnitProcessorDetails::SigningInformation() const
    {
        return m_signingInformation.GetView();
    }

    IVectorView<IConfigurationUnitSettingDetails> DscConfigurationUnitProcessorDetails::Settings() const
    {
        return (m_settings ? m_settings.GetView() : nullptr);
    }

    bool DscConfigurationUnitProcessorDetails::IsPublic() const
    {
        return m_isPublic;
    }

    bool DscConfigurationUnitProcessorDetails::IsGroup() const
    {
        return m_isGroup;
    }
}
