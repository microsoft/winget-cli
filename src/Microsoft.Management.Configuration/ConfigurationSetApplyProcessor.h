// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ConfigurationSet.h"
#include "ConfigurationUnit.h"
#include "ApplyGroupSettingsResult.h"
#include "ApplyConfigurationUnitResult.h"
#include "ConfigurationUnitResultInformation.h"
#include <winget/AsyncTokens.h>

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
        using ConfigurationSetChangeData = Configuration::ConfigurationSetChangeData;

        using result_type = decltype(make_self<wil::details::module_count_wrapper<implementation::ApplyGroupSettingsResult>>());
        using progress_type = AppInstaller::WinRT::AsyncProgress<IApplyGroupSettingsResult, IApplyGroupMemberSettingsResult>;

        ConfigurationSetApplyProcessor(const ConfigurationSet& configurationSet, IConfigurationSetProcessor setProcessor, progress_type&& progress);

        // Processes the apply for the configuration set.
        void Process(bool preProcessOnly = false);

        // Gets the result object.
        IApplyGroupSettingsResult Result() const;

    private:
        // Contains all of the relevant data for a configuration unit.
        struct UnitInfo
        {
            UnitInfo(const ConfigurationUnit& unit);

            ConfigurationUnit Unit;
            std::vector<size_t> DependencyIndices;
            decltype(make_self<wil::details::module_count_wrapper<implementation::ApplyConfigurationUnitResult>>()) Result;
            decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>()) ResultInformation;
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

        // Return true to process these intents, false to skip them.
        using IntentFilterPtr = bool (*)(ConfigurationUnitIntent);

        // Runs the processing using the given functions.
        bool ProcessInternal(CheckDependencyPtr checkDependencyFunction, ProcessUnitPtr processUnitFunction, bool sendProgress = false);

        // Processes one of the non-writing intent types, which are fatal if not all successful
        bool ProcessIntentInternal(
            std::vector<size_t>& unitsToProcess,
            CheckDependencyPtr checkDependencyFunction,
            ProcessUnitPtr processUnitFunction,
            IntentFilterPtr intentFilter,
            hresult errorForOtherIntents,
            hresult errorForFailures,
            bool sendProgress);

        // Determines if the given unit has the given intent and all of its dependencies are satisfied
        bool HasIntentAndSatisfiedDependencies(
            const UnitInfo& unitInfo,
            IntentFilterPtr intentFilter,
            CheckDependencyPtr checkDependencyFunction) const;

        // Checks a dependency for preprocessing.
        static bool HasPreprocessed(const UnitInfo& unitInfo);

        // Marks a unit as preprocessed.
        bool MarkPreprocessed(UnitInfo& unitInfo);

        // Checks a dependency for having processed successfully.
        static bool HasProcessedSuccessfully(const UnitInfo& unitInfo);

        // Processes a configuration unit per its intent.
        bool ProcessUnit(UnitInfo& unitInfo);

        // Sends progress
        // TODO: Eventually these functions/call sites will be used for history
        void SendProgress(ConfigurationUnitState state, const UnitInfo& unitInfo);
        void SendProgressIfNotComplete(ConfigurationUnitState state, const UnitInfo& unitInfo);

        ConfigurationSet m_configurationSet;
        IConfigurationSetProcessor m_setProcessor;
        result_type m_result;
        progress_type m_progress;
        std::vector<UnitInfo> m_unitInfo;
        std::map<std::string, size_t> m_idToUnitInfoIndex;
        hresult m_resultCode;
    };
}
