// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ApplyConfigurationUnitResult.g.h"
#include "ConfigurationUnitResultInformation.h"
#include <atomic>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct ApplyConfigurationUnitResult : ApplyConfigurationUnitResultT<ApplyConfigurationUnitResult, IApplyGroupMemberSettingsResult>
    {
        using ConfigurationUnit = Configuration::ConfigurationUnit;
        using ResultInformationT = decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>());

        ApplyConfigurationUnitResult();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
        void Initialize(const IApplySettingsResult& result);
        void Initialize(const IApplyGroupMemberSettingsResult& unitResult);

        void Unit(ConfigurationUnit value);
        void State(ConfigurationUnitState value);
        void PreviouslyInDesiredState(bool value);
        void RebootRequired(bool value);
        void ResultInformation(const Configuration::IConfigurationUnitResultInformation& value);
        ResultInformationT ResultInformationInternal();
#endif

        ConfigurationUnit Unit();
        ConfigurationUnitState State() const;
        bool PreviouslyInDesiredState() const;
        bool RebootRequired() const;
        IConfigurationUnitResultInformation ResultInformation();

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        ConfigurationUnit m_unit = nullptr;
        std::atomic<ConfigurationUnitState> m_state = ConfigurationUnitState::Pending;
        bool m_previouslyInDesiredState = false;
        bool m_rebootRequired = false;
        ResultInformationT m_resultInformation;
#endif
    };
}
