// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSet.h"
#include "ConfigurationUnit.h"
#include "ApplyConfigurationUnitResult.h"
#include "ConfigurationUnitResultInformation.h"

#include <map>
#include <string>
#include <vector>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    // A helper to better organize the configuration set Apply.
    struct ConfigurationSetApplyProcessor
    {
        using ConfigurationSet = Configuration::ConfigurationSet;
        using ConfigurationUnit = Configuration::ConfigurationUnit;

        ConfigurationSetApplyProcessor(const ConfigurationSet& configurationSet, IConfigurationSetProcessor&& setProcessor);

        // Processes the apply for the configuration set.
        void Process();

        // Gets the unit results from the processing.
        std::vector<Configuration::ApplyConfigurationUnitResult> GetUnitResults() const;

        // Gets the overall result code from the processing.
        hresult ResultCode() const;

    private:
        // Contains all of the relevant data for a configuration unit.
        struct UnitInfo
        {
            UnitInfo(const ConfigurationUnit& unit);

            ConfigurationUnit Unit;
            std::vector<size_t> DependencyIndices;
            decltype(make_self<wil::details::module_count_wrapper<ConfigurationUnitResultInformation>>()) Result;
            bool PreviouslyInDesiredState = false;
            bool PreProcessed = false;
            bool Processed = false;
        };

        // Builds out some data used during processing and validates the set along the way.
        bool PreProcess();

        // Adds the given unit to the identifier to unit info index map.
        bool AddUnitToMap(UnitInfo& unitInfo, size_t unitInfoIndex);

        // Checks the dependency; returns true to indicate that the dependency is satisfied, false if not.
        using CheckDependencyPtr = bool (*)(const UnitInfo&);

        // Processes the unit; returns true if successful, false if not.
        using ProcessUnitPtr = bool (ConfigurationSetApplyProcessor::*)(UnitInfo&);

        // Runs the processing using the given functions.
        bool ProcessInternal(CheckDependencyPtr checkDependencyFunction, ProcessUnitPtr processUnitFunction);

        // Processes one of the non-writing intent types, which are fatal if not all successful
        bool ProcessIntentInternal(
            std::vector<size_t> unitsToProcess,
            CheckDependencyPtr checkDependencyFunction,
            ProcessUnitPtr processUnitFunction,
            ConfigurationUnitIntent intent,
            hresult errorForOtherIntents,
            hresult errorForFailures);

        // Determines if the given unit has the given intent and all of its dependencies are satisfied
        bool HasIntentAndSatisfiedDependencies(
            const UnitInfo& unitInfo,
            ConfigurationUnitIntent intent,
            CheckDependencyPtr checkDependencyFunction) const;

        // Checks a dependency for preprocessing.
        static bool HasPreprocessed(const UnitInfo& unitInfo);

        // Marks a unit as preprocessed.
        bool MarkPreprocessed(UnitInfo& unitInfo);

        // Checks a dependency for having processed successfully.
        static bool HasProcessedSuccessfully(const UnitInfo& unitInfo);

        // Processes a configuration unit per its intent.
        bool ProcessUnit(UnitInfo& unitInfo);

        IConfigurationSetProcessor m_setProcessor;
        std::vector<UnitInfo> m_unitInfo;
        std::map<std::string, size_t> m_idToUnitInfoIndex;
        hresult m_resultCode;
    };
}
