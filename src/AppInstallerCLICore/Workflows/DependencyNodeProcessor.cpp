// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DependencyNodeProcessor.h"
#include "ManifestComparator.h"

using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;

namespace AppInstaller::CLI::Workflow
{
    DependencyNodeProcessor::DependencyNodeProcessor(Execution::Context& context)
        : m_context(context) {}

    DependencyNodeProcessorResult DependencyNodeProcessor::EvaluateDependencies(Dependency& dependencyNode)
    {
        SearchRequest searchRequest;
        const auto& source = m_context.Get<Execution::Data::DependencySource>();
        auto error = m_context.Reporter.Error();
        auto info = m_context.Reporter.Info();

        searchRequest.Filters.emplace_back(PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, dependencyNode.Id()));

        const auto& matches = source.Search(searchRequest).Matches;

        if (matches.empty())
        {
            error << Resource::String::DependenciesFlowNoMatches;
            return DependencyNodeProcessorResult::Error;
        }

        if (matches.size() > 1)
        {
            auto dependencyNodeId = Utility::LocIndString{ Utility::Normalize(dependencyNode.Id()) };
            error << Resource::String::DependenciesFlowSourceTooManyMatches(dependencyNodeId);
            AICLI_LOG(CLI, Error, << "Too many matches for package " << dependencyNode.Id());
            return DependencyNodeProcessorResult::Error;
        }

        const auto& match = matches.at(0);
        const auto& package = match.Package;
        auto packageId = package->GetProperty(PackageProperty::Id);
        m_nodePackageInstalledVersion = package->GetInstalledVersion();

        PinBehavior pinBehavior;
        if (m_context.Args.Contains(Execution::Args::Type::Force))
        {
            pinBehavior = PinBehavior::IgnorePins;
        }
        else
        {
            pinBehavior = m_context.Args.Contains(Execution::Args::Type::IncludePinned) ? PinBehavior::IncludePinned : PinBehavior::ConsiderPins;
        }

        m_nodePackageLatestVersion = package->GetLatestAvailableVersion(pinBehavior);

        if (m_nodePackageInstalledVersion && dependencyNode.IsVersionOk(Utility::Version(m_nodePackageInstalledVersion->GetProperty(PackageVersionProperty::Version))))
        {
            // return empty dependency list,
            // as we won't keep searching for dependencies for installed packages
            return DependencyNodeProcessorResult::Skipped;
        }

        
        if (!m_nodePackageLatestVersion)
        {
            error << Resource::String::DependenciesFlowPackageVersionNotFound(Utility::LocIndView{ Utility::Normalize(packageId) });
            AICLI_LOG(CLI, Error, << "Latest available version not found for package " << packageId);
            return DependencyNodeProcessorResult::Error;
        }

        if (!dependencyNode.IsVersionOk(Utility::Version(m_nodePackageLatestVersion->GetProperty(PackageVersionProperty::Version))))
        {
            error << Resource::String::DependenciesFlowNoMinVersion(Utility::LocIndView{ Utility::Normalize(packageId) });
            AICLI_LOG(CLI, Error, << "No suitable min version found for package " << packageId);
            return DependencyNodeProcessorResult::Error;
        }

        m_nodeManifest = m_nodePackageLatestVersion->GetManifest();
        m_nodeManifest.ApplyLocale();

        if (m_nodeManifest.Installers.empty())
        {
            error << Resource::String::DependenciesFlowNoInstallerFound(Utility::LocIndView{ Utility::Normalize(m_nodeManifest.Id) });
            AICLI_LOG(CLI, Error, << "Installer not found for manifest " << m_nodeManifest.Id << " with version" << m_nodeManifest.Version);
            return DependencyNodeProcessorResult::Error;
        }

        IPackageVersion::Metadata installationMetadata;
        if (m_nodePackageInstalledVersion)
        {
            installationMetadata = m_nodePackageInstalledVersion->GetMetadata();
        }

        ManifestComparator manifestComparator(m_context, installationMetadata);
        auto [installer, inapplicabilities] = manifestComparator.GetPreferredInstaller(m_nodeManifest);

        if (!installer.has_value())
        {
            auto manifestId = Utility::LocIndString{ Utility::Normalize(m_nodeManifest.Id) };
            auto manifestVersion = Utility::LocIndString{ m_nodeManifest.Version };
            error << Resource::String::DependenciesFlowNoSuitableInstallerFound(manifestId, manifestVersion);
            AICLI_LOG(CLI, Error, << "No suitable installer found for manifest " << m_nodeManifest.Id << " with version " << m_nodeManifest.Version);
            return DependencyNodeProcessorResult::Error;
        }

        m_installer = installer.value();
        m_dependenciesList = m_installer.Dependencies;
        return DependencyNodeProcessorResult::Success;
    }
}