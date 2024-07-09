// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Database/ConfigurationDatabase.h"
#include <winrt/Microsoft.Management.Configuration.h>
#include <map>
#include <memory>
#include <mutex>
#include <vector>


namespace winrt::Microsoft::Management::Configuration::implementation
{
    // Forward declarations
    struct ConfigurationProcessor;
    struct ConfigurationSet;

    namespace details
    {
        struct ChangeListener;
        struct ChangeSignaler;
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
            SetChangeRegistration(const winrt::guid& instanceIdentifier);

            SetChangeRegistration(const SetChangeRegistration&) = delete;
            SetChangeRegistration& operator=(const SetChangeRegistration&) = delete;

            SetChangeRegistration(SetChangeRegistration&&) = delete;
            SetChangeRegistration& operator=(SetChangeRegistration&&) = delete;

            ~SetChangeRegistration();

        private:
            std::shared_ptr<ConfigurationStatus> m_status;
            winrt::guid m_instanceIdentifier;
        };

        std::shared_ptr<SetChangeRegistration> RegisterForSetChange(const ConfigurationSet& set);
        void RemoveSetChangeRegistration(const winrt::guid& instanceIdentifier) noexcept;

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

        std::shared_ptr<ChangeRegistration> RegisterForChange(const ConfigurationProcessor& processor);
        void RemoveChangeRegistration(const winrt::guid& instanceIdentifier) noexcept;

    private:
        ConfigurationStatus();

        void EnableChangeListeningIfNeeded();
        void DisableChangeListeningIfNeeded();

        // Indicate that a change occurred, receiving the previous change identifier value.
        // Returns the latest change identifier.
        int64_t ChangeDetected(int64_t previousChangeIdentifier);

        std::shared_ptr<details::ChangeSignaler> GetChangeSignaler();

        ConfigurationDatabase m_database;

        std::mutex m_changeRegistrationsMutex;
        std::map<winrt::guid, ConfigurationSet*> m_setChangeRegistrations;
        std::vector<std::pair<winrt::guid, ConfigurationProcessor*>> m_changeRegistrations;
        std::unique_ptr<details::ChangeListener> m_changeListener;

        std::shared_ptr<details::ChangeSignaler> m_changeSignaler;
    };
}
