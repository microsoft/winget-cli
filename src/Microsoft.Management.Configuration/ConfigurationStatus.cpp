// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ConfigurationStatus.h"
#include "ConfigurationProcessor.h"
#include "ConfigurationSet.h"


namespace winrt::Microsoft::Management::Configuration::implementation
{
    namespace details
    {
#define WINGET_CHANGE_SYNC_EVENT_NAME_1 L"WinGetChangeSyncEvent1"
#define WINGET_CHANGE_SYNC_EVENT_NAME_2 L"WinGetChangeSyncEvent2"

        struct ChangeSynchronizationBase
        {
            ChangeSynchronizationBase()
            {
                m_events[0].create(wil::EventOptions::ManualReset, WINGET_CHANGE_SYNC_EVENT_NAME_1);
                m_events[1].create(wil::EventOptions::ManualReset, WINGET_CHANGE_SYNC_EVENT_NAME_2);
            }

            wil::unique_event& CurrentEvent()
            {
                return m_events[m_currentEvent];
            }

            wil::unique_event& NextEvent()
            {
                m_currentEvent = (m_currentEvent + 1) % m_events.size();
                return CurrentEvent();
            }

        private:
            size_t m_currentEvent = 0;
            std::array<wil::unique_event, 2> m_events;
        };

        // Implements the consuming side of the status signaling.
        struct ChangeListener : protected ChangeSynchronizationBase
        {
            ChangeListener(ConfigurationStatus& status, int64_t changeIdentifier) : m_status(status), m_changeIdentifier(changeIdentifier)
            {
                m_threadPoolWait.reset(CreateThreadpoolWait(StaticWaitCallback, this, nullptr));
                THROW_LAST_ERROR_IF(!m_threadPoolWait);

                SetThreadpoolWait(m_threadPoolWait.get(), CurrentEvent().get(), NULL);
            }

        private:
            static void StaticWaitCallback(PTP_CALLBACK_INSTANCE, void* context, TP_WAIT*, TP_WAIT_RESULT)
            {
                reinterpret_cast<ChangeListener*>(context)->WaitCallback();
            }

            void WaitCallback()
            {
                SetThreadpoolWait(m_threadPoolWait.get(), NextEvent().get(), NULL);

                if (ShouldBecomeUpdateThread())
                {
                    do
                    {

                    } while (ShouldContinueUpdateThread());
                }
            }

            bool ShouldBecomeUpdateThread()
            {
                size_t state =  m_workState.load();
                while (state < 2)
                {
                    size_t newState = state + 1;
                    m_workState.compare_exchange_strong(state, newState);
                }

                return state == 0;
            }

            bool ShouldContinueUpdateThread()
            {
                return m_workState.fetch_sub(1) != 1;
            }

            ConfigurationStatus& m_status;
            int64_t m_changeIdentifier;
            wil::unique_threadpool_wait m_threadPoolWait;
            std::atomic_size_t m_workState;
        };

        // Implements the producing side of the status signaling
        struct ChangeSignaler : protected ChangeSynchronizationBase
        {
            ChangeSignaler() = default;

            void PrepareForChange()
            {
                CurrentEvent().ResetEvent();
            }

            void SignalChange()
            {
                NextEvent().SetEvent();
            }
        };
    }

    ConfigurationStatus::ConfigurationStatus() = default;

    ConfigurationStatus::~ConfigurationStatus() = default;

    std::shared_ptr<ConfigurationStatus> ConfigurationStatus::Instance()
    {
        static std::shared_ptr<ConfigurationStatus> s_instance;

        std::shared_ptr<ConfigurationStatus> result = std::atomic_load(&s_instance);
        if (!result)
        {
            result = std::make_shared<ConfigurationStatus>();
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
        return m_database.GetSetFirstApply(instanceIdentifier);
    }

    clock::time_point ConfigurationStatus::GetSetApplyBegun(const winrt::guid& instanceIdentifier)
    {
        m_database.EnsureOpened(false);
        return m_database.GetSetApplyBegun(instanceIdentifier);
    }

    clock::time_point ConfigurationStatus::GetSetApplyEnded(const winrt::guid& instanceIdentifier)
    {
        m_database.EnsureOpened(false);
        return m_database.GetSetApplyEnded(instanceIdentifier);
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

    ConfigurationStatus::SetChangeRegistration::SetChangeRegistration(const winrt::guid& instanceIdentifier) :
        m_status(Instance()), m_instanceIdentifier(instanceIdentifier) {}

    ConfigurationStatus::SetChangeRegistration::~SetChangeRegistration()
    {
        m_status->RemoveSetChangeRegistration(m_instanceIdentifier);
    }

    std::shared_ptr<ConfigurationStatus::SetChangeRegistration> ConfigurationStatus::RegisterForSetChange(const ConfigurationSet& set)
    {
        m_database.EnsureOpened();

        winrt::guid instanceIdentifier = set.InstanceIdentifier();

        {
            std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };
            m_setChangeRegistrations.emplace(instanceIdentifier, &set);
            EnableChangeListeningIfNeeded();
        }

        return std::make_shared<SetChangeRegistration>(instanceIdentifier);
    }

    void ConfigurationStatus::RemoveSetChangeRegistration(const winrt::guid& instanceIdentifier) noexcept
    {
        std::lock_guard<std::mutex> lock{ m_changeRegistrationsMutex };

        m_setChangeRegistrations.erase(instanceIdentifier);
        DisableChangeListeningIfNeeded();
    }

    ConfigurationStatus::ChangeRegistration::ChangeRegistration(const winrt::guid& instanceIdentifier) :
        m_status(Instance()), m_instanceIdentifier(instanceIdentifier) {}

    ConfigurationStatus::ChangeRegistration::~ChangeRegistration()
    {
        m_status->RemoveChangeRegistration(m_instanceIdentifier);
    }

    std::shared_ptr<ConfigurationStatus::ChangeRegistration> ConfigurationStatus::RegisterForChange(const ConfigurationProcessor& processor)
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
            m_changeListener = std::make_unique<details::ChangeListener>(*this, m_database.GetLatestChangeIdentifier());
        }
    }

    void ConfigurationStatus::DisableChangeListeningIfNeeded()
    {
        if (m_changeListener && m_setChangeRegistrations.empty() && m_changeRegistrations.empty())
        {
            m_changeListener.reset();
        }
    }

    int64_t ConfigurationStatus::ChangeDetected(int64_t previousChangeIdentifier)
    {

    }

    std::shared_ptr<details::ChangeSignaler> ConfigurationStatus::GetChangeSignaler()
    {
        std::shared_ptr<details::ChangeSignaler> result = std::atomic_load(&m_changeSignaler);
        if (!result)
        {
            result = std::make_shared<details::ChangeSignaler>();
            std::shared_ptr<details::ChangeSignaler> empty;

            if (!std::atomic_compare_exchange_strong(&m_changeSignaler, &empty, result))
            {
                result = empty;
            }
        }

        return result;
    }
}
