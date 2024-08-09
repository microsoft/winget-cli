// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestConfiguration.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Foundation::Collections;
using namespace winrt::Microsoft::Management::Configuration;

namespace TestCommon
{
    IConfigurationSetProcessor TestConfigurationSetProcessorFactory::CreateSetProcessor(const ConfigurationSet& configurationSet)
    {
        if (CreateSetProcessorFunc)
        {
            return CreateSetProcessorFunc(configurationSet);
        }
        else
        {
            return winrt::make<TestConfigurationSetProcessor>();
        }
    }

    winrt::event_token TestConfigurationSetProcessorFactory::Diagnostics(const EventHandler<IDiagnosticInformation>& handler)
    {
        return m_diagnostics.add(handler);
    }

    void TestConfigurationSetProcessorFactory::Diagnostics(const winrt::event_token& token) noexcept
    {
        m_diagnostics.remove(token);
    }

    DiagnosticLevel TestConfigurationSetProcessorFactory::MinimumLevel()
    {
        return DiagnosticLevel::Informational;
    }

    void TestConfigurationSetProcessorFactory::MinimumLevel(DiagnosticLevel)
    {
    }

    IConfigurationUnitProcessorDetails TestConfigurationSetProcessor::GetUnitProcessorDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags)
    {
        if (GetUnitProcessorDetailsFunc)
        {
            return GetUnitProcessorDetailsFunc(unit, detailFlags);
        }
        else
        {
            return winrt::make<TestConfigurationUnitProcessorDetails>(unit);
        }
    }

    IConfigurationUnitProcessor TestConfigurationSetProcessor::CreateUnitProcessor(const ConfigurationUnit& unit)
    {
        if (CreateUnitProcessorFunc)
        {
            return CreateUnitProcessorFunc(unit);
        }
        else
        {
            return winrt::make<TestConfigurationUnitProcessor>(unit);
        }
    }

    TestConfigurationUnitProcessorDetails::TestConfigurationUnitProcessorDetails(const ConfigurationUnit& unit) :
        UnitTypeValue(unit.Type())
    {}

    TestConfigurationUnitProcessor::TestConfigurationUnitProcessor(const ConfigurationUnit& unit) :
        UnitValue(unit)
    {}

    ITestSettingsResult TestConfigurationUnitProcessor::TestSettings()
    {
        if (TestSettingsFunc)
        {
            return TestSettingsFunc();
        }
        else
        {
            return winrt::make<TestSettingsResultInstance>(UnitValue);
        }
    }

    IGetSettingsResult TestConfigurationUnitProcessor::GetSettings()
    {
        if (GetSettingsFunc)
        {
            return GetSettingsFunc();
        }
        else
        {
            return winrt::make<GetSettingsResultInstance>(UnitValue);
        }
    }

    IApplySettingsResult TestConfigurationUnitProcessor::ApplySettings()
    {
        if (ApplySettingsFunc)
        {
            return ApplySettingsFunc();
        }
        else
        {
            return winrt::make<ApplySettingsResultInstance>(UnitValue);
        }
    }
}
