// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationStatus.h"
#include "ConfigurationChangeData.h"
#include "ConfigurationProcessor.h"
#include "ConfigurationSet.h"
#include "ConfigurationUnitResultInformation.h"
#include <AppInstallerStrings.h>
#include <AppInstallerLanguageUtilities.h>


namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace details
    {
        // Implements the consuming side of the status signaling.
        struct ChangeListener
        {
            struct SetStatusItem
            {
                ConfigurationDatabase::StatusItem Status;
                com_ptr<implementation::ConfigurationSet> Set;
            };

            ChangeListener(ConfigurationStatus& status) : m_status(status)
            {
                ConfigurationDatabase::StatusBaseline baseline = m_status.Database().GetStatusBaseline();
                m_changeIdentifier = baseline.ChangeIdentifier;

                for (const auto& item : baseline.SetStatus)
                {
                    m_lastSetStatus.emplace(item.SetInstanceIdentifier, SetStatusItem{ item });
                }

                std::wstring objectName = L"WinGetConfigListener_" + AppInstaller::Utility::CreateNewGuidNameWString();
                m_listenerEventName = AppInstaller::Utility::ConvertToUTF8(objectName);
                m_listenerEvent.create(wil::EventOptions::None, objectName.c_str());

                m_status.Database().AddListener(m_listenerEventName);

                m_threadPoolWait.reset(CreateThreadpoolWait(StaticWaitCallback, this, nullptr));
                THROW_LAST_ERROR_IF(!m_threadPoolWait);

                SetThreadpoolWait(m_threadPoolWait.get(), m_listenerEvent.get(), NULL);
            }

            ~ChangeListener()
            {
                try
                {
                    m_status.Database().RemoveListener(m_listenerEventName);
                }
                CATCH_LOG();
            }

        private:
            static void NTAPI StaticWaitCallback(PTP_CALLBACK_INSTANCE, void* context, TP_WAIT*, TP_WAIT_RESULT)
            {
                reinterpret_cast<ChangeListener*>(context)->WaitCallback();
            }

            void WaitCallback() try
            {
                std::vector<ConfigurationDatabase::StatusItem> changes = m_status.Database().GetStatusSince(m_changeIdentifier);

                // Convert status items to relevant change information
                for (const auto& change : changes)
                {
                    if (change.UnitInstanceIdentifier)
                    {
                        if (m_status.HasSetChangeRegistration(change.SetInstanceIdentifier))
                        {
                            // A unit status change
                            ConfigurationUnitState state = AppInstaller::ToEnum<ConfigurationUnitState>(change.State);

                            decltype(make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>()) resultInformation;

                            if (change.ResultCode)
                            {
                                resultInformation = make_self<wil::details::module_count_wrapper<implementation::ConfigurationUnitResultInformation>>();
                                resultInformation->ResultCode(change.ResultCode.value());
                                resultInformation->Description(hstring{ AppInstaller::Utility::ConvertToUTF16(change.ResultDescription) });
                                resultInformation->Details(hstring{ AppInstaller::Utility::ConvertToUTF16(change.ResultDetails) });
                                resultInformation->ResultSource(change.ResultSource);
                            }

                            auto changeData = make_self<implementation::ConfigurationSetChangeData>();
                            changeData->Initialize(state, *resultInformation, nullptr);

                            m_status.SetChangeDetected(change.SetInstanceIdentifier, changeData, change.UnitInstanceIdentifier);
                        }
                    }
                    else
                    {
                        // A set status change
                        ConfigurationSetState state = AppInstaller::ToEnum<ConfigurationSetState>(change.State);
                        ConfigurationChangeEventType changeType = ConfigurationChangeEventType::Unknown;

                        SetStatusItem* setStatusItem = nullptr;
                        auto itr = m_lastSetStatus.find(change.SetInstanceIdentifier);
                        if (itr != m_lastSetStatus.end())
                        {
                            setStatusItem = &itr->second;
                        }

                        if (!setStatusItem)
                        {
                            changeType = ConfigurationChangeEventType::SetAdded;

                            std::tie(itr, std::ignore) = m_lastSetStatus.emplace(change.SetInstanceIdentifier, SetStatusItem{ change });
                            setStatusItem = &itr->second;
                        }
                        else
                        {
                            changeType = (change.InQueue ? ConfigurationChangeEventType::SetStateChanged : ConfigurationChangeEventType::SetRemoved);
                        }

                        if (m_status.HasChangeRegistrations())
                        {
                            if (!setStatusItem->Set)
                            {
                                setStatusItem->Set = m_status.Database().GetSet(change.SetInstanceIdentifier);
                            }

                            auto changeData = make_self<wil::details::module_count_wrapper<implementation::ConfigurationChangeData>>();
                            changeData->Initialize(changeType, change.SetInstanceIdentifier, state);

                            m_status.ChangeDetected(*setStatusItem->Set, *changeData);
                        }

                        auto setChangeData = make_self<implementation::ConfigurationSetChangeData>();
                        setChangeData->Initialize(state);

                        m_status.SetChangeDetected(change.SetInstanceIdentifier, setChangeData, std::nullopt);
                    }

                    m_changeIdentifier = change.ChangeIdentifier;
                }

                SetThreadpoolWait(m_threadPoolWait.get(), m_listenerEvent.get(), NULL);
            }
            CATCH_LOG_MSG("ChangeListener::WaitCallback exception");

            ConfigurationStatus& m_status;
            int64_t m_changeIdentifier;
            std::map<winrt::guid, SetStatusItem> m_lastSetStatus;
            wil::unique_event m_listenerEvent;
            std::string m_listenerEventName;

            // Keep last to destroy first
            wil::unique_threadpool_wait m_threadPoolWait;
        };
    }

    ConfigurationStatus::ConfigurationStatus(private_construction) {}

    ConfigurationStatus::~ConfigurationStatus() = default;

    std::shared_ptr<ConfigurationStatus> ConfigurationStatus::Instance()
    {
        static std::shared_ptr<ConfigurationStatus> s_instance;

        std::shared_ptr<ConfigurationStatus> result = std::atomic_load(&s_instance);
        if (!result)
        {
            result = std::make_shared<ConfigurationStatus>(private_construction{});
            std::shared_ptr<ConfigurationStatus> empty;

            if (!std::atomic_compare_exchange_strong(&s_instance, &empty, result))
            {
                result = empty;
            }
        }

        return result;
    }

    ConfigurationSetState ConfigurationStatus::GetSetState(const winrt::guid& instanceIdentifier)
    {
        m_database.EnsureOpened(false);
        return m_database.GetSetState(instanceIdentifier);
    }

    clock::time_point ConfigurationStatus::GetSetFirstApply(const winrt::guid& instanceIdentifier)
    {
        m_database.EnsureOpened(false);
        return clock::from_sys(m_database.GetSetFirstApply(instanceIdentifier));
    }

    clock::time_point ConfigurationStatus::GetSetApplyBegun(const winrt::guid& instanceIdentifier)
    {
        using system_clock = std::chrono::system_clock;

        m_database.EnsureOpened(false);
        system_clock::time_point result = m_database.GetSetApplyBegun(instanceIdentifier);
        return (result == system_clock::time_point{} ? clock::time_point{} : clock::from_sys(result));
    }

    clock::time_point ConfigurationStatus::GetSetApplyEnded(const winrt::guid& instanceIdentifier)
    {
        using system_clock = std::chrono::system_clock;

        m_database.EnsureOpened(false);
        system_clock::time_point result = m_database.GetSetApplyEnded(instanceIdentifier);
        return (result == system_clock::time_point{} ? clock::time_point{} : clock::from_sys(result));
    }

    ConfigurationUnitState ConfigurationStatus::GetUnitState(const winrt::guid& instanceIdentifier)
    {
        m_database.EnsureOpened(false);
        return m_database.GetUnitState(instanceIdentifier);
    }

    IConfigurationUnitResultInformation ConfigurationStatus::GetUnitResultInformation(const winrt::guid& instanceIdentifier)
    {
        m_database.EnsureOpened(false);
        return m_database.GetUnitResultInformation(instanceIdentifier);
    }

    void ConfigurationStatus::UpdateSetState(const guid& setInstanceIdentifier, ConfigurationSetState state)
    {
        m_database.EnsureOpened();
        m_database.UpdateSetState(setInstanceIdentifier, state);
        SignalChangeListeners();
    }

    void ConfigurationStatus::UpdateSetState(const guid& setInstanceIdentifier, bool inQueue)
    {
        m_database.EnsureOpened();
        m_database.UpdateSetInQueue(setInstanceIdentifier, inQueue);
        SignalChangeListeners();
    }

    void ConfigurationStatus::UpdateUnitState(const guid& setInstanceIdentifier, const com_ptr<implementation::ConfigurationSetChangeData>& changeData)
    {
        m_database.EnsureOpened();
        m_database.UpdateUnitState(setInstanceIdentifier, changeData);
        SignalChangeListeners();
    }

    ConfigurationStatus::SetChangeRegistration::SetChangeRegistration(const winrt::guid& instanceIdentifier, ConfigurationSet* configurationSet) :
        m_status(Instance()), m_instanceIdentifier(instanceIdentifier), m_configurationSet(configurationSet) {}

    ConfigurationStatus::SetChangeRegistration::~SetChangeRegistration()
    {
        m_status->RemoveSetChangeRegistration(m_instanceIdentifier, m_configurationSet);
    }

    std::shared_ptr<ConfigurationStatus::SetChangeRegistration> ConfigurationStatus::RegisterForSetChange(ConfigurationSet& set)
    {
        m_database.EnsureOpened();

        winrt::guid instanceIdentifier = set.InstanceIdentifier();

        {
            std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };
            m_setChangeRegistrations.emplace(instanceIdentifier, &set);
            EnableChangeListeningIfNeeded();
        }

        return std::make_shared<SetChangeRegistration>(instanceIdentifier, &set);
    }

    void ConfigurationStatus::RemoveSetChangeRegistration(const winrt::guid& instanceIdentifier, ConfigurationSet* configurationSet) noexcept
    {
        std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };

        auto [begin, end] = m_setChangeRegistrations.equal_range(instanceIdentifier);

        for (; begin != end; ++begin)
        {
            if (begin->second == configurationSet)
            {
                m_setChangeRegistrations.erase(begin);
                break;
            }
        }

        DisableChangeListeningIfNeeded();
    }

    ConfigurationStatus::ChangeRegistration::ChangeRegistration(const winrt::guid& instanceIdentifier) :
        m_status(Instance()), m_instanceIdentifier(instanceIdentifier) {}

    ConfigurationStatus::ChangeRegistration::~ChangeRegistration()
    {
        m_status->RemoveChangeRegistration(m_instanceIdentifier);
    }

    std::shared_ptr<ConfigurationStatus::ChangeRegistration> ConfigurationStatus::RegisterForChange(ConfigurationProcessor& processor)
    {
        m_database.EnsureOpened();

        GUID instanceIdentifier;
        std::ignore = CoCreateGuid(&instanceIdentifier);

        {
            std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };
            m_changeRegistrations.emplace_back(instanceIdentifier, &processor);
            EnableChangeListeningIfNeeded();
        }

        return std::make_shared<ChangeRegistration>(instanceIdentifier);
    }

    void ConfigurationStatus::RemoveChangeRegistration(const winrt::guid& instanceIdentifier) noexcept
    {
        std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };

        for (auto itr = m_changeRegistrations.begin(); itr != m_changeRegistrations.end(); ++itr)
        {
            if (itr->first == instanceIdentifier)
            {
                m_changeRegistrations.erase(itr);
                DisableChangeListeningIfNeeded();
                return;
            }
        }
    }

    void ConfigurationStatus::EnableChangeListeningIfNeeded()
    {
        if (!m_changeListener)
        {
            m_changeListener = std::make_unique<details::ChangeListener>(*this);
        }
    }

    void ConfigurationStatus::DisableChangeListeningIfNeeded()
    {
        if (m_changeListener && m_setChangeRegistrations.empty() && m_changeRegistrations.empty())
        {
            m_changeListener.reset();
        }
    }

    void ConfigurationStatus::SignalChangeListeners()
    {
        std::vector<ConfigurationDatabase::StatusChangeListener> changeListeners = m_database.GetChangeListeners();

        for (const auto& listener : changeListeners)
        {
            std::wstring objectName = AppInstaller::Utility::ConvertToUTF16(listener.ObjectName);
            wil::unique_event listenerEvent;
            if (listenerEvent.try_open(objectName.c_str(), EVENT_MODIFY_STATE))
            {
                listenerEvent.SetEvent();
            }
            else
            {
                m_database.RemoveListener(listener.ObjectName);
            }
        }
    }

    ConfigurationDatabase& ConfigurationStatus::Database()
    {
        return m_database;
    }

    bool ConfigurationStatus::HasSetChangeRegistration(const guid& setInstanceIdentifier)
    {
        std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };
        auto [begin, end] = m_setChangeRegistrations.equal_range(setInstanceIdentifier);
        return begin != end;
    }

    bool ConfigurationStatus::HasChangeRegistrations()
    {
        std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };
        return !m_changeRegistrations.empty();
    }

    void ConfigurationStatus::SetChangeDetected(const winrt::guid& setInstanceIdentifier, com_ptr<ConfigurationSetChangeData>& data, const std::optional<GUID>& unitInstanceIdentifier)
    {
        std::vector<ConfigurationSet*> setChangeRegistrations;

        {
            std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };

            auto [begin, end] = m_setChangeRegistrations.equal_range(setInstanceIdentifier);

            for (; begin != end; ++begin)
            {
                setChangeRegistrations.emplace_back(begin->second);
            }
        }

        for (ConfigurationSet* set : setChangeRegistrations)
        {
            set->ConfigurationSetChange(data, unitInstanceIdentifier);
        }
    }

    void ConfigurationStatus::ChangeDetected(const Configuration::ConfigurationSet& set, const Configuration::ConfigurationChangeData& data)
    {
        std::vector<std::pair<winrt::guid, ConfigurationProcessor*>> changeRegistrations;

        {
            std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };
            changeRegistrations = m_changeRegistrations;
        }

        for (const auto& registration : changeRegistrations)
        {
            registration.second->ConfigurationChange(set, data);
        }
    }
}
