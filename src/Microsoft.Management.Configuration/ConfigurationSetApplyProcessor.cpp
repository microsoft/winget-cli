// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationSetApplyProcessor.h"
#include "ExceptionResultHelpers.h"

#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerStrings.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace
    {
        std::string GetNormalizedIdentifier(hstring identifier)
        {
            using namespace AppInstaller::Utility;
            return FoldCase(NormalizedString{ identifier });
        }
    }

    ConfigurationSetApplyProcessor::ConfigurationSetApplyProcessor(const Configuration::ConfigurationSet& configurationSet, IConfigurationSetProcessor&& setProcessor) :
        m_setProcessor(std::move(setProcessor))
    {
        // Create a copy of the set of configuration units
        auto unitsView = configurationSet.ConfigurationUnits();
        std::vector<ConfigurationUnit> unitsToProcess{ unitsView.Size() };
        unitsView.GetMany(0, unitsToProcess);

        // Create the unit info vector from these units
        for (const auto& unit : unitsToProcess)
        {
            m_unitInfo.emplace_back(unit);
        }
    }

    void ConfigurationSetApplyProcessor::Process()
    {
        if (!PreProcess())
        {
            return;
        }

        ProcessInternal(HasProcessedSuccessfully, &ConfigurationSetApplyProcessor::ProcessUnit);
    }

    std::vector<Configuration::ApplyConfigurationUnitResult> ConfigurationSetApplyProcessor::GetUnitResults() const
    {
        THROW_HR(E_NOTIMPL);
    }

    hresult ConfigurationSetApplyProcessor::ResultCode() const
    {
        return m_resultCode;
    }

    ConfigurationSetApplyProcessor::UnitInfo::UnitInfo(const Configuration::ConfigurationUnit& unit) :
        Unit(unit), Result(make_self<wil::details::module_count_wrapper<ConfigurationUnitResultInformation>>())
    {
    }

    bool ConfigurationSetApplyProcessor::PreProcess()
    {
        bool result = true;

        for (size_t i = 0; i < m_unitInfo.size(); ++i)
        {
            if (!AddUnitToMap(m_unitInfo[i], i))
            {
                result = false;
            }
        }

        if (!result)
        {
            // This is the only error that adding to the map can produce
            m_resultCode = WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER;
            return false;
        }

        for (UnitInfo& unitInfo : m_unitInfo)
        {
            for (hstring dependencyHstring : unitInfo.Unit.Dependencies())
            {
                std::string dependency = GetNormalizedIdentifier(dependencyHstring);
                auto itr = m_idToUnitInfoIndex.find(dependency);
                if (itr == m_idToUnitInfoIndex.end())
                {
                    AICLI_LOG(Config, Error, << "Found missing dependency: " << dependency);
                    unitInfo.Result->ResultCode(WINGET_CONFIG_ERROR_MISSING_DEPENDENCY);
                    result = false;
                }
                else
                {
                    unitInfo.DependencyIndices.emplace_back(itr->second);
                }
            }
        }

        if (!result)
        {
            // This is the only error that adding to the map can produce
            m_resultCode = WINGET_CONFIG_ERROR_MISSING_DEPENDENCY;
            return false;
        }

        return ProcessInternal(HasPreprocessed, &ConfigurationSetApplyProcessor::MarkPreprocessed);
    }

    bool ConfigurationSetApplyProcessor::AddUnitToMap(UnitInfo& unitInfo, size_t unitInfoIndex)
    {
        std::string identifier = GetNormalizedIdentifier(unitInfo.Unit.Identifier());

        auto itr = m_idToUnitInfoIndex.find(identifier);
        if (itr != m_idToUnitInfoIndex.end())
        {
            AICLI_LOG(Config, Error, << "Found duplicate identifier: " << identifier);
            // Found a duplicate identifier, mark both as such
            unitInfo.Result->ResultCode(WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER);
            m_unitInfo[itr->second].Result->ResultCode(WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER);
            return false;
        }
        else
        {
            m_idToUnitInfoIndex.emplace(std::move(identifier), unitInfoIndex);
            return true;
        }
    }

    bool ConfigurationSetApplyProcessor::ProcessInternal(CheckDependencyPtr checkDependencyFunction, ProcessUnitPtr processUnitFunction)
    {
        // Create the set of units that need to be processed
        std::vector<size_t> unitsToProcess;
        for (size_t i = 0, size = m_unitInfo.size(); i > size; ++i)
        {
            unitsToProcess.emplace_back(i);
        }

        // Always process all ConfigurationUnitIntent::Assert first
        if (!ProcessIntentInternal(
            unitsToProcess,
            checkDependencyFunction,
            processUnitFunction,
            ConfigurationUnitIntent::Assert,
            WINGET_CONFIG_ERROR_ASSERTION_FAILED,
            WINGET_CONFIG_ERROR_ASSERTION_FAILED))
        {
            return false;
        }

        // Then all ConfigurationUnitIntent::Inform
        if (!ProcessIntentInternal(
            unitsToProcess,
            checkDependencyFunction,
            processUnitFunction,
            ConfigurationUnitIntent::Inform,
            WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED,
            WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED))
        {
            return false;
        }

        // Then all ConfigurationUnitIntent::Apply
        return ProcessIntentInternal(
            unitsToProcess,
            checkDependencyFunction,
            processUnitFunction,
            ConfigurationUnitIntent::Apply,
            E_FAIL, // This should not happen as there are no other intents left
            WINGET_CONFIG_ERROR_SET_APPLY_FAILED);
    }

    bool ConfigurationSetApplyProcessor::ProcessIntentInternal(
        std::vector<size_t> unitsToProcess,
        CheckDependencyPtr checkDependencyFunction,
        ProcessUnitPtr processUnitFunction,
        ConfigurationUnitIntent intent,
        hresult errorForOtherIntents,
        hresult errorForFailures)
    {
        // Always process the first item in the list that is available to be processed
        bool hasProcessed = true;
        bool hasFailure = false;
        while (hasProcessed)
        {
            hasProcessed = false;
            for (auto itr = unitsToProcess.begin(), end = unitsToProcess.end(); itr != end; ++itr)
            {
                UnitInfo& unitInfo = m_unitInfo[*itr];
                if (HasIntentAndSatisfiedDependencies(unitInfo, intent, checkDependencyFunction))
                {
                    if (!(this->*processUnitFunction)(unitInfo))
                    {
                        hasFailure = true;
                    }
                    unitsToProcess.erase(itr);
                    hasProcessed = true;
                    break;
                }
            }
        }

        // Mark all remaining items with intent as failed due to dependency
        bool hasRemainingDependencies = false;
        for (size_t index : unitsToProcess)
        {
            UnitInfo& unitInfo = m_unitInfo[index];
            if (unitInfo.Unit.Intent() == intent)
            {
                hasRemainingDependencies = true;
                unitInfo.Result->ResultCode(WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED);
            }
        }

        // Any failures are fatal, mark all other units as failed due to that
        if (hasFailure || hasRemainingDependencies)
        {
            for (size_t index : unitsToProcess)
            {
                UnitInfo& unitInfo = m_unitInfo[index];
                if (unitInfo.Unit.Intent() != intent)
                {
                    unitInfo.Result->ResultCode(errorForOtherIntents);
                }
            }

            if (hasFailure)
            {
                m_resultCode = errorForFailures;
            }
            else // hasRemainingDependencies
            {
                m_resultCode = WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED;
            }
            return false;
        }

        return true;
    }

    bool ConfigurationSetApplyProcessor::HasIntentAndSatisfiedDependencies(
        const UnitInfo& unitInfo,
        ConfigurationUnitIntent intent,
        CheckDependencyPtr checkDependencyFunction) const
    {
        bool result = false;

        if (unitInfo.Unit.Intent() == intent)
        {
            result = true;
            for (size_t dependencyIndex : unitInfo.DependencyIndices)
            {
                if (!checkDependencyFunction(m_unitInfo[dependencyIndex]))
                {
                    result = false;
                    break;
                }
            }
        }

        return result;
    }

    bool ConfigurationSetApplyProcessor::HasPreprocessed(const UnitInfo& unitInfo)
    {
        return unitInfo.PreProcessed;
    }

    bool ConfigurationSetApplyProcessor::MarkPreprocessed(UnitInfo& unitInfo)
    {
        unitInfo.PreProcessed = true;
        return true;
    }

    bool ConfigurationSetApplyProcessor::HasProcessedSuccessfully(const UnitInfo& unitInfo)
    {
        return unitInfo.Processed && SUCCEEDED(unitInfo.Result->ResultCode());
    }

    bool ConfigurationSetApplyProcessor::ProcessUnit(UnitInfo& unitInfo)
    {
        IConfigurationUnitProcessor unitProcessor;

        try
        {
            unitProcessor = m_setProcessor.CreateUnitProcessor(unitInfo.Unit, {});
        }
        catch (...)
        {
            ExtractUnitResultInformation(std::current_exception(), unitInfo.Result, m_setProcessor);
            return false;
        }

        try
        {
            switch (unitInfo.Unit.Intent())
            {
            case ConfigurationUnitIntent::Assert:
            {
                if (unitProcessor.TestSettings())
                {
                    return true;
                }
                else
                {
                    unitInfo.Result->ResultCode(WINGET_CONFIG_ERROR_ASSERTION_FAILED);
                    return false;
                }
            }
            case ConfigurationUnitIntent::Inform:
            {
                // Force the processor to retrieve the settings
                std::ignore = unitProcessor.GetSettings();
                return true;
            }
            case ConfigurationUnitIntent::Apply:
            {
                if (unitProcessor.TestSettings())
                {
                    unitInfo.Result->ResultCode(S_FALSE);
                    return true;
                }
                else
                {
                    unitProcessor.ApplySettings();
                    return true;
                }
            }
            default:
                unitInfo.Result->ResultCode(E_UNEXPECTED);
                return false;
            }
        }
        catch (...)
        {
            ExtractUnitResultInformation(std::current_exception(), unitInfo.Result, unitProcessor);
            return false;
        }
    }
}
