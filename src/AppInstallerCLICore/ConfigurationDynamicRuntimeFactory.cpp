// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include <AppInstallerErrors.h>
#include <AppInstallerLanguageUtilities.h>
#include <AppInstallerStrings.h>
#include <winget/ILifetimeWatcher.h>
#include <winget/Security.h>
#include <winrt/Microsoft.Management.Configuration.SetProcessorFactory.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::Management::Configuration;
using namespace winrt::Windows::Storage;

namespace AppInstaller::CLI::ConfigurationRemoting
{
    namespace anonymous
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        constexpr std::wstring_view EnableTestModeTestGuid = L"1e62d683-2999-44e7-81f7-6f8f35e8d731";
        constexpr std::wstring_view ForceHighIntegrityLevelUnitsTestGuid = L"f698d20f-3584-4f28-bc75-28037e08e651";
        constexpr std::wstring_view EnableRestrictedIntegrityLevelTestGuid = L"5cae3226-185f-4289-815c-3c089d238dc6";

        // Checks the configuration set metadata for a specific test guid that controls the behavior flow.
        bool GetConfigurationSetMetadataOverride(const ConfigurationSet& configurationSet, const std::wstring_view& testGuid)
        {
            auto metadataOverride = configurationSet.Metadata().TryLookup(testGuid);
            if (metadataOverride)
            {
                auto metadataOverrideProperty = metadataOverride.try_as<IPropertyValue>();
                if (metadataOverrideProperty && metadataOverrideProperty.Type() == PropertyType::Boolean)
                {
                    return metadataOverrideProperty.GetBoolean();
                }
            }

            return false;
        }
#endif

        // This is implemented completely in the packaged context for now, if we want to make it more configurable, we will probably want to move it to configuration and
        // have this implementation leverage that one with an event handler for the packaged specifics.
        // TODO: Add SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties and pass values along to sets on creation
        //       In turn, any properties must only be set via the command line (or eventual UI requests to the user).
        struct DynamicFactory : winrt::implements<DynamicFactory, IConfigurationSetProcessorFactory, SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties, winrt::cloaked<WinRT::ILifetimeWatcher>>, WinRT::LifetimeWatcherBase
        {
            DynamicFactory();

            IConfigurationSetProcessor CreateSetProcessor(const ConfigurationSet& configurationSet);

            winrt::event_token Diagnostics(const EventHandler<IDiagnosticInformation>& handler);
            void Diagnostics(const winrt::event_token& token) noexcept;

            DiagnosticLevel MinimumLevel();
            void MinimumLevel(DiagnosticLevel value);

            HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher);

            IConfigurationSetProcessorFactory& DefaultFactory();

            void SendDiagnostics(const IDiagnosticInformation& information);

            Collections::IVectorView<winrt::hstring> AdditionalModulePaths() const
            {
                THROW_HR(E_NOTIMPL);
            }

            void AdditionalModulePaths(const Collections::IVectorView<winrt::hstring>&)
            {
                THROW_HR(E_NOTIMPL);
            }

            SetProcessorFactory::PwshConfigurationProcessorPolicy Policy() const
            {
                THROW_HR(E_NOTIMPL);
            }

            void Policy(SetProcessorFactory::PwshConfigurationProcessorPolicy)
            {
                THROW_HR(E_NOTIMPL);
            }

            SetProcessorFactory::PwshConfigurationProcessorLocation Location() const
            {
                return m_location;
            }

            void Location(SetProcessorFactory::PwshConfigurationProcessorLocation value)
            {
                auto pwshFactory = m_defaultRemoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>();
                pwshFactory.Location(value);
                m_location = value;
            }

            winrt::hstring CustomLocation() const
            {
                return m_customLocation;
            }

            void CustomLocation(winrt::hstring value)
            {
                auto pwshFactory = m_defaultRemoteFactory.as<SetProcessorFactory::IPwshConfigurationSetProcessorFactoryProperties>();
                pwshFactory.CustomLocation(value);
                m_customLocation = value;
            }

