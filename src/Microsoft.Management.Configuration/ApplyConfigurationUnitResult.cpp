// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ApplyConfigurationUnitResult.h"
#include "ApplyConfigurationUnitResult.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    ApplyConfigurationUnitResult::ApplyConfigurationUnitResult() :
        m_resultInformation(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>())
    {
    }

    void ApplyConfigurationUnitResult::Initialize(const IApplySettingsResult& result)
    {
        m_unit = result.Unit();
        THROW_HR_IF(E_POINTER, !m_unit);
        m_resultInformation->Initialize(result.ResultInformation());
    }

    void ApplyConfigurationUnitResult::Initialize(const IApplyGroupMemberSettingsResult& unitResult)
    {
        m_unit = unitResult.Unit();
        THROW_HR_IF(E_POINTER, !m_unit);
        m_resultInformation->Initialize(unitResult.ResultInformation());
        m_state = unitResult.State();
        m_previouslyInDesiredState = unitResult.PreviouslyInDesiredState();
        m_rebootRequired = unitResult.RebootRequired();
    }

    ConfigurationUnit ApplyConfigurationUnitResult::Unit()
    {
        return m_unit;
    }

    void ApplyConfigurationUnitResult::Unit(ConfigurationUnit value)
    {
        m_unit = std::move(value);
    }

    ConfigurationUnitState ApplyConfigurationUnitResult::State() const
    {
        return m_state.load();
    }

    void ApplyConfigurationUnitResult::State(ConfigurationUnitState value)
    {
        m_state.store(value);
    }

    bool ApplyConfigurationUnitResult::PreviouslyInDesiredState() const
    {
        return m_previouslyInDesiredState;
    }

    void ApplyConfigurationUnitResult::PreviouslyInDesiredState(bool value)
    {
        m_previouslyInDesiredState = value;
    }

    bool ApplyConfigurationUnitResult::RebootRequired() const
    {
        return m_rebootRequired;
    }

    void ApplyConfigurationUnitResult::RebootRequired(bool value)
    {
        m_rebootRequired = value;
    }

    IConfigurationUnitResultInformation ApplyConfigurationUnitResult::ResultInformation()
    {
        return *m_resultInformation;
    }

    void ApplyConfigurationUnitResult::ResultInformation(const Configuration::IConfigurationUnitResultInformation& value)
    {
        m_resultInformation->Initialize(value);
    }

    ApplyConfigurationUnitResult::ResultInformationT ApplyConfigurationUnitResult::ResultInformationInternal()
    {
        return m_resultInformation;
    }
}
