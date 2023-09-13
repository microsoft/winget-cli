// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "DependenciesFlow.h"
#include "ManifestComparator.h"
#include "InstallFlow.h"
#include "winget\DependenciesGraph.h"
#include "winget\WindowsFeature.h"
#include "DependencyNodeProcessor.h"

using namespace AppInstaller::Repository;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::WindowsFeature;

namespace AppInstaller::CLI::Workflow
{
    namespace
    {
        // Contains all the information needed to install a dependency package.
        struct DependencyPackageCandidate
        {
            DependencyPackageCandidate(
                std::shared_ptr<Repository::IPackageVersion>&& packageVersion,
                std::shared_ptr<Repository::IPackageVersion>&& installedPackageVersion,
                Manifest::Manifest&& manifest,
                Manifest::ManifestInstaller&& installer)
                : PackageVersion(std::move(packageVersion)), InstalledPackageVersion(std::move(installedPackageVersion)), Manifest(std::move(manifest)), Installer(std::move(installer)) { }

            std::shared_ptr<Repository::IPackageVersion> PackageVersion;
            std::shared_ptr<Repository::IPackageVersion> InstalledPackageVersion;
            Manifest::Manifest Manifest;
            Manifest::ManifestInstaller Installer;
        };
    }

    void ReportDependencies::operator()(Execution::Context& context) const
    {
        auto info = context.Reporter.Info();

        const auto& dependencies = context.Get<Execution::Data::Dependencies>();
        if (dependencies.HasAny())
        {
            info << m_messageId << std::endl;

            if (dependencies.HasAnyOf(DependencyType::WindowsFeature))
            {
                info << "  - " << Resource::String::WindowsFeaturesDependencies << std::endl;
                dependencies.ApplyToType(DependencyType::WindowsFeature, [&info](Dependency dependency) {info << "      " << dependency.Id() << std::endl; });
            }

            if (dependencies.HasAnyOf(DependencyType::WindowsLibrary))
            {
                info << "  - " << Resource::String::WindowsLibrariesDependencies << std::endl;
                dependencies.ApplyToType(DependencyType::WindowsLibrary, [&info](Dependency dependency) {info << "      " << dependency.Id() << std::endl; });
            }

            if (dependencies.HasAnyOf(DependencyType::Package))
            {
                info << "  - " << Resource::String::PackageDependencies << std::endl;
                dependencies.ApplyToType(DependencyType::Package, [&info](Dependency dependency)
                    {
                        info << "      " << dependency.Id();
                        if (dependency.MinVersion)
                        {
                            info << " [>= " << dependency.MinVersion.value().ToString() << "]";
                        }
                        info << std::endl;
                    });
            }

            if (dependencies.HasAnyOf(DependencyType::External))
            {
                context.Reporter.Warn() << "  - " << Resource::String::ExternalDependencies << std::endl;
                dependencies.ApplyToType(DependencyType::External, [&info](Dependency dependency) {info << "      " << dependency.Id() << std::endl; });
            }
        }
    }

    void GetInstallersDependenciesFromManifest(Execution::Context& context)
    {
        const auto& manifest = context.Get<Execution::Data::Manifest>();
        DependencyList allDependencies;

        for (const auto& installer : manifest.Installers)
        {
            allDependencies.Add(installer.Dependencies);
        }

        context.Add<Execution::Data::Dependencies>(std::move(allDependencies));
    }

    void GetDependenciesFromInstaller(Execution::Context& context)
    {
        const auto& installer = context.Get<Execution::Data::Installer>();
        if (installer)
        {
            context.Add<Execution::Data::Dependencies>(installer->Dependencies);
        }
    }

    void GetDependenciesInfoForUninstall(Execution::Context& context)
    {
        // TODO make best effort to get the correct installer information, it may be better to have a record of installations and save the correct installers
        context.Add<Execution::Data::Dependencies>(DependencyList()); // sending empty list of dependencies for now
    }

