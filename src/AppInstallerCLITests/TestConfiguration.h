// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Security.Cryptography.Certificates.h>
#include <winrt/Microsoft.Management.Configuration.h>
#include <functional>

namespace TestCommon
{
    struct TestConfigurationSetProcessorFactory : winrt::implements<TestConfigurationSetProcessorFactory, winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory>
    {
        winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor CreateSetProcessor(const winrt::Microsoft::Management::Configuration::ConfigurationSet& configurationSet);

        winrt::event_token Diagnostics(const winrt::Windows::Foundation::EventHandler<winrt::Microsoft::Management::Configuration::DiagnosticInformation>& handler);
        void Diagnostics(const winrt::event_token& token) noexcept;

        winrt::Microsoft::Management::Configuration::DiagnosticLevel MinimumLevel();
        void MinimumLevel(winrt::Microsoft::Management::Configuration::DiagnosticLevel value);

        std::function<winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor(const winrt::Microsoft::Management::Configuration::ConfigurationSet&)> CreateSetProcessorFunc;

    private:
        winrt::event<winrt::Windows::Foundation::EventHandler<winrt::Microsoft::Management::Configuration::DiagnosticInformation>> m_diagnostics;
    };

    struct TestConfigurationSetProcessor : winrt::implements<TestConfigurationSetProcessor, winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor>
    {
        winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails GetUnitProcessorDetails(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit,
            winrt::Microsoft::Management::Configuration::ConfigurationUnitDetailLevel detailLevel);

        std::function<winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit&,
            winrt::Microsoft::Management::Configuration::ConfigurationUnitDetailLevel)> GetUnitProcessorDetailsFunc;

        winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessor CreateUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit,
            const winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Windows::Foundation::IInspectable>& directivesOverlay);

        std::function<winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit&,
            const winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Windows::Foundation::IInspectable>&)> CreateUnitProcessorFunc;
    };

    struct TestConfigurationUnitProcessorDetails : winrt::implements<TestConfigurationUnitProcessorDetails, winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails>
    {
        TestConfigurationUnitProcessorDetails(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        winrt::hstring UnitNameValue;
        winrt::hstring UnitName() const { return UnitNameValue; }

        winrt::hstring UnitDescriptionValue;
        winrt::hstring UnitDescription() const { return UnitDescriptionValue; }

        winrt::Windows::Foundation::Uri UnitDocumentationUriValue = nullptr;
        winrt::Windows::Foundation::Uri UnitDocumentationUri() const { return UnitDocumentationUriValue; }

        winrt::Windows::Foundation::Uri UnitIconUriValue = nullptr;
        winrt::Windows::Foundation::Uri UnitIconUri() const { return UnitIconUriValue; }

        winrt::hstring ModuleNameValue;
        winrt::hstring ModuleName() const { return ModuleNameValue; }

        winrt::hstring ModuleTypeValue;
        winrt::hstring ModuleType() const { return ModuleTypeValue; }

        winrt::hstring ModuleSourceValue;
        winrt::hstring ModuleSource() const { return ModuleSourceValue; }

        winrt::hstring ModuleDescriptionValue;
        winrt::hstring ModuleDescription() const { return ModuleDescriptionValue; }

        winrt::Windows::Foundation::Uri ModuleDocumentationUriValue = nullptr;
        winrt::Windows::Foundation::Uri ModuleDocumentationUri() const { return ModuleDocumentationUriValue; }

        winrt::Windows::Foundation::Uri PublishedModuleUriValue = nullptr;
        winrt::Windows::Foundation::Uri PublishedModuleUri() const { return PublishedModuleUriValue; }

        winrt::hstring VersionValue;
        winrt::hstring Version() const { return VersionValue; }

        winrt::Windows::Foundation::DateTime PublishedDateValue;
        winrt::Windows::Foundation::DateTime PublishedDate() const { return PublishedDateValue; }

        bool IsLocalValue;
        bool IsLocal() const { return IsLocalValue; }

        winrt::hstring AuthorValue;
        winrt::hstring Author() const { return AuthorValue; }

        winrt::hstring PublisherValue;
        winrt::hstring Publisher() const { return PublisherValue; }

        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable> SigningInformationValue = nullptr;
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable> SigningInformation() const { return SigningInformationValue.GetView(); }

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Configuration::IConfigurationUnitSettingDetails> SettingsValue;
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Configuration::IConfigurationUnitSettingDetails> Settings() const { return (SettingsValue ? SettingsValue.GetView() : nullptr); }

        bool IsPublicValue;
        bool IsPublic() const { return IsPublicValue; }
    };

    struct TestConfigurationUnitProcessor : winrt::implements<TestConfigurationUnitProcessor, winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessor>
    {
        TestConfigurationUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit,
            const winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Windows::Foundation::IInspectable>& directivesOverlay);

        winrt::Microsoft::Management::Configuration::ConfigurationUnit UnitValue;
        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit() { return UnitValue; }

        winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Windows::Foundation::IInspectable> DirectivesOverlayValue;
        winrt::Windows::Foundation::Collections::IMapView<winrt::hstring, winrt::Windows::Foundation::IInspectable> DirectivesOverlay() { return DirectivesOverlayValue; }

        winrt::Microsoft::Management::Configuration::TestSettingsResult TestSettings();

        std::function<winrt::Microsoft::Management::Configuration::TestSettingsResult()> TestSettingsFunc;

        winrt::Microsoft::Management::Configuration::GetSettingsResult GetSettings();

        std::function<winrt::Microsoft::Management::Configuration::GetSettingsResult()> GetSettingsFunc;

        winrt::Microsoft::Management::Configuration::ApplySettingsResult ApplySettings();

        std::function<winrt::Microsoft::Management::Configuration::ApplySettingsResult()> ApplySettingsFunc;
    };
}