        private:
            IConfigurationSetProcessorFactory m_defaultRemoteFactory;
            winrt::event<EventHandler<IDiagnosticInformation>> m_diagnostics;
            IConfigurationSetProcessorFactory::Diagnostics_revoker m_factoryDiagnosticsEventRevoker;
            std::mutex m_diagnosticsMutex;
            DiagnosticLevel m_minimumLevel = DiagnosticLevel::Informational;
            SetProcessorFactory::PwshConfigurationProcessorLocation m_location = SetProcessorFactory::PwshConfigurationProcessorLocation::Default;
            winrt::hstring m_customLocation;
        };

        struct DynamicProcessorInfo
        {
            IConfigurationSetProcessorFactory Factory;
            IConfigurationSetProcessor Processor;
            IConfigurationSetProcessorFactory::Diagnostics_revoker DiagnosticsEventRevoker;
        };

        struct DynamicSetProcessor : winrt::implements<DynamicSetProcessor, IConfigurationSetProcessor>
        {
            using ProcessorMap = std::map<Security::IntegrityLevel, DynamicProcessorInfo>;

            DynamicSetProcessor(winrt::com_ptr<DynamicFactory> dynamicFactory, IConfigurationSetProcessor defaultRemoteSetProcessor, const ConfigurationSet& configurationSet) :
                m_dynamicFactory(std::move(dynamicFactory)), m_configurationSet(configurationSet)
            {
#ifndef AICLI_DISABLE_TEST_HOOKS
                m_enableTestMode = GetConfigurationSetMetadataOverride(m_configurationSet, EnableTestModeTestGuid);
                m_enableRestrictedIntegrityLevel = GetConfigurationSetMetadataOverride(m_configurationSet, EnableRestrictedIntegrityLevelTestGuid);
                m_forceHighIntegrityLevelUnits = GetConfigurationSetMetadataOverride(m_configurationSet, ForceHighIntegrityLevelUnitsTestGuid);

                m_currentIntegrityLevel = m_enableTestMode ? Security::IntegrityLevel::Medium : Security::GetEffectiveIntegrityLevel();
#else
                m_currentIntegrityLevel = Security::GetEffectiveIntegrityLevel();
#endif

                m_setIntegrityLevel = m_currentIntegrityLevel;
                m_setIntegrityLevel = SecurityContextToIntegrityLevel(m_configurationSet.Environment().Context());

                // Check for multiple integrity level requirements
                bool multipleIntegrityLevels = false;
                bool higherIntegrityLevelsThanCurrent = false;
                for (const auto& environment : m_configurationSet.GetUnitEnvironments())
                {
                    auto integrityLevel = SecurityContextToIntegrityLevel(environment.Context());
                    if (integrityLevel != m_currentIntegrityLevel)
                    {
                        multipleIntegrityLevels = true;

                        if (ToIntegral(m_currentIntegrityLevel) < ToIntegral(integrityLevel))
                        {
                            higherIntegrityLevelsThanCurrent = true;
                            break;
                        }
                    }
                }

                // Prevent supplied parameters from crossing integrity levels
                for (const auto& parameter : m_configurationSet.Parameters())
                {
                    if (parameter.ProvidedValue() != nullptr)
                    {
                        THROW_HR_IF(WINGET_CONFIG_ERROR_PARAMETER_INTEGRITY_BOUNDARY, higherIntegrityLevelsThanCurrent || (multipleIntegrityLevels && parameter.IsSecure()));
                    }
                }

                m_setProcessors.emplace(m_currentIntegrityLevel, DynamicProcessorInfo{ m_dynamicFactory->DefaultFactory(), defaultRemoteSetProcessor});
            }

            IConfigurationUnitProcessorDetails GetUnitProcessorDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags)
            {
                // Always get processor details from the current integrity level
                return m_setProcessors[m_currentIntegrityLevel].Processor.GetUnitProcessorDetails(unit, detailFlags);
            }

            // Creates a configuration unit processor for the given unit.
            IConfigurationUnitProcessor CreateUnitProcessor(const ConfigurationUnit& unit)
            {
                // Determine and create set processors for all required integrity levels.
                // Doing this here avoids creating them if the only call is going to be for details (ex. `configure show`) 
                std::call_once(m_createUnitSetProcessorsOnce,
                    [&]()
                    {
                        for (const auto& environment : m_configurationSet.GetUnitEnvironments())
                        {
                            Security::IntegrityLevel requiredIntegrityLevel = SecurityContextToIntegrityLevel(environment.Context());

                            if (m_setProcessors.find(requiredIntegrityLevel) == m_setProcessors.end())
                            {
                                CreateSetProcessorForIntegrityLevel(requiredIntegrityLevel);
                            }
                        }
                    });

                // Create set and unit processor for current unit.
#ifndef AICLI_DISABLE_TEST_HOOKS
                Security::IntegrityLevel requiredIntegrityLevel = m_forceHighIntegrityLevelUnits ? Security::IntegrityLevel::High : GetIntegrityLevelForUnit(unit);
#else
                Security::IntegrityLevel requiredIntegrityLevel = GetIntegrityLevelForUnit(unit);
#endif

                auto itr = m_setProcessors.find(requiredIntegrityLevel);
                if (itr == m_setProcessors.end())
                {
                    itr = CreateSetProcessorForIntegrityLevel(requiredIntegrityLevel);
                }

                return itr->second.Processor.CreateUnitProcessor(unit);
            }

        private:
            // Converts the string representation of SecurityContext to the target integrity level for this instance
            Security::IntegrityLevel SecurityContextToIntegrityLevel(SecurityContext securityContext)
            {
                switch (securityContext)
                {
                case SecurityContext::Current:
                    return m_setIntegrityLevel;
                case SecurityContext::Restricted:
#ifndef AICLI_DISABLE_TEST_HOOKS
                    if (m_enableRestrictedIntegrityLevel)
                    {
                        return Security::IntegrityLevel::Medium;
                    }
                    else
#endif
                    {
                        // Not supporting elevated callers downgrading at the moment.
                        THROW_WIN32(ERROR_NOT_SUPPORTED);

                        // Technically this means the default level of the user token, so if UAC is disabled it would be the only integrity level (aka current).
                        // return Security::IntegrityLevel::Medium;
                    }
                case SecurityContext::Elevated:
                    return Security::IntegrityLevel::High;
                default:
                    THROW_WIN32(ERROR_NOT_SUPPORTED);
                }
            }

            // Gets the integrity level that the given unit should be run at
            Security::IntegrityLevel GetIntegrityLevelForUnit(const ConfigurationUnit& unit)
            {
                return SecurityContextToIntegrityLevel(unit.Environment().Context());
            }

            // Serializes the set properties to be sent to the remote server
            std::string SerializeSetProperties()
            {
                Json::Value json{ Json::ValueType::objectValue };

                json["path"] = winrt::to_string(m_configurationSet.Path());

                std::string locationString;
                switch (m_dynamicFactory->Location())
                {
                case SetProcessorFactory::PwshConfigurationProcessorLocation::AllUsers:
                    locationString = "AllUsers";
                    break;
                case SetProcessorFactory::PwshConfigurationProcessorLocation::CurrentUser:
                    locationString = "CurrentUser";
                    break;
                case SetProcessorFactory::PwshConfigurationProcessorLocation::Custom:
                    locationString = Utility::ConvertToUTF8(m_dynamicFactory->CustomLocation());
                    break;
                case SetProcessorFactory::PwshConfigurationProcessorLocation::Default:
                    break;
                }

                if (!locationString.empty())
                {
                    json["modulePath"] = locationString;
                }

                Json::StreamWriterBuilder writerBuilder;
                writerBuilder.settings_["indentation"] = "\t";
                return Json::writeString(writerBuilder, json);
            }

            /// <summary>
            /// Creates a separate configuration set containing high integrity units and returns the serialized string value.
            /// </summary>
            /// <returns>Serialized string value.</returns>
            std::string SerializeHighIntegrityLevelSet()
            {
                ConfigurationSet highIntegritySet;
                highIntegritySet.SchemaVersion(m_configurationSet.SchemaVersion());
                highIntegritySet.Metadata(m_configurationSet.Metadata());
                highIntegritySet.Parameters(m_configurationSet.Parameters());
                highIntegritySet.Variables(m_configurationSet.Variables());

                std::vector<ConfigurationUnit> highIntegrityUnits;
                auto units = m_configurationSet.Units();

                for (auto unit : units)
                {
                    if (unit.IsActive() && GetIntegrityLevelForUnit(unit) == Security::IntegrityLevel::High)
                    {
                        highIntegrityUnits.emplace_back(unit);
                    }
                }

                highIntegritySet.Units(std::move(highIntegrityUnits));

                // Serialize high integrity set and return output string.
                Streams::InMemoryRandomAccessStream memoryStream;
                highIntegritySet.Serialize(memoryStream);

                Streams::DataReader reader(memoryStream.GetInputStreamAt(0));
                THROW_HR_IF(E_UNEXPECTED, memoryStream.Size() > std::numeric_limits<uint32_t>::max());
                uint32_t streamSize = (uint32_t)memoryStream.Size();
                std::vector<uint8_t> bytes;
                bytes.resize(streamSize);
                reader.LoadAsync(streamSize);
                reader.ReadBytes(bytes);
                reader.DetachStream();
                memoryStream.Close();

                return { bytes.begin(), bytes.end() };
            }

            ProcessorMap::iterator CreateSetProcessorForIntegrityLevel(Security::IntegrityLevel integrityLevel)
            {
                IConfigurationSetProcessorFactory factory;
                IConfigurationSetProcessorFactory::Diagnostics_revoker factoryDiagnosticsEventRevoker;

                // If we got here, the only option is that the current integrity level is not High.
                if (integrityLevel == Security::IntegrityLevel::High)
                {
                    bool useRunAs = true;
#ifndef AICLI_DISABLE_TEST_HOOKS
                    useRunAs = !m_enableTestMode;
#endif

                    factory = CreateOutOfProcessFactory(useRunAs, SerializeSetProperties(), SerializeHighIntegrityLevelSet());
                }
                else
                {
                    THROW_WIN32(ERROR_NOT_SUPPORTED);
                }

                if (factory)
                {
                    factoryDiagnosticsEventRevoker = factory.Diagnostics(winrt::auto_revoke,
                        [weak_this{ get_weak() }](const IInspectable&, const IDiagnosticInformation& information)
                        {
                            if (auto strong_this{ weak_this.get() })
                            {
                                strong_this->m_dynamicFactory->SendDiagnostics(information);
                            }
                        });
                }

                return m_setProcessors.emplace(integrityLevel, DynamicProcessorInfo{ factory, factory.CreateSetProcessor(m_configurationSet), std::move(factoryDiagnosticsEventRevoker) }).first;
            }

            winrt::com_ptr<DynamicFactory> m_dynamicFactory;
            Security::IntegrityLevel m_currentIntegrityLevel;
            Security::IntegrityLevel m_setIntegrityLevel;
            ProcessorMap m_setProcessors;
            ConfigurationSet m_configurationSet;
            std::once_flag m_createUnitSetProcessorsOnce;

