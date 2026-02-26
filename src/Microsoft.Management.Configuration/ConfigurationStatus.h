// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Database/ConfigurationDatabase.h"
#include "ConfigurationSetChangeData.h"
#include <winrt/Microsoft.Management.Configuration.h>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>


namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Forward declarations
    struct ConfigurationProcessor;
    struct ConfigurationSet;
    struct ConfigurationSetChangeData;

    namespace details
    {
        struct ChangeListener;
    }

    // Provides access to overall configuration status information.
    struct ConfigurationStatus
    {
    private:
        struct private_construction {};

    public:
        friend details::ChangeListener;

        ConfigurationStatus(private_construction);

        ConfigurationStatus(const ConfigurationStatus&) = delete;
        ConfigurationStatus& operator=(const ConfigurationStatus&) = delete;

        ConfigurationStatus(ConfigurationStatus&&) = delete;
        ConfigurationStatus& operator=(ConfigurationStatus&&) = delete;

        ~ConfigurationStatus();

        // Gets the singleton instance.
        static std::shared_ptr<ConfigurationStatus> Instance();

        // Get various set state information
        ConfigurationSetState GetSetState(const guid& instanceIdentifier);
        clock::time_point GetSetFirstApply(const guid& instanceIdentifier);
        clock::time_point GetSetApplyBegun(const guid& instanceIdentifier);
        clock::time_point GetSetApplyEnded(const guid& instanceIdentifier);
        ConfigurationUnitState GetUnitState(const guid& instanceIdentifier);
        IConfigurationUnitResultInformation GetUnitResultInformation(const guid& instanceIdentifier);

        // Record state changes
        void UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state);
        void UpdateSetState(const guid& setInstanceIdentifier, bool inQueue);
        void UpdateUnitState(const guid& setInstanceIdentifier, const com_ptr<implementation::ConfigurationSetChangeData>& changeData);

        // Keeps data for a set change listener.
        struct SetChangeRegistration
        {
            SetChangeRegistration(const guid& instanceIdentifier, ConfigurationSet* configurationSet);

            SetChangeRegistration(const SetChangeRegistration&) = delete;
            SetChangeRegistration& operator=(const SetChangeRegistration&) = delete;

            SetChangeRegistration(SetChangeRegistration&&) = delete;
            SetChangeRegistration& operator=(SetChangeRegistration&&) = delete;

            ~SetChangeRegistration();

        private:
            std::shared_ptr<ConfigurationStatus> m_status;
            guid m_instanceIdentifier;
            ConfigurationSet* m_configurationSet;
        };

        std::shared_ptr<SetChangeRegistration> RegisterForSetChange(ConfigurationSet& set);
        void RemoveSetChangeRegistration(const guid& instanceIdentifier, ConfigurationSet* configurationSet) noexcept;

        // Keeps data for a change listener.
        struct ChangeRegistration
        {
            ChangeRegistration(const guid& instanceIdentifier);

            ChangeRegistration(const ChangeRegistration&) = delete;
            ChangeRegistration& operator=(const ChangeRegistration&) = delete;

            ChangeRegistration(ChangeRegistration&&) = delete;
            ChangeRegistration& operator=(ChangeRegistration&&) = delete;

            ~ChangeRegistration();

        private:
            std::shared_ptr<ConfigurationStatus> m_status;
            guid m_instanceIdentifier;
        };

        std::shared_ptr<ChangeRegistration> RegisterForChange(ConfigurationProcessor& processor);
        void RemoveChangeRegistration(const guid& instanceIdentifier) noexcept;

    private:
        void EnableChangeListeningIfNeeded();
        void DisableChangeListeningIfNeeded();

        void SignalChangeListeners();

        ConfigurationDatabase& Database();

        bool HasSetChangeRegistration(const guid& setInstanceIdentifier);
        bool HasChangeRegistrations();

        void SetChangeDetected(const guid& setInstanceIdentifier, com_ptr<ConfigurationSetChangeData>& data, const std::optional<GUID>& unitInstanceIdentifier);
        void ChangeDetected(const Configuration::ConfigurationSet& set, const Configuration::ConfigurationChangeData& data);

        ConfigurationDatabase m_database;

        std::mutex m_changeRegistrationsMutex;
        std::multimap<guid, ConfigurationSet*> m_setChangeRegistrations;
        std::vector<std::pair<guid, ConfigurationProcessor*>> m_changeRegistrations;

        // Keep this last to ensure it is destroyed first
        std::unique_ptr<details::ChangeListener> m_changeListener;
    };
}
