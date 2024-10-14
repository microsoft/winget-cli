// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SmartScreenFlow.h"
#include <AppInstallerDownloader.h>
#include <UriValidation/UriValidation.h>

namespace AppInstaller::CLI::Workflow
{
    // Check if smart screen is required for a given zone.
    bool IsSmartScreenRequired(Settings::SecurityZoneOptions zone)
    {
        return zone == Settings::SecurityZoneOptions::Internet
            || zone == Settings::SecurityZoneOptions::UntrustedSites;
    }

    // Check if the given uri is blocked by smart screen.
    bool IsBlockedBySmartScreen(Execution::Context& context, const std::string& uri)
    {
        auto response = AppInstaller::UriValidation::ValidateUri(uri);
        switch (response.Decision())
        {
        case AppInstaller::UriValidation::UriValidationDecision::Block:
            AICLI_LOG(Config, Error, << "URI '" << uri << "' was blocked by smart screen. Feedback URL: " << response.Feedback());
            context.Reporter.Error() << Resource::String::UriBlockedBySmartScreen << std::endl;
            return true;
        case AppInstaller::UriValidation::UriValidationDecision::Allow:
        default:
            return false;
        }
    }

    // Get Uri zone for a given uri or file path.
    Settings::SecurityZoneOptions GetUriZone(const std::string& uri)
    {
        DWORD dwZone;
        auto pInternetSecurityManager = winrt::create_instance<IInternetSecurityManager>(CLSID_InternetSecurityManager, CLSCTX_ALL);
        auto mapResult = pInternetSecurityManager->MapUrlToZone(AppInstaller::Utility::ConvertToUTF16(uri).c_str(), &dwZone, 0);

        // Treat invalid uri argument as local machine
        if (mapResult == E_INVALIDARG)
        {
            return Settings::SecurityZoneOptions::LocalMachine;
        }

        // Treat all zones higher than untrusted as untrusted
        if (dwZone > static_cast<DWORD>(Settings::SecurityZoneOptions::UntrustedSites))
        {
            return Settings::SecurityZoneOptions::UntrustedSites;
        }

        return static_cast<Settings::SecurityZoneOptions>(dwZone);
    }

    // Validate group policy for a given zone.
    bool IsBlockedByGroupPolicy(Execution::Context& context, const Settings::SecurityZoneOptions zone)
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
            context.Reporter.Error() << Resource::String::UriZoneBlockedByPolicy << std::endl;
            return true;
        }

        AICLI_LOG(Config, Info, << "Configuration is configured in zone " << zone << " with value " << (isAllowed ? "allowed" : "blocked"));
        return false;
    }

    // Evaluate the given uri for group policy and smart screen.
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

    // Evaluate the configuration uri for group policy and smart screen.
    HRESULT EvaluateConfigurationUri(Execution::Context& context)
    {
        std::string argPath{ context.Args.GetArg(Execution::Args::Type::ConfigurationFile) };
        if (Utility::IsUrlRemote(argPath))
        {
            context.Reporter.Info() << "Validating Uri: " << argPath;
            AICLI_LOG(Config, Error, << "URI validation blocked this uri: " << argPath);
            return EvaluateUri(context, argPath);
        }

        AICLI_LOG(Config, Info, << "Skipping Uri validation for local file: " << argPath);
        return S_OK;
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
    void ExecuteSmartScreen::operator()(Execution::Context& context) const
    {
        if (m_isConfigurationFlow)
        {
            auto uriValidation = EvaluateConfigurationUri(context);
            if(FAILED(uriValidation))
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
