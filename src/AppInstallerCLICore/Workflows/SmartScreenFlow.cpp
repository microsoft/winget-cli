// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SmartScreenFlow.h"
#include <AppInstallerDownloader.h>
#include <UriValidation/UriValidation.h>

namespace AppInstaller::CLI::Workflow
{
    bool IsSmartScreenRequired(Settings::ConfigurationAllowedZonesOptions zone)
    {
        return zone == Settings::ConfigurationAllowedZonesOptions::Internet
            || zone == Settings::ConfigurationAllowedZonesOptions::UntrustedSites;
    }

    // Validate smart screen for a given url.
    bool IsBlockedBySmartScreen(Execution::Context& context, const std::string& url)
    {
        auto response = AppInstaller::UriValidation::UriValidation(url);
        switch (response.Decision())
        {
        case AppInstaller::UriValidation::UriValidationDecision::Block:
            context.Reporter.Error() << std::endl << "Blocked by smart screen" << std::endl << "Feedback: " << response.Feedback() << std::endl;
            return true;
        case AppInstaller::UriValidation::UriValidationDecision::Allow:
        default:
            return false;
        }
    }

    // Get Uri zone for a given uri or file path.
    Settings::ConfigurationAllowedZonesOptions GetUriZone(const std::string& uri)
    {
        DWORD dwZone;
        auto pInternetSecurityManager = winrt::create_instance<IInternetSecurityManager>(CLSID_InternetSecurityManager, CLSCTX_ALL);
        pInternetSecurityManager->MapUrlToZone(AppInstaller::Utility::ConvertToUTF16(uri).c_str(), &dwZone, 0);

        // Treat all zones higher than untrusted as untrusted
        if (dwZone > static_cast<DWORD>(Settings::ConfigurationAllowedZonesOptions::UntrustedSites))
        {
            return Settings::ConfigurationAllowedZonesOptions::UntrustedSites;
        }

        return static_cast<Settings::ConfigurationAllowedZonesOptions>(dwZone);
    }

    // Validate group policy for a given zone.
    bool IsBlockedByGroupPolicy(Execution::Context& context, const Settings::ConfigurationAllowedZonesOptions zone)
    {
        auto configurationPolicies = Settings::GroupPolicies().GetValue<Settings::ValuePolicy::ConfigurationAllowedZones>();
        if (!configurationPolicies.has_value())
        {
            AICLI_LOG(Config, Warning, << "ConfigurationAllowedZones policy is not set");
            return false;
        }

        if (configurationPolicies->find(zone) == configurationPolicies->end())
        {
            AICLI_LOG(Config, Warning, << "Configuration is not configured in the zone " << zone);
            return false;
        }

        auto isAllowed = configurationPolicies->at(zone);
        if(!isAllowed)
        {
            context.Reporter.Error() << "Configuration is disabled for Zone: " << zone << std::endl;
            return true;
        }

        AICLI_LOG(Config, Info, << "Configuration is configured in zone " << zone << " with value " << (isAllowed ? "allowed" : "blocked"));
        return false;
    }

    HRESULT EvaluateUri(Execution::Context& context, const std::string& uri)
    {
        auto zone = GetUriZone(uri);
        if(IsBlockedByGroupPolicy(context, zone))
        {
            return APPINSTALLER_CLI_ERROR_BLOCKED_BY_POLICY;
        }

        if (IsSmartScreenRequired(zone) && IsBlockedBySmartScreen(context, uri))
        {
            return APPINSTALLER_CLI_ERROR_BLOCKED_BY_REPUTATION_SERVICE;
        }

        return S_OK;
    }

    void EvaluateUri(Execution::Context& context)
    {
        if (context.Args.Contains(Execution::Args::Type::ConfigurationFile))
        {
            std::string argPath{ context.Args.GetArg(Execution::Args::Type::ConfigurationFile) };

            if (Utility::IsUrlRemote(argPath))
            {
                context.Reporter.Info() << "Validating Uri: " << argPath;
                auto uriValidation = EvaluateUri(context, argPath);
                if (FAILED(uriValidation))
                {
                    AICLI_LOG(Config, Error, << "URI validation blocked this uri: " << argPath);
                    AICLI_TERMINATE_CONTEXT(uriValidation);
                }
            }
            else
            {
                AICLI_LOG(Config, Info, << "Skipping Uri validation for local file: " << argPath);
            }
        }
    }
}
