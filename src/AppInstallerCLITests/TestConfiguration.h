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

        winrt::event_token Diagnostics(const winrt::Windows::Foundation::EventHandler<winrt::Microsoft::Management::Configuration::IDiagnosticInformation>& handler);
        void Diagnostics(const winrt::event_token& token) noexcept;

        winrt::Microsoft::Management::Configuration::DiagnosticLevel MinimumLevel();
        void MinimumLevel(winrt::Microsoft::Management::Configuration::DiagnosticLevel value);

        std::function<winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor(const winrt::Microsoft::Management::Configuration::ConfigurationSet&)> CreateSetProcessorFunc;

    private:
        winrt::event<winrt::Windows::Foundation::EventHandler<winrt::Microsoft::Management::Configuration::IDiagnosticInformation>> m_diagnostics;
    };

    struct TestConfigurationSetProcessor : winrt::implements<TestConfigurationSetProcessor, winrt::Microsoft::Management::Configuration::IConfigurationSetProcessor>
    {
        winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails GetUnitProcessorDetails(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit,
            winrt::Microsoft::Management::Configuration::ConfigurationUnitDetailFlags detailFlags);

        std::function<winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit&,
            winrt::Microsoft::Management::Configuration::ConfigurationUnitDetailFlags)> GetUnitProcessorDetailsFunc;

        winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessor CreateUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        std::function<winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit&)> CreateUnitProcessorFunc;
    };

    struct TestConfigurationUnitProcessorDetails : winrt::implements<TestConfigurationUnitProcessorDetails, winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessorDetails>
    {
        TestConfigurationUnitProcessorDetails(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        winrt::hstring UnitTypeValue;
        winrt::hstring UnitType() const { return UnitTypeValue; }

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

        bool IsLocalValue = false;
        bool IsLocal() const { return IsLocalValue; }

        winrt::hstring AuthorValue;
        winrt::hstring Author() const { return AuthorValue; }

        winrt::hstring PublisherValue;
        winrt::hstring Publisher() const { return PublisherValue; }

        winrt::Windows::Foundation::Collections::IVector<winrt::Windows::Foundation::IInspectable> SigningInformationValue = nullptr;
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::Foundation::IInspectable> SigningInformation() const { return SigningInformationValue.GetView(); }

        winrt::Windows::Foundation::Collections::IVector<winrt::Microsoft::Management::Configuration::IConfigurationUnitSettingDetails> SettingsValue;
        winrt::Windows::Foundation::Collections::IVectorView<winrt::Microsoft::Management::Configuration::IConfigurationUnitSettingDetails> Settings() const { return (SettingsValue ? SettingsValue.GetView() : nullptr); }

        bool IsPublicValue = false;
        bool IsPublic() const { return IsPublicValue; }
    };

    struct TestConfigurationUnitProcessor : winrt::implements<TestConfigurationUnitProcessor, winrt::Microsoft::Management::Configuration::IConfigurationUnitProcessor>
    {
        TestConfigurationUnitProcessor(
            const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit);

        winrt::Microsoft::Management::Configuration::ConfigurationUnit UnitValue;
        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit() { return UnitValue; }

        winrt::Microsoft::Management::Configuration::ITestSettingsResult TestSettings();

        std::function<winrt::Microsoft::Management::Configuration::ITestSettingsResult()> TestSettingsFunc;

        winrt::Microsoft::Management::Configuration::IGetSettingsResult GetSettings();

        std::function<winrt::Microsoft::Management::Configuration::IGetSettingsResult()> GetSettingsFunc;

        winrt::Microsoft::Management::Configuration::IApplySettingsResult ApplySettings();

        std::function<winrt::Microsoft::Management::Configuration::IApplySettingsResult()> ApplySettingsFunc;
    };

    struct TestSettingsResultInstance : winrt::implements<TestSettingsResultInstance, winrt::Microsoft::Management::Configuration::ITestSettingsResult>
    {
        TestSettingsResultInstance(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit) : m_unit(unit) {}

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit() { return m_unit; }

        winrt::Microsoft::Management::Configuration::ConfigurationTestResult TestResult() { return m_testResult; }
        void TestResult(winrt::Microsoft::Management::Configuration::ConfigurationTestResult value) { m_testResult = value; }

        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation ResultInformation() { return m_resultInformation; }
        void ResultInformation(winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation value) { m_resultInformation = value; }

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        winrt::Microsoft::Management::Configuration::ConfigurationTestResult m_testResult = winrt::Microsoft::Management::Configuration::ConfigurationTestResult::Unknown;
        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };

    struct ApplySettingsResultInstance : winrt::implements<ApplySettingsResultInstance, winrt::Microsoft::Management::Configuration::IApplySettingsResult>
    {
        ApplySettingsResultInstance(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit) : m_unit(unit) {}

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit() { return m_unit; }

        bool RebootRequired() { return m_rebootRequired; }
        void RebootRequired(bool value) { m_rebootRequired = value; }

        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation ResultInformation() { return m_resultInformation; }
        void ResultInformation(winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation value) { m_resultInformation = value; }

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        bool m_rebootRequired = false;
        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };

    struct GetSettingsResultInstance : winrt::implements<GetSettingsResultInstance, winrt::Microsoft::Management::Configuration::IGetSettingsResult>
    {
        GetSettingsResultInstance(const winrt::Microsoft::Management::Configuration::ConfigurationUnit& unit) : m_unit(unit) {}

        winrt::Microsoft::Management::Configuration::ConfigurationUnit Unit() { return m_unit; }

        winrt::Windows::Foundation::Collections::ValueSet Settings() { return m_settings; }
        void Settings(winrt::Windows::Foundation::Collections::ValueSet value) { m_settings = value; }

        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation ResultInformation() { return m_resultInformation; }
        void ResultInformation(winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation value) { m_resultInformation = value; }

    private:
        winrt::Microsoft::Management::Configuration::ConfigurationUnit m_unit;
        winrt::Windows::Foundation::Collections::ValueSet m_settings;
        winrt::Microsoft::Management::Configuration::IConfigurationUnitResultInformation m_resultInformation;
    };
}
