// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Database/ConfigurationDatabase.h"
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
        friend details::ChangeListener;

        ConfigurationStatus(const ConfigurationStatus&) = delete;
        ConfigurationStatus& operator=(const ConfigurationStatus&) = delete;

        ConfigurationStatus(ConfigurationStatus&&) = delete;
        ConfigurationStatus& operator=(ConfigurationStatus&&) = delete;

        ~ConfigurationStatus();

        // Gets the singleton instance.
        static std::shared_ptr<ConfigurationStatus> Instance();

        // Get various set state information
        ConfigurationSetState GetSetState(const winrt::guid& instanceIdentifier);
        clock::time_point GetSetFirstApply(const winrt::guid& instanceIdentifier);
        clock::time_point GetSetApplyBegun(const winrt::guid& instanceIdentifier);
        clock::time_point GetSetApplyEnded(const winrt::guid& instanceIdentifier);
        ConfigurationUnitState GetUnitState(const winrt::guid& instanceIdentifier);
        IConfigurationUnitResultInformation GetUnitResultInformation(const winrt::guid& instanceIdentifier);

        // Keeps data for a set change listener.
        struct SetChangeRegistration
        {
            SetChangeRegistration(const winrt::guid& instanceIdentifier, ConfigurationSet* configurationSet);

            SetChangeRegistration(const SetChangeRegistration&) = delete;
            SetChangeRegistration& operator=(const SetChangeRegistration&) = delete;

            SetChangeRegistration(SetChangeRegistration&&) = delete;
            SetChangeRegistration& operator=(SetChangeRegistration&&) = delete;

            ~SetChangeRegistration();

        private:
            std::shared_ptr<ConfigurationStatus> m_status;
            winrt::guid m_instanceIdentifier;
            ConfigurationSet* m_configurationSet;
        };

        std::shared_ptr<SetChangeRegistration> RegisterForSetChange(ConfigurationSet& set);
        void RemoveSetChangeRegistration(const winrt::guid& instanceIdentifier, ConfigurationSet* configurationSet) noexcept;

        // Keeps data for a change listener.
        struct ChangeRegistration
        {
            ChangeRegistration(const winrt::guid& instanceIdentifier);

            ChangeRegistration(const ChangeRegistration&) = delete;
            ChangeRegistration& operator=(const ChangeRegistration&) = delete;

            ChangeRegistration(ChangeRegistration&&) = delete;
            ChangeRegistration& operator=(ChangeRegistration&&) = delete;

            ~ChangeRegistration();

        private:
            std::shared_ptr<ConfigurationStatus> m_status;
            winrt::guid m_instanceIdentifier;
        };

        std::shared_ptr<ChangeRegistration> RegisterForChange(ConfigurationProcessor& processor);
        void RemoveChangeRegistration(const winrt::guid& instanceIdentifier) noexcept;

    private:
        ConfigurationStatus();

        void EnableChangeListeningIfNeeded();
        void DisableChangeListeningIfNeeded();

        ConfigurationDatabase& Database();

        bool HasChangeRegistrations();

        void SetChangeDetected(const winrt::guid& setInstanceIdentifier, com_ptr<ConfigurationSetChangeData>& data, const std::optional<GUID>& unitInstanceIdentifier);
        void ChangeDetected(Configuration::ConfigurationSet& set, Configuration::ConfigurationChangeData data);

        ConfigurationDatabase m_database;

        std::mutex m_changeRegistrationsMutex;
        std::multimap<winrt::guid, ConfigurationSet*> m_setChangeRegistrations;
        std::vector<std::pair<winrt::guid, ConfigurationProcessor*>> m_changeRegistrations;

        // Keep this last to ensure it is destroyed first
        std::unique_ptr<details::ChangeListener> m_changeListener;
    };
}
