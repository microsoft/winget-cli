// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ApplyGroupSettingsResult.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ApplyGroupSettingsResult::ApplyGroupSettingsResult() :
        m_resultInformation(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>()),
        m_unitResults(winrt::multi_threaded_vector<IApplyGroupMemberSettingsResult>())
    {}

    void ApplyGroupSettingsResult::Group(const Windows::Foundation::IInspectable& value)
    {
        m_group = value;
    }

    void ApplyGroupSettingsResult::RebootRequired(bool value)
    {
        m_rebootRequired = value;
    }

    ApplyGroupSettingsResult::ResultInformationPtr ApplyGroupSettingsResult::ResultInformationInternal()
    {
        return m_resultInformation;
    }

    Windows::Foundation::IInspectable ApplyGroupSettingsResult::Group()
    {
        return m_group;
    }

    bool ApplyGroupSettingsResult::RebootRequired()
    {
        return m_rebootRequired;
    }

    IConfigurationUnitResultInformation ApplyGroupSettingsResult::ResultInformation()
    {
        return *m_resultInformation;
    }

    Windows::Foundation::Collections::IVector<IApplyGroupMemberSettingsResult> ApplyGroupSettingsResult::UnitResults()
    {
        return m_unitResults;
    }
}
