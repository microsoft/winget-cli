// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UriValidationFlow.h"
#include <AppInstallerDownloader.h>
#include <UriValidation/UriValidation.h>
#include <regex>

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // Convert the security zone to string.
        std::string ToString(Settings::SecurityZoneOptions zone)
        {
            switch (zone)
            {
            case Settings::SecurityZoneOptions::LocalMachine:
                return "LocalMachine";
            case Settings::SecurityZoneOptions::Intranet:
                return "Intranet";
            case Settings::SecurityZoneOptions::TrustedSites:
                return "TrustedSites";
            case Settings::SecurityZoneOptions::Internet:
                return "Internet";
            case Settings::SecurityZoneOptions::UntrustedSites:
                return "UntrustedSites";
            default:
                return "Unknown";
            }
        }

        // Check if smart screen is required for a given zone.
        bool IsSmartScreenRequired(Settings::SecurityZoneOptions zone, bool isSourceTrusted)
        {
            if (zone != Settings::SecurityZoneOptions::Internet && zone != Settings::SecurityZoneOptions::UntrustedSites)
            {
                AICLI_LOG(Core, Info, << "Skipping smart screen validation for zone " << ToString(zone));
                return false;
            }

            auto ssPolicyState = Settings::GroupPolicies().GetState(Settings::TogglePolicy::Policy::SmartScreenCheck);
            if (ssPolicyState == Settings::PolicyState::Disabled)
            {
                AICLI_LOG(Core, Info, << "Smart screen validation is disabled by group policy");
                return false;
            }

            if (ssPolicyState == Settings::PolicyState::Enabled)
            {
                AICLI_LOG(Core, Info, << "Smart screen validation is enabled by group policy");
                return true;
            }

            AICLI_LOG(Core, Info, << "Smart screen validation is not configured by group policy");

            if (isSourceTrusted)
            {
                AICLI_LOG(Core, Info, << "Skipping smart screen validation for trusted source");
                return false;
            }

            AICLI_LOG(Core, Info, << "Smart screen validation is required for untrusted source");
            return true;
        }

        // Check if the given uri is blocked by smart screen.
        bool IsUriBlockedBySmartScreen(Execution::Context& context, const std::string& uri)
        {
            auto response = AppInstaller::UriValidation::ValidateUri(uri);
            switch (response.Decision())
            {
            case AppInstaller::UriValidation::UriValidationDecision::Block:
                AICLI_LOG(Core, Verbose, << "URI '" << uri << "' was blocked by smart screen. Feedback URL: " << response.Feedback());
                context.Reporter.Error() << Resource::String::UriBlockedBySmartScreen << std::endl;
                return true;
            case AppInstaller::UriValidation::UriValidationDecision::Allow:
            default:
                return false;
            }
        }

        // Get Uri zone for a given uri or file path.
        HRESULT GetUriZone(const std::string& uri, Settings::SecurityZoneOptions& zone)
        {
#ifndef AICLI_DISABLE_TEST_HOOKS
            // For testing purposes, allow the zone to be set via the uri
            std::smatch match;
            if (std::regex_search(uri, match, std::regex("/zone(\\d+)/")))
            {
                std::string number = match[1].str();
                zone = static_cast<Settings::SecurityZoneOptions>(std::stoul(number));
                return S_OK;
            }
#endif

            DWORD dwZone;
            auto pInternetSecurityManager = winrt::create_instance<IInternetSecurityManager>(CLSID_InternetSecurityManager, CLSCTX_ALL);
            auto mapResult = pInternetSecurityManager->MapUrlToZone(AppInstaller::Utility::ConvertToUTF16(uri).c_str(), &dwZone, 0);

            // Ensure MapUrlToZone was successful and the zone value is valid
            if (FAILED(mapResult))
            {
                return mapResult;
            }

            // Treat all zones higher than untrusted as untrusted
            if (dwZone > static_cast<DWORD>(Settings::SecurityZoneOptions::UntrustedSites))
            {
                zone = Settings::SecurityZoneOptions::UntrustedSites;
            }
            else
            {
                zone = static_cast<Settings::SecurityZoneOptions>(dwZone);
            }

            return S_OK;
        }

        HRESULT GetIsSourceTrusted(const Execution::Context& context, bool& isTrusted)
        {
            if (context.Contains(Execution::Data::PackageVersion))
            {
                const auto packageVersion = context.Get<Execution::Data::PackageVersion>();
                const auto source = packageVersion->GetSource();
                isTrusted = WI_IsFlagSet(source.GetDetails().TrustLevel, Repository::SourceTrustLevel::Trusted);
                return S_OK;
            }

            return E_FAIL;
        }

        HRESULT GetInstallerUrl(const Execution::Context& context, std::string& installerUrl)
        {
            if (context.Contains(Execution::Data::Installer))
            {
                installerUrl = context.Get<Execution::Data::Installer>()->Url;
                return S_OK;
            }

            return E_FAIL;
        }

        HRESULT GetConfigurationUri(const Execution::Context& context, std::string& configurationUri)
        {
            if (context.Args.Contains(Execution::Args::Type::ConfigurationFile))
            {
                configurationUri = context.Args.GetArg(Execution::Args::Type::ConfigurationFile);
                return S_OK;
            }

            return E_FAIL;
        }

        // Validate group policy for a given zone.
        bool IsZoneBlockedByGroupPolicy(Execution::Context& context, const Settings::SecurityZoneOptions& zone)
        {
            if (!Settings::GroupPolicies().IsEnabled(Settings::TogglePolicy::Policy::AllowedSecurityZones))
            {
                AICLI_LOG(Core, Info, << "WindowsPackageManagerAllowedSecurityZones policy is disabled");
                return false;
            }

            auto allowedSecurityZones = Settings::GroupPolicies().GetValue<Settings::ValuePolicy::AllowedSecurityZones>();
            if (!allowedSecurityZones.has_value())
            {
                AICLI_LOG(Core, Warning, << "WindowsPackageManagerAllowedSecurityZones policy is not set");
                return false;
            }

            auto zoneIterator = allowedSecurityZones->find(zone);
            if (zoneIterator == allowedSecurityZones->end())
            {
                AICLI_LOG(Core, Warning, << "Security zone " << zone << " was not found in the group policy WindowsPackageManagerAllowedSecurityZones");
                return false;
            }

            auto isAllowed = zoneIterator->second;
            if (!isAllowed)
            {
                AICLI_LOG(Core, Error, << "Security zone " << zone << " is blocked by group policy");
                context.Reporter.Error() << Resource::String::UriSecurityZoneBlockedByPolicy << std::endl;
                return true;
            }

            AICLI_LOG(Core, Info, << "Configuration is disabled in zone " << zone);
            return false;
        }

        // Evaluate the configuration uri for group policy and smart screen.
        HRESULT EvaluateConfigurationUri(Execution::Context& context)
        {
            std::string configurationUri;
            if (FAILED(GetConfigurationUri(context, configurationUri)))
            {
                AICLI_LOG(Core, Warning, << "Configuration URI is not available. Skipping validation.");
                return S_OK;
            }

            Settings::SecurityZoneOptions configurationUriZone;
            if (FAILED(GetUriZone(configurationUri, configurationUriZone)))
            {
                AICLI_LOG(Core, Warning, << "Failed to get zone for configuration URI: " << configurationUri << ". Skipping validation.");
                return S_OK;
            }

            if (IsZoneBlockedByGroupPolicy(context, configurationUriZone))
            {
                AICLI_LOG(Core, Error, << "Configuration URI's zone is blocked by group policy: " << configurationUri << " (" << ToString(configurationUriZone) << ")");
                return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
            }

            if (IsSmartScreenRequired(configurationUriZone, false) && IsUriBlockedBySmartScreen(context, configurationUri))
            {
                AICLI_LOG(Core, Error, << "Configuration URI was blocked by smart screen: " << configurationUri);
                return APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE;
            }

            AICLI_LOG(Core, Info, << "Configuration URI is validated: " << configurationUri);
            return S_OK;
        }

        HRESULT EvaluateDownloadUri(Execution::Context& context)
        {
            std::string installerUrl;
            if (FAILED(GetInstallerUrl(context, installerUrl)))
            {
                AICLI_LOG(Core, Warning, << "Installer URL is not available. Skipping validation.");
                return S_OK;
            }

            Settings::SecurityZoneOptions installerUrlZone;
            if (FAILED(GetUriZone(installerUrl, installerUrlZone)))
            {
                AICLI_LOG(Core, Warning, << "Failed to get zone for installer URL: " << installerUrl << ". Skipping validation.");
                return S_OK;
            }

            if (IsZoneBlockedByGroupPolicy(context, installerUrlZone))
            {
                AICLI_LOG(Core, Error, << "Installer URL's zone is blocked by group policy: " << installerUrl << " (" << ToString(installerUrlZone) << ")");
                return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
            }

            bool isSourceTrusted;
            if (FAILED(GetIsSourceTrusted(context, isSourceTrusted)))
            {
                AICLI_LOG(Core, Warning, << "Source trust level is not available. Skipping smart screen validation.");
                return S_OK;
            }

            if (IsSmartScreenRequired(installerUrlZone, isSourceTrusted) && IsUriBlockedBySmartScreen(context, installerUrl))
            {
                AICLI_LOG(Core, Error, << "Installer URL was blocked by smart screen: " << installerUrl);
                return APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE;
            }

            AICLI_LOG(Core, Info, << "Installer URL is validated: " << installerUrl);
            return S_OK;
        }
    }

    // Execute the smart screen flow.
    void ExecuteUriValidation::operator()(Execution::Context& context) const
    {
        if (m_uriValidationSource == UriValidationSource::ConfigurationSource)
        {
            auto uriValidation = EvaluateConfigurationUri(context);
            if (FAILED(uriValidation))
            {
                AICLI_TERMINATE_CONTEXT(uriValidation);
            }
        }
        else
        {
            auto uriValidation = EvaluateDownloadUri(context);
            if (FAILED(uriValidation))
            {
                AICLI_TERMINATE_CONTEXT(uriValidation);
            }
        }
    }
}