    void OpenDependencySource(Execution::Context& context)
    {
        if (context.Contains(Execution::Data::PackageVersion))
        {
            const auto& packageVersion = context.Get<Execution::Data::PackageVersion>();
            context.Add<Execution::Data::DependencySource>(packageVersion->GetSource());
        }
        else
        {
            // install from manifest requires --dependency-source to be set
            context <<
                Workflow::OpenSource(true);
        }

        if (WI_IsFlagClear(context.GetFlags(), Execution::ContextFlag::InstallerDownloadOnly))
        {
            // Installed source is not needed when only downloading the installer.
            context <<
                Workflow::OpenCompositeSource(Repository::PredefinedSource::Installed, true, Repository::CompositeSearchBehavior::AvailablePackages);
        }
    }

    void EnableWindowsFeaturesDependencies(Execution::Context& context)
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::WindowsFeature))
        {
            return;
        }

        const auto& rootDependencies = context.Get<Execution::Data::Installer>()->Dependencies;

        if (rootDependencies.Empty())
        {
            return;
        }

        if (rootDependencies.HasAnyOf(DependencyType::WindowsFeature))
        {
            context << Workflow::EnsureRunningAsAdmin;
            if (context.IsTerminated())
            {
                return;
            }

            HRESULT hr = S_OK;
            std::shared_ptr<DismHelper> dismHelper = std::make_shared<DismHelper>();

            bool force = context.Args.Contains(Execution::Args::Type::Force);
            bool rebootRequired = false;

            rootDependencies.ApplyToType(DependencyType::WindowsFeature, [&context, &hr, &dismHelper, &force, &rebootRequired](Dependency dependency)
                {
                    if (SUCCEEDED(hr) || force)
                    {
                        auto featureName = dependency.Id();
                        WindowsFeature::WindowsFeature windowsFeature = dismHelper->GetWindowsFeature(featureName);

                        if (windowsFeature.DoesExist())
                        {
                            if (!windowsFeature.IsEnabled())
                            {
                                Utility::LocIndString featureDisplayName = windowsFeature.GetDisplayName();
                                Utility::LocIndView locIndFeatureName{ featureName };

                                context.Reporter.Info() << Resource::String::EnablingWindowsFeature(featureDisplayName, locIndFeatureName) << std::endl;

                                AICLI_LOG(Core, Info, << "Enabling Windows Feature [" << featureName << "] returned with HRESULT: " << hr);
                                auto enableFeatureFunction = [&](IProgressCallback& progress)->HRESULT { return windowsFeature.Enable(progress); };
                                hr = context.Reporter.ExecuteWithProgress(enableFeatureFunction, true);

                                if (FAILED(hr))
                                {
                                    AICLI_LOG(Core, Error, << "Failed to enable Windows Feature " << featureDisplayName << " [" << locIndFeatureName << "] with exit code: " << hr);
                                    context.Reporter.Warn() << Resource::String::FailedToEnableWindowsFeature(featureDisplayName, locIndFeatureName) << std::endl
                                        << GetUserPresentableMessage(hr) << std::endl;
                                }

                                if (hr == ERROR_SUCCESS_REBOOT_REQUIRED || windowsFeature.GetRestartRequiredStatus() == DismRestartType::DismRestartRequired)
                                {
                                    rebootRequired = true;
                                }
                            }
                        }
                        else
                        {
                            // Note: If a feature is not found, continue enabling the rest of the dependencies but block immediately after unless force arg is present.
                            AICLI_LOG(Core, Info, << "Windows Feature [" << featureName << "] does not exist");
                            hr = APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY;
                            context.Reporter.Warn() << Resource::String::WindowsFeatureNotFound(Utility::LocIndView{ featureName }) << std::endl;
                        }
                    }
            });

            if (FAILED(hr))
            {
                if (force)
                {
                    context.Reporter.Warn() << Resource::String::FailedToEnableWindowsFeatureOverridden << std::endl;
                }
                else
                {
                    context.Reporter.Error() << Resource::String::FailedToEnableWindowsFeatureOverrideRequired << std::endl;
                    AICLI_TERMINATE_CONTEXT(hr);
                }
            }
            else if (rebootRequired)
            {
                if (force)
                {
                    context.Reporter.Warn() << Resource::String::RebootRequiredToEnableWindowsFeatureOverridden << std::endl;
                }
                else
                {
                    context.Reporter.Error() << Resource::String::RebootRequiredToEnableWindowsFeatureOverrideRequired << std::endl;
                    AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_REBOOT_REQUIRED_TO_INSTALL);
                }
            }
        }
    }

    void CreateDependencySubContexts::operator()(Execution::Context& context) const
    {
        auto info = context.Reporter.Info();
        auto error = context.Reporter.Error();
        const auto& rootManifest = context.Get<Execution::Data::Manifest>();

        Dependency rootAsDependency = Dependency(DependencyType::Package, rootManifest.Id, rootManifest.Version);

        const auto& rootInstaller = context.Get<Execution::Data::Installer>();
        const auto& rootDependencies = rootInstaller->Dependencies;

        if (rootDependencies.Empty())
        {
            // If there's no dependencies there's nothing to do aside of logging the outcome
            return;
        }

        context << OpenDependencySource;
        if (context.IsTerminated())
        {
            info << Resource::String::DependenciesFlowSourceNotFound << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INTERNAL_ERROR); 
        }

        std::map<string_t, DependencyPackageCandidate> idToPackageMap;
        bool foundError = false;
        DependencyGraph dependencyGraph(rootAsDependency, rootDependencies, 
            [&](Dependency node)
            {
                DependencyNodeProcessor nodeProcessor(context);

                auto result = nodeProcessor.EvaluateDependencies(node);
                DependencyList list = nodeProcessor.GetDependencyList();
                foundError = foundError || (result == DependencyNodeProcessorResult::Error);

                if (result == DependencyNodeProcessorResult::Success)
                {
                    DependencyPackageCandidate dependencyPackageCandidate{
                        std::move(nodeProcessor.GetPackageLatestVersion()),
                        std::move(nodeProcessor.GetPackageInstalledVersion()),
                        std::move(nodeProcessor.GetManifest()),
                        std::move(nodeProcessor.GetPreferredInstaller()) };

                    idToPackageMap.emplace(node.Id(), std::move(dependencyPackageCandidate));
                };

                return list;
            });

        dependencyGraph.BuildGraph();

        if (foundError)
        {
            error << Resource::String::DependenciesManagementExitMessage << std::endl;
            AICLI_TERMINATE_CONTEXT(APPINSTALLER_CLI_ERROR_INSTALL_MISSING_DEPENDENCY);
        }

        if (dependencyGraph.HasLoop())
        {
            context.Reporter.Warn() << Resource::String::DependenciesFlowContainsLoop;
        }

        const auto& installationOrder = dependencyGraph.GetInstallationOrder();

        std::vector<std::unique_ptr<Execution::Context>> dependencyPackageContexts;

        for (auto const& node : installationOrder)
        {
            auto itr = idToPackageMap.find(node.Id());
            // if the package was already installed (with a useful version) or is the root
            // then there will be no installer for it on the map.
            if (itr != idToPackageMap.end())
            {
                auto dependencyContextPtr = context.CreateSubContext();
                Execution::Context& dependencyContext = *dependencyContextPtr;
                auto previousThreadGlobals = dependencyContext.SetForCurrentThread();

                Logging::Telemetry().LogSelectedInstaller(
                    static_cast<int>(itr->second.Installer.Arch),
                    itr->second.Installer.Url,
                    Manifest::InstallerTypeToString(itr->second.Installer.EffectiveInstallerType()),
                    Manifest::ScopeToString(itr->second.Installer.Scope),
                    itr->second.Installer.Locale);

                Logging::Telemetry().LogManifestFields(
                    itr->second.Manifest.Id,
                    itr->second.Manifest.DefaultLocalization.Get<Manifest::Localization::PackageName>(),
                    itr->second.Manifest.Version);

                // Extract the data needed for installing
                dependencyContext.Add<Execution::Data::PackageVersion>(itr->second.PackageVersion);
                dependencyContext.Add<Execution::Data::Manifest>(itr->second.Manifest);
                dependencyContext.Add<Execution::Data::InstalledPackageVersion>(itr->second.InstalledPackageVersion);
                dependencyContext.Add<Execution::Data::Installer>(itr->second.Installer);

                if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::InstallerDownloadOnly))
                {
                    dependencyContext.Add<Execution::Data::DownloadDirectory>(context.Get<Execution::Data::DownloadDirectory>() / "Dependencies");
                }

                dependencyPackageContexts.emplace_back(std::move(dependencyContextPtr));
            }
        }

        context.Add<Execution::Data::PackageSubContexts>(std::move(dependencyPackageContexts));
    }
}