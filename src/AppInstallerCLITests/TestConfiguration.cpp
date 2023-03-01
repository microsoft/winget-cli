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

    IConfigurationUnitProcessorDetails TestConfigurationSetProcessor::GetUnitProcessorDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailLevel detailLevel)
    {
        if (GetUnitProcessorDetailsFunc)
        {
            return GetUnitProcessorDetailsFunc(unit, detailLevel);
        }
        else
        {
            return winrt::make<TestConfigurationUnitProcessorDetails>(unit);
        }
    }

    IConfigurationUnitProcessor TestConfigurationSetProcessor::CreateUnitProcessor(const ConfigurationUnit& unit, const IMapView<winrt::hstring, IInspectable>& directivesOverlay)
    {
        if (CreateUnitProcessorFunc)
        {
            return CreateUnitProcessorFunc(unit, directivesOverlay);
        }
        else
        {
            return winrt::make<TestConfigurationUnitProcessor>(unit, directivesOverlay);
        }
    }

    TestConfigurationUnitProcessorDetails::TestConfigurationUnitProcessorDetails(const ConfigurationUnit& unit) :
        UnitNameValue(unit.UnitName())
    {}

    TestConfigurationUnitProcessor::TestConfigurationUnitProcessor(const ConfigurationUnit& unit, const IMapView<winrt::hstring, IInspectable>& directivesOverlay) :
        UnitValue(unit), DirectivesOverlayValue(directivesOverlay)
    {}

    TestSettingsResult TestConfigurationUnitProcessor::TestSettings()
    {
        if (TestSettingsFunc)
        {
            return TestSettingsFunc();
        }
        else
        {
            return TestSettingsResult{};
        }
    }

    GetSettingsResult TestConfigurationUnitProcessor::GetSettings()
    {
        if (GetSettingsFunc)
        {
            return GetSettingsFunc();
        }
        else
        {
            return GetSettingsResult{};
        }
    }

    ApplySettingsResult TestConfigurationUnitProcessor::ApplySettings()
    {
        if (ApplySettingsFunc)
        {
            return ApplySettingsFunc();
        }
        else
        {
            return ApplySettingsResult{};
        }
    }
}
