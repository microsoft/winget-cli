// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/ConfigurationSetProcessorFactoryRemoting.h"
#include <AppInstallerStrings.h>
#include <winget/ILifetimeWatcher.h>
#include <winget/Security.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Microsoft::Management::Configuration;

namespace AppInstaller::CLI::ConfigurationRemoting
{
    namespace anonymous
    {
        struct DynamicSetProcessor : winrt::implements<DynamicSetProcessor, IConfigurationSetProcessor>
        {
            DynamicSetProcessor(IConfigurationSetProcessor defaultRemoteSetProcessor, const ConfigurationSet& configurationSet) : m_configurationSet(configurationSet)
            {
                m_currentIntegrityLevel = Security::GetEffectiveIntegrityLevel();
                m_setProcessors.emplace(m_currentIntegrityLevel, defaultRemoteSetProcessor);
            }

            IConfigurationUnitProcessorDetails GetUnitProcessorDetails(const ConfigurationUnit& unit, ConfigurationUnitDetailFlags detailFlags)
            {
                // Always get processor details from the current integrity level
                return m_setProcessors[m_currentIntegrityLevel].GetUnitProcessorDetails(unit, detailFlags);
            }

            // Creates a configuration unit processor for the given unit.
            IConfigurationUnitProcessor CreateUnitProcessor(const ConfigurationUnit& unit)
            {
                // TODO: Inspect the configuration set to determine the complete set of integrity levels we will need and create the set processors for all of them.
                //       We would do this here to avoid creating them if the only call is going to be for details.
                //       We want to do this now to prevent a UAC from showing much later than the start of the operation.

                Security::IntegrityLevel requiredIntegrityLevel = GetIntegrityLevelForUnit(unit);
                auto itr = m_setProcessors.find(requiredIntegrityLevel);
                if (itr == m_setProcessors.end())
                {
                    itr = CreateSetProcessorForIntegrityLevel(requiredIntegrityLevel);
                }

                return itr->second.CreateUnitProcessor(unit);
            }

        private:
            // Converts the string representation of SecurityContext to the integrity level
            Security::IntegrityLevel SecurityContextToIntegrityLevel(winrt::hstring securityContext)
            {
                std::wstring securityContextLower = Utility::ToLower(securityContext);

                if (securityContextLower == L"elevated")
                {
                    return Security::IntegrityLevel::High;
                }
                else if (securityContextLower == L"restricted")
                {
                    // Not supporting elevated callers downgrading at the moment.
                    THROW_WIN32(ERROR_NOT_SUPPORTED);

                    // Technically this means the default level of the user token, so if UAC is disabled it would be the only integrity level (aka current).
                    //return Security::IntegrityLevel::Medium;
                }
                else if (securityContextLower == L"current")
                {
                    return m_currentIntegrityLevel;
                }

                THROW_WIN32(ERROR_NOT_SUPPORTED);
            }

            // Gets the integrity level that the given unit should be run at
            Security::IntegrityLevel GetIntegrityLevelForUnit(const ConfigurationUnit& unit)
            {
                // Support for 0.2 schema via metadata value
                // TODO: Support case insensitive lookup by iteration
                auto unitMetadata = unit.Metadata();
                auto securityContext = unitMetadata.TryLookup(L"SecurityContext");
                if (securityContext)
                {
                    auto securityContextProperty = securityContext.try_as<IPropertyValue>();
                    if (securityContextProperty && securityContextProperty.Type() == PropertyType::String)
                    {
                        return SecurityContextToIntegrityLevel(securityContextProperty.GetString());
                    }
                }

                // TODO: Support for 0.3 schema will require a group processor wrapper

                return m_currentIntegrityLevel;
            }

            // Serializes the set properties to be sent to the remote server
            std::string SerializeSetProperties()
            {

            }

            // Serializes a version of the set that only contains the units that require high integrity level
            std::string SerializeHighIntegrityLevelSet()
            {

            }

            std::map<Security::IntegrityLevel, IConfigurationSetProcessor>::iterator CreateSetProcessorForIntegrityLevel(Security::IntegrityLevel integrityLevel)
            {
                IConfigurationSetProcessorFactory factory;

                // If we got here, the only option is that the current integrity level is not High.
                if (integrityLevel == Security::IntegrityLevel::High)
                {
                    factory = CreateOutOfProcessFactory(true, SerializeSetProperties(), SerializeHighIntegrityLevelSet());
                }
                else
                {
                    THROW_WIN32(ERROR_NOT_SUPPORTED);
                }

                return m_setProcessors.emplace(integrityLevel, factory).first;
            }

            Security::IntegrityLevel m_currentIntegrityLevel;
            std::map<Security::IntegrityLevel, IConfigurationSetProcessor> m_setProcessors;
            ConfigurationSet m_configurationSet;
        };

        // This is implemented completely in the packaged context for now, if we want to make it more configurable, we will probably want to move it to configuration and
        // have this implementation leverage that one with an event handler for the packaged specifics.
        struct DynamicFactory : winrt::implements<DynamicFactory, IConfigurationSetProcessorFactory, winrt::cloaked<WinRT::ILifetimeWatcher>>, WinRT::LifetimeWatcherBase
        {
            DynamicFactory()
            {
                m_defaultRemoteFactory = CreateOutOfProcessFactory();
            }

            IConfigurationSetProcessor CreateSetProcessor(const ConfigurationSet& configurationSet)
            {
                return winrt::make<DynamicSetProcessor>(m_defaultRemoteFactory.CreateSetProcessor(configurationSet), configurationSet);
            }

            winrt::event_token Diagnostics(const EventHandler<IDiagnosticInformation>&)
            {
                // TODO: If we want diagnostics here, see ConfigurationProcessor for how to integrate nicely with the infrastructure.
                //       Best solution is probably to create a base class that both can leverage to handle it cleanly.
                return {};
            }

            void Diagnostics(const winrt::event_token&) noexcept
            {
            }

            DiagnosticLevel MinimumLevel()
            {
                return m_minimumLevel;
            }

            void MinimumLevel(DiagnosticLevel value)
            {
                m_minimumLevel = value;
            }

        private:
            IConfigurationSetProcessorFactory m_defaultRemoteFactory;
            DiagnosticLevel m_minimumLevel = DiagnosticLevel::Informational;
        };
    }

    winrt::Microsoft::Management::Configuration::IConfigurationSetProcessorFactory CreateDynamicRuntimeFactory()
    {
        return winrt::make<anonymous::DynamicFactory>();
    }
}