#ifndef AICLI_DISABLE_TEST_HOOKS
            bool m_enableTestMode = false;
            bool m_enableRestrictedIntegrityLevel = false;
            bool m_forceHighIntegrityLevelUnits = false;
#endif
        };

        DynamicFactory::DynamicFactory()
        {
            m_defaultRemoteFactory = CreateOutOfProcessFactory();

            if (m_defaultRemoteFactory)
            {
                m_factoryDiagnosticsEventRevoker = m_defaultRemoteFactory.Diagnostics(winrt::auto_revoke,
                    [weak_this{ get_weak() }](const IInspectable&, const IDiagnosticInformation& information)
                    {
                        if (auto strong_this{ weak_this.get() })
                        {
                            strong_this->SendDiagnostics(information);
                        }
                    });
            }
        }

        IConfigurationSetProcessor DynamicFactory::CreateSetProcessor(const ConfigurationSet& configurationSet)
        {
            return winrt::make<DynamicSetProcessor>(get_strong(), m_defaultRemoteFactory.CreateSetProcessor(configurationSet), configurationSet);
        }

        winrt::event_token DynamicFactory::Diagnostics(const EventHandler<IDiagnosticInformation>& handler)
        {
            return m_diagnostics.add(handler);
        }

        void DynamicFactory::Diagnostics(const winrt::event_token& token) noexcept
        {
            m_diagnostics.remove(token);
        }

        DiagnosticLevel DynamicFactory::MinimumLevel()
        {
            return m_minimumLevel;
        }

        void DynamicFactory::MinimumLevel(DiagnosticLevel value)
        {
            m_minimumLevel = value;
        }

        HRESULT STDMETHODCALLTYPE DynamicFactory::SetLifetimeWatcher(IUnknown* watcher)
        {
            return WinRT::LifetimeWatcherBase::SetLifetimeWatcher(watcher);
        }

        IConfigurationSetProcessorFactory& DynamicFactory::DefaultFactory()
        {
            return m_defaultRemoteFactory;
        }

        void DynamicFactory::SendDiagnostics(const IDiagnosticInformation& information) try
        {
            if (information.Level() >= m_minimumLevel)
            {
                std::lock_guard<std::mutex> lock{ m_diagnosticsMutex };
                m_diagnostics(*this, information);
            }
        }
        // While diagnostics can be important, a failure to send them should not cause additional issues.
        catch (...) {}
    }

    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateDynamicRuntimeFactory()
    {
        return winrt::make<anonymous::DynamicFactory>();
    }
}
