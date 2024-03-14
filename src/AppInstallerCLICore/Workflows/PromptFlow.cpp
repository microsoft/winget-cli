// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PromptFlow.h"
#include "ShowFlow.h"
#include <winget/UserSettings.h>

using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        bool IsInteractivityAllowed(Execution::Context& context)
        {
            // Interactivity can be disabled for several reasons:
            //   * We are running in a non-interactive context (e.g., COM call)
            //   * It is disabled in the settings
            //   * It was disabled from the command line

            if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::DisableInteractivity))
            {
                AICLI_LOG(CLI, Verbose, << "Skipping prompt. Interactivity is disabled due to non-interactive context.");
                return false;
            }

            if (context.Args.Contains(Execution::Args::Type::DisableInteractivity))
            {
                AICLI_LOG(CLI, Verbose, << "Skipping prompt. Interactivity is disabled by command line argument.");
                return false;
            }

            if (Settings::User().Get<Settings::Setting::InteractivityDisable>())
            {
                AICLI_LOG(CLI, Verbose, << "Skipping prompt. Interactivity is disabled in settings.");
                return false;
            }

            return true;
        }

        bool HandleSourceAgreementsForOneSource(Execution::Context& context, const Repository::Source& source)
        {
            auto details = source.GetDetails();
            AICLI_LOG(CLI, Verbose, << "Checking Source agreements for source: " << details.Name);

            if (source.CheckSourceAgreements())
            {
                AICLI_LOG(CLI, Verbose, << "Source agreements satisfied. Source: " << details.Name);
                return true;
            }

            // Show source agreements
            context.Reporter.Info()
                << Execution::SourceInfoEmphasis
                << Resource::String::SourceAgreementsTitle(Utility::LocIndView{ details.Name })
                << std::endl;

            const auto& agreements = source.GetInformation().SourceAgreements;

            for (const auto& agreement : agreements)
            {
                if (!agreement.Label.empty())
                {
                    context.Reporter.Info() << Execution::SourceInfoEmphasis << Utility::LocIndString{ agreement.Label } << ": "_liv;
                }

                if (!agreement.Text.empty())
                {
                    context.Reporter.Info() << Utility::LocIndString{ agreement.Text } << std::endl;
                }

                if (!agreement.Url.empty())
                {
                    context.Reporter.Info() << Utility::LocIndString{ agreement.Url } << std::endl;
                }
            }

            // Show message for each individual implicit agreement field
            auto fields = source.GetAgreementFieldsFromSourceInformation();
            if (WI_IsFlagSet(fields, Repository::ImplicitAgreementFieldEnum::Market))
            {
                context.Reporter.Info() << Resource::String::SourceAgreementsMarketMessage << std::endl;
            }

            context.Reporter.Info() << std::endl;

            bool accepted = context.Args.Contains(Execution::Args::Type::AcceptSourceAgreements);

            if (!accepted && IsInteractivityAllowed(context))
            {
                accepted = context.Reporter.PromptForBoolResponse(Resource::String::SourceAgreementsPrompt);
            }

            if (accepted)
            {
                AICLI_LOG(CLI, Verbose, << "Source agreements accepted. Source: " << details.Name);
                source.SaveAcceptedSourceAgreements();
            }
            else
            {
                AICLI_LOG(CLI, Verbose, << "Source agreements not accepted. Source: " << details.Name);
            }

            return accepted;
        }

        // An interface for defining prompts to the user regarding a package.
        // Note that each prompt may behave differently when running non-interactively
        // (e.g. failing if it is needed vs. continuing silently), and they may
        // do some work while checking if the prompt is needed even if no prompt is shown,
        // so they need to always run.
        struct PackagePrompt
        {
            virtual ~PackagePrompt() = default;

            // Determines whether a package needs this prompt.
            // Inputs: Manifest, Installer
            // Outputs: None
            virtual bool PackageNeedsPrompt(Execution::Context& context) = 0;

            // Prompts for the information needed for a single package.
            // Inputs: Manifest, Installer
            // Outputs: None
            virtual void PromptForSinglePackage(Execution::Context& context) = 0;

            // Prompts for the information needed for multiple packages.
            // Inputs: Manifest, Installer (for each sub context)
            // Outputs: None
            virtual void PromptForMultiplePackages(Execution::Context& context, std::vector<Execution::Context*>& packagesToPrompt) = 0;
        };

        // Prompt for accepting package agreements.
        struct PackageAgreementsPrompt : public PackagePrompt
        {
            PackageAgreementsPrompt(bool ensureAgreementsAcceptance) : m_ensureAgreementsAcceptance(ensureAgreementsAcceptance) {}

            bool PackageNeedsPrompt(Execution::Context& context) override
            {
                const auto& agreements = context.Get<Execution::Data::Manifest>().CurrentLocalization.Get<AppInstaller::Manifest::Localization::Agreements>();
                return !agreements.empty();
            }

            void PromptForSinglePackage(Execution::Context& context) override
            {
                ShowPackageAgreements(context);
                EnsurePackageAgreementsAcceptance(context, /* showPrompt */ true);
            }

            void PromptForMultiplePackages(Execution::Context& context, std::vector<Execution::Context*>& packagesToPrompt) override
            {
                for (auto packageContext : packagesToPrompt)
                {
                    // Show agreements for each package
                    Execution::Context& showContext = *packageContext;
                    auto previousThreadGlobals = showContext.SetForCurrentThread();

                    ShowPackageAgreements(showContext);
                    if (showContext.IsTerminated())
                    {
                        AICLI_TERMINATE_CONTEXT(showContext.GetTerminationHR());
                    }
                }

                EnsurePackageAgreementsAcceptance(context, /* showPrompt */ false);
            }

        private:
            void ShowPackageAgreements(Execution::Context& context)
            {
                const auto& manifest = context.Get<Execution::Data::Manifest>();
                auto agreements = manifest.CurrentLocalization.Get<AppInstaller::Manifest::Localization::Agreements>();

                if (agreements.empty())
                {
                    // Nothing to do
                    return;
                }

                context << Workflow::ReportManifestIdentityWithVersion(Resource::String::ReportIdentityForAgreements) << Workflow::ShowAgreementsInfo;
                context.Reporter.EmptyLine();
            }

            void EnsurePackageAgreementsAcceptance(Execution::Context& context, bool showPrompt) const
            {
                if (!m_ensureAgreementsAcceptance)
                {
                    return;
                }

                if (context.Args.Contains(Execution::Args::Type::AcceptPackageAgreements))
                {
                    AICLI_LOG(CLI, Info, << "Package agreements accepted by CLI flag");
                    return;
                }

                if (showPrompt)
                {
                    AICLI_LOG(CLI, Verbose, << "Prompting to accept package agreements");
                    if (IsInteractivityAllowed(context))
                    {
                        bool accepted = context.Reporter.PromptForBoolResponse(Resource::String::PackageAgreementsPrompt);
                        if (accepted)
                        {
                            AICLI_LOG(CLI, Info, << "Package agreements accepted in prompt");
                            return;
                        }
                        else
                        {
                            AICLI_LOG(CLI, Info, << "Package agreements not accepted in prompt");
                        }
                    }
                }

                AICLI_LOG(CLI, Error, << "Package agreements were not agreed to.");
                context.Reporter.Error() << Resource::String::PackageAgreementsNotAgreedTo << std::endl;
                AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
            }

            bool m_ensureAgreementsAcceptance;
        };

        // Prompt for getting the install root when a package requires it and it is not
        // specified by the settings.
        struct InstallRootPrompt : public PackagePrompt
        {
            InstallRootPrompt() : m_installLocation(User().Get<Setting::InstallDefaultRoot>()) {}

            bool PackageNeedsPrompt(Execution::Context& context) override
            {
                if (context.Get<Execution::Data::Installer>()->InstallLocationRequired &&
                    !context.Args.Contains(Execution::Args::Type::InstallLocation))
                {
                    AICLI_LOG(CLI, Info, << "Package [" << context.Get<Execution::Data::Manifest>().Id << "] requires an install location.");

                    // An install location is required but one wasn't provided.
                    // Check if there is a default one from settings.
                    if (m_installLocation.empty())
                    {
                        // We need to prompt
                        return true;
                    }
                    else
                    {
                        // Use the default
                        SetInstallLocation(context);
                    }
                }

                return false;
            }

            void PromptForSinglePackage(Execution::Context& context) override
            {
                context.Reporter.Info() << Resource::String::InstallerRequiresInstallLocation << std::endl;
                PromptForInstallRoot(context);

                // When prompting for a single package, we use the provided location directly.
                // This is different from when we prompt for multiple packages or use the root in the settings.
                context.Args.AddArg(Execution::Args::Type::InstallLocation, m_installLocation.u8string());
            }

            void PromptForMultiplePackages(Execution::Context& context, std::vector<Execution::Context*>& packagesToPrompt) override
            {
                // Report packages that will be affected.
                context.Reporter.Info() << Resource::String::InstallersRequireInstallLocation << std::endl;
                for (auto packageContext : packagesToPrompt)
                {
                    *packageContext << ReportManifestIdentityWithVersion(" - "_liv,  Execution::Reporter::Level::Warning);
                    if (packageContext->IsTerminated())
                    {
                        AICLI_TERMINATE_CONTEXT(packageContext->GetTerminationHR());
                    }
                }

                PromptForInstallRoot(context);

                // Set the install location for each package.
                for (auto packageContext : packagesToPrompt)
                {
                    SetInstallLocation(*packageContext);
                }
            }

        private:
            void PromptForInstallRoot(Execution::Context& context)
            {
                if (!IsInteractivityAllowed(context))
                {
                    AICLI_LOG(CLI, Error, << "Install location is required but was not provided.");
                    context.Reporter.Error() << Resource::String::InstallLocationNotProvided << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_LOCATION_REQUIRED);
                }

                AICLI_LOG(CLI, Info, << "Prompting for install root.");
                m_installLocation = context.Reporter.PromptForPath(Resource::String::PromptForInstallRoot);
                if (m_installLocation.empty())
                {
                    AICLI_LOG(CLI, Error, << "Install location is required but the provided path was empty.");
                    context.Reporter.Error() << Resource::String::InstallLocationNotProvided << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_LOCATION_REQUIRED);
                }
                AICLI_LOG(CLI, Info, << "Proceeding with installation using install root: " << m_installLocation);
            }

            // Sets the install location for an execution context.
            // The install location is obtained by appending the package ID to the install root.
            // This function assumes that m_installLocation is set, either from settings or from the prompt,
            // and that the context does not already have an install location.
            void SetInstallLocation(Execution::Context& context)
            {
                auto packageId = context.Get<Execution::Data::Manifest>().Id;
                auto installLocation = m_installLocation;
                installLocation += "\\" + packageId;
                AICLI_LOG(CLI, Info, << "Setting install location for package [" << packageId << "] to: " << installLocation);
                context.Args.AddArg(Execution::Args::Type::InstallLocation, installLocation.u8string());
            }

            std::filesystem::path m_installLocation;
        };

        // Prompt asking whether to continue when an installer will abort the terminal.
        struct InstallerAbortsTerminalPrompt : public PackagePrompt
        {
            bool PackageNeedsPrompt(Execution::Context& context) override
            {
                return context.Get<Execution::Data::Installer>()->InstallerAbortsTerminal;
            }

            void PromptForSinglePackage(Execution::Context& context) override
            {
                AICLI_LOG(CLI, Info, << "This installer may abort the terminal");
                context.Reporter.Warn() << Resource::String::InstallerAbortsTerminal << std::endl;
                PromptToProceed(context);
            }

            void PromptForMultiplePackages(Execution::Context& context, std::vector<Execution::Context*>& packagesToPrompt) override
            {
                AICLI_LOG(CLI, Info, << "One or more installers may abort the terminal");
                context.Reporter.Warn() << Resource::String::InstallersAbortTerminal << std::endl;
                for (auto packageContext : packagesToPrompt)
                {
                    *packageContext << ReportManifestIdentityWithVersion(" - "_liv, Execution::Reporter::Level::Warning);
                    if (packageContext->IsTerminated())
                    {
                        AICLI_TERMINATE_CONTEXT(packageContext->GetTerminationHR());
                    }
                }

                PromptToProceed(context);
            }

        private:
            void PromptToProceed(Execution::Context& context)
            {
                AICLI_LOG(CLI, Info, << "Prompting before proceeding with installer that aborts terminal.");
                if (!IsInteractivityAllowed(context))
                {
                    return;
                }

                bool accepted = context.Reporter.PromptForBoolResponse(Resource::String::PromptToProceed, Reporter::Level::Warning, true);
                if (accepted)
                {
                    AICLI_LOG(CLI, Info, << "Proceeding with installation");
                }
                else
                {
                    AICLI_LOG(CLI, Error, << "Aborting installation");
                    context.Reporter.Error() << Resource::String::Cancelled << std::endl;
                    AICLI_TERMINATE_CONTEXT(E_ABORT);
                }
            }
        };

        // Gets all the prompts that may be displayed, in order of appearance
        std::vector<std::unique_ptr<PackagePrompt>> GetPackagePrompts(bool ensureAgreementsAcceptance = true, bool installerDownloadOnly = false)
        {
            std::vector<std::unique_ptr<PackagePrompt>> result;

            if (installerDownloadOnly)
            {
                result.push_back(std::make_unique<PackageAgreementsPrompt>(ensureAgreementsAcceptance));
            }
            else
            {
                result.push_back(std::make_unique<PackageAgreementsPrompt>(ensureAgreementsAcceptance));
                result.push_back(std::make_unique<InstallRootPrompt>());
                result.push_back(std::make_unique<InstallerAbortsTerminalPrompt>());
            }

            return result;
        }
    }

    void HandleSourceAgreements::operator()(Execution::Context& context) const
    {
        bool allAccepted = true;

        if (m_source.IsComposite())
        {
            for (auto const& source : m_source.GetAvailableSources())
            {
                if (!HandleSourceAgreementsForOneSource(context, source))
                {
                    allAccepted = false;
                }
            }
        }
        else
        {
            allAccepted = HandleSourceAgreementsForOneSource(context, m_source);
        }

        if (!allAccepted)
        {
            context.Reporter.Error() << Resource::String::SourceAgreementsNotAgreedTo << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_SOURCE_AGREEMENTS_NOT_ACCEPTED);
        }
    }

    void ShowPromptsForSinglePackage::operator()(Execution::Context& context) const
    {
        bool installerDownloadOnly = WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerDownloadOnly);

        for (auto& prompt : GetPackagePrompts(true, installerDownloadOnly))
        {
            // Show the prompt if needed
            if (prompt->PackageNeedsPrompt(context))
            {
                prompt->PromptForSinglePackage(context);
            }

            if (context.IsTerminated())
            {
                return;
            }
        }
    }

    void ShowPromptsForMultiplePackages::operator()(Execution::Context& context) const
    {
        for (auto& prompt : GetPackagePrompts(m_ensureAgreementsAcceptance, m_installerDownloadOnly))
        {
            // Find which packages need this prompt
            std::vector<Execution::Context*> packagesToPrompt;
            for (auto& packageContext : context.Get<Execution::Data::PackageSubContexts>())
            {
                if (prompt->PackageNeedsPrompt(*packageContext))
                {
                    packagesToPrompt.push_back(packageContext.get());
                }
            }

            // Prompt only if needed
            if (!packagesToPrompt.empty())
            {
                prompt->PromptForMultiplePackages(context, packagesToPrompt);
                if (context.IsTerminated())
                {
                    return;
                }
            }
        }
    }

    void RequireInteractivity::operator()(Execution::Context& context) const
    {
        if (!IsInteractivityAllowed(context))
        {
            AICLI_TERMINATE_CONTEXT(m_nonInteractiveError);
        }
    }
}
