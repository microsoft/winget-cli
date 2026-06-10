// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "winrt/Microsoft.Management.Configuration.h"
#include "ConfigurationUnitResultInformation.h"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplyGroupSettingsResult : winrt::implements<ApplyGroupSettingsResult, IApplyGroupSettingsResult>
    {
        ApplyGroupSettingsResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        using ResultInformationPtr = decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>());

        void Group(const Windows::Foundation::IInspectable& value);
        void RebootRequired(bool value);
        ResultInformationPtr ResultInformationInternal();
#endif

        Windows::Foundation::IInspectable Group();
        bool RebootRequired();
        IConfigurationUnitResultInformation ResultInformation();
        Windows::Foundation::Collections::IVector<IApplyGroupMemberSettingsResult> UnitResults();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        Windows::Foundation::IInspectable m_group;
        bool m_rebootRequired = false;
        ResultInformationPtr m_resultInformation;
        Windows::Foundation::Collections::IVector<IApplyGroupMemberSettingsResult> m_unitResults;
#endif
    };
}
