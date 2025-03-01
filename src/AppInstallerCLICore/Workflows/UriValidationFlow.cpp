// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UriValidationFlow.h"
#include <AppInstallerDownloader.h>
#include <UriValidation/UriValidation.h>
#include <winget/AdminSettings.h>
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
        bool IsSmartScreenRequired(Execution::Context& context, Settings::SecurityZoneOptions zone, bool isTrusted)
        {
            // Smart screen is required only for Internet and UntrustedSites zones.
            if (zone != Settings::SecurityZoneOptions::Internet && zone != Settings::SecurityZoneOptions::UntrustedSites)
            {
                AICLI_LOG(Core, Info, << "Skipping smart screen validation for zone " << ToString(zone));
                return false;
            }

            auto policyState = Settings::GroupPolicies().GetState(Settings::TogglePolicy::Policy::BypassSmartScreenCheck);

            // If group policy is disabled, smart screen validation cannot be bypassed even for trusted URIs.
            if (policyState == Settings::PolicyState::Disabled)
            {
                AICLI_LOG(Core, Info, << "Smart screen validation is required by group policy");
                return true;
            }

            // Skip smart screen validation if the --ignore-smartscreen argument is provided.
            if (context.Args.Contains(Execution::Args::Type::IgnoreSmartScreen))
            {
                AICLI_LOG(Core, Info, << "Skipping smart screen validation as the user has opted out");
                return false;
            }

            // Skip smart screen validation for trusted URIs.
            if (isTrusted)
            {
                AICLI_LOG(Core, Info, << "Skipping smart screen validation for trusted URI");
                return false;
            }

            // Smart screen validation is required for untrusted URIs.
            AICLI_LOG(Core, Info, << "Smart screen validation is required for untrusted URI");
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

        // Get whether or not the source is trusted.
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

        // Get the installer url from the context.
        HRESULT GetInstallerUrl(const Execution::Context& context, std::string& installerUrl)
        {
            if (context.Contains(Execution::Data::Installer))
            {
                installerUrl = context.Get<Execution::Data::Installer>()->Url;
                return S_OK;
            }

            return E_FAIL;
        }

        // Get the configuration uri from the context.
        HRESULT GetConfigurationUri(const Execution::Context& context, std::string& configurationUri)
        {
            if (context.Args.Contains(Execution::Args::Type::ConfigurationFile))
            {
                configurationUri = context.Args.GetArg(Execution::Args::Type::ConfigurationFile);
                return S_OK;
            }

            return E_FAIL;
        }

        // Get the source add url from the context.
        HRESULT GetSourceAddUrl(const Execution::Context& context, std::string& sourceAddUrl)
        {
            if (context.Contains(Execution::Data::Source))
            {
                auto& sourceToAdd = context.Get<Execution::Data::Source>();
                auto details = sourceToAdd.GetDetails();
                sourceAddUrl = details.Arg;
                return S_OK;
            }

            return E_FAIL;
        }

        // Validate group policy for a given zone.
        bool IsZoneBlockedByGroupPolicy(Execution::Context& context, const Settings::SecurityZoneOptions& zone)
        {
            // If the group policy for allowed security zones is not enabled then skip validation.
            if (!Settings::GroupPolicies().IsEnabled(Settings::TogglePolicy::Policy::AllowedSecurityZones))
            {
                AICLI_LOG(Core, Info, << "Group policy for allowed security zones is disabled.");
                return false;
            }

            // If the group policy is enabled but no zones are configured then skip validation.
            auto allowedSecurityZones = Settings::GroupPolicies().GetValue<Settings::ValuePolicy::AllowedSecurityZones>();
            if (!allowedSecurityZones.has_value())
            {
                AICLI_LOG(Core, Warning, << "Group policy for allowed security zones is enabled but no zones are configured.");
                return false;
            }

            // If the zone is not found in the allowed security zones then skip validation.
            auto zoneIterator = allowedSecurityZones->find(zone);
            if (zoneIterator == allowedSecurityZones->end())
            {
                AICLI_LOG(Core, Warning, << "Security zone " << ToString(zone) << " was not found in the group policy for allowed security zones.");
                return false;
            }

            // If the zone is found in the allowed security zones but is not allowed then block the configuration.
            auto isAllowed = zoneIterator->second;
            if (!isAllowed)
            {
                AICLI_LOG(Core, Error, << "Security zone " << ToString(zone) << " is blocked by group policy");
                context.Reporter.Error() << Resource::String::UriSecurityZoneBlockedByPolicy << std::endl;
                return true;
            }

            AICLI_LOG(Core, Info, << "Security zone " << ToString(zone) << " is allowed by group policy");
            return false;
        }

        // Core logic to evaluate the uri.
        HRESULT EvaluateUri(Execution::Context& context, std::string uri, bool isUriTrusted)
        {
            Settings::SecurityZoneOptions uriZone;
            if (FAILED(GetUriZone(uri, uriZone)))
            {
                AICLI_LOG(Core, Warning, << "Failed to get security zone for URI: " << uri << ". Skipping validation.");
                return S_OK;
            }

            if (IsZoneBlockedByGroupPolicy(context, uriZone))
            {
                AICLI_LOG(Core, Error, << "URI security zone is blocked by group policy: " << uri << " (" << ToString(uriZone) << ")");
                return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
            }

            if (IsSmartScreenRequired(context, uriZone, isUriTrusted) && IsUriBlockedBySmartScreen(context, uri))
            {
                AICLI_LOG(Core, Error, << "URI was blocked by smart screen: " << uri);
                return APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE;
            }

            AICLI_LOG(Core, Info, << "URI was validated successfully: " << uri);
            return S_OK;

        }

        // Evaluate the configuration uri
        HRESULT EvaluateConfigurationUri(Execution::Context& context)
        {
            std::string configurationUri;
            if (FAILED(GetConfigurationUri(context, configurationUri)))
            {
                AICLI_LOG(Core, Warning, << "Configuration URI is not available. Skipping validation.");
                return S_OK;
            }

            return EvaluateUri(context, configurationUri, false);
        }

        // Evaluate the download uri
        HRESULT EvaluateDownloadUri(Execution::Context& context)
        {
            std::string installerUrl;
            if (FAILED(GetInstallerUrl(context, installerUrl)))
            {
                AICLI_LOG(Core, Warning, << "Installer URL is not available. Skipping validation.");
                return S_OK;
            }

            bool isSourceTrusted;
            if (FAILED(GetIsSourceTrusted(context, isSourceTrusted)))
            {
                AICLI_LOG(Core, Warning, << "Source trust level is not available. Skipping smart screen validation.");
                return S_OK;
            }

            return EvaluateUri(context, installerUrl, isSourceTrusted);
        }

        // Evaluate the source add uri
        HRESULT EvaluateSourceAddUri(Execution::Context& context)
        {
            std::string sourceAddUrl;
            if (FAILED(GetSourceAddUrl(context, sourceAddUrl)))
            {
                AICLI_LOG(Core, Warning, << "Source URL is not available. Skipping validation.");
                return S_OK;
            }

            return EvaluateUri(context, sourceAddUrl, false);
        }

        // Evaluate the uri based on the source.
        HRESULT EvaluateUriBySource(Execution::Context& context, UriValidationSource uriValidationSource)
        {
            switch (uriValidationSource)
            {
            case UriValidationSource::Configuration:
                return EvaluateConfigurationUri(context);
            case UriValidationSource::Package:
                return EvaluateDownloadUri(context);
            case UriValidationSource::SourceAdd:
                return EvaluateSourceAddUri(context);
            default:
                THROW_HR(E_UNEXPECTED);
            }
        }
    }

    // Execute the smart screen flow.
    void ExecuteUriValidation::operator()(Execution::Context& context) const
    {
        auto uriValidation = EvaluateUriBySource(context, m_uriValidationSource);
        if (FAILED(uriValidation))
        {
            AICLI_TERMINATE_CONTEXT(uriValidation);
        }
    }
}
