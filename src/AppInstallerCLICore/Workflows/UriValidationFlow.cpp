// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "UriValidationFlow.h"
#include <AppInstallerDownloader.h>
#include <UriValidation/UriValidation.h>

namespace AppInstaller::CLI::Workflow
{
    // Check if smart screen is required for a given zone.
    bool IsSmartScreenRequired(Settings::SecurityZoneOptions zone)
    {
        auto isSmartScreenEnabled = Settings::GroupPolicies().IsEnabled(Settings::TogglePolicy::Policy::SmartScreenValidation);
        AICLI_LOG(Core, Info, << "SmartScreen validation is " << (isSmartScreenEnabled ? "enabled" : "disabled"));

        auto isSecurityZoneCheckRequired = zone == Settings::SecurityZoneOptions::Internet || zone == Settings::SecurityZoneOptions::UntrustedSites;
        AICLI_LOG(Core, Info, << "Security zone check is " << (isSecurityZoneCheckRequired ? "required" : "not required"));

        return isSmartScreenEnabled && isSecurityZoneCheckRequired;
    }

    // Check if the given uri is blocked by smart screen.
    bool IsBlockedBySmartScreen(Execution::Context& context, const std::string& uri)
    {
        auto response = AppInstaller::UriValidation::ValidateUri(uri);
        switch (response.Decision())
        {
        case AppInstaller::UriValidation::UriValidationDecision::Block:
            AICLI_LOG(Core, Error, << "URI '" << uri << "' was blocked by smart screen. Feedback URL: " << response.Feedback());
            context.Reporter.Error() << Resource::String::UriBlockedBySmartScreen << std::endl;
            return true;
        case AppInstaller::UriValidation::UriValidationDecision::Allow:
        default:
            return false;
        }
    }

    // Get Uri zone for a given uri or file path.
    HRESULT GetUriZone(const std::string& uri, Settings::SecurityZoneOptions* zone)
    {
        if (!zone)
        {
            return E_INVALIDARG;
        }

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
            *zone = Settings::SecurityZoneOptions::UntrustedSites;
        }
        else
        {
            *zone = static_cast<Settings::SecurityZoneOptions>(dwZone);
        }

        return S_OK;
    }

    // Validate group policy for a given zone.
    bool IsBlockedByGroupPolicy(Execution::Context& context, const Settings::SecurityZoneOptions zone)
    {
        auto allowedSecurityZones = Settings::GroupPolicies().GetValue<Settings::ValuePolicy::AllowedSecurityZones>();
        if (!allowedSecurityZones.has_value())
        {
            AICLI_LOG(Core, Warning, << "AllowedSecurityZones policy is not set");
            return false;
        }

        if (allowedSecurityZones->find(zone) == allowedSecurityZones->end())
        {
            AICLI_LOG(Core, Warning, << "Security zone " << zone << " was not found in the group policy AllowedSecurityZones");
            return false;
        }

        auto isAllowed = allowedSecurityZones->at(zone);
        if(!isAllowed)
        {
            AICLI_LOG(Core, Error, << "Security zone " << zone << " is blocked by group policy");
            context.Reporter.Error() << Resource::String::UriSecurityZoneBlockedByPolicy << std::endl;
            return true;
        }

        AICLI_LOG(Core, Info, << "Configuration is configured in zone " << zone << " with value " << (isAllowed ? "allowed" : "blocked"));
        return false;
    }

    // Evaluate the given uri for group policy and smart screen.
    HRESULT EvaluateUri(Execution::Context& context, const std::string& uri)
    {
        AICLI_LOG(Core, Info, << "Validating URI: " << uri);

        Settings::SecurityZoneOptions zone;
        auto zoneResult = GetUriZone(uri, &zone);

        if (SUCCEEDED(zoneResult))
        {
            if(IsBlockedByGroupPolicy(context, zone))
            {
                return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
            }

            if (IsSmartScreenRequired(zone) && IsBlockedBySmartScreen(context, uri))
            {
                return APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE;
            }
        }
        else
        {
            AICLI_LOG(Core, Warning, << "Failed to get zone for URI: " << uri << " with error: " << zoneResult << ". Skipping validation.");
        }

        return S_OK;
    }

    // Evaluate the configuration uri for group policy and smart screen.
    HRESULT EvaluateConfigurationUri(Execution::Context& context)
    {
        std::string argPath{ context.Args.GetArg(Execution::Args::Type::ConfigurationFile) };
        return EvaluateUri(context, argPath);
    }

    // Evaluate the download uri for group policy and smart screen.
    HRESULT EvaluateDownloadUri(Execution::Context& context)
    {
        const auto packageVersion = context.Get<Execution::Data::PackageVersion>();
        const auto source = packageVersion->GetSource();
        const auto isTrusted = WI_IsFlagSet(source.GetDetails().TrustLevel, Repository::SourceTrustLevel::Trusted);
        if (!isTrusted)
        {
            auto installer = context.Get<Execution::Data::Installer>();
            return EvaluateUri(context, installer->Url);
        }

        return S_OK;
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
