// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DependencyNodeProcessor.h"
#include "WorkflowBase.h"
#include <winget/PinningData.h>
#include <winget/PackageVersionSelection.h>

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
            error << Resource::String::DependenciesFlowNoMatches << std::endl;
            return DependencyNodeProcessorResult::Error;
        }

        if (matches.size() > 1)
        {
            auto dependencyNodeId = Utility::LocIndString{ Utility::Normalize(dependencyNode.Id()) };
            error << Resource::String::DependenciesFlowSourceTooManyMatches(dependencyNodeId) << std::endl;
            AICLI_LOG(CLI, Error, << "Too many matches for package " << dependencyNode.Id());
            return DependencyNodeProcessorResult::Error;
        }

        const auto& match = matches.at(0);
        const auto& package = match.Package;
        auto packageId = package->GetProperty(PackageProperty::Id);
        m_nodePackageInstalledVersion = GetInstalledVersion(package);
        std::shared_ptr<IPackageVersionCollection> availableVersions = GetAvailableVersionsForInstalledVersion(package);

        // If the package is already installed and meets the dependency's MinVersion, skip.
        if (m_nodePackageInstalledVersion && dependencyNode.IsVersionOk(Utility::Version(m_nodePackageInstalledVersion->GetProperty(PackageVersionProperty::Version))))
        {
            // return empty dependency list,
            // as we won't keep searching for dependencies for installed packages
            return DependencyNodeProcessorResult::Skipped;
        }

        if (!availableVersions)
        {
            error << Resource::String::DependenciesFlowPackageVersionNotFound(Utility::LocIndView{ Utility::Normalize(packageId) }) << std::endl;
            AICLI_LOG(CLI, Error, << "Available versions not found for package " << packageId);
            return DependencyNodeProcessorResult::Error;
        }

        // Determine pin behavior: --force should ignore pin restrictions for selection purposes.
        Pinning::PinBehavior pinBehavior = m_context.Args.Contains(Execution::Args::Type::IncludePinned) || m_context.Args.Contains(Execution::Args::Type::Force)
            ? Pinning::PinBehavior::IncludePinned
            : Pinning::PinBehavior::ConsiderPins;

        Pinning::PinningData pinningData{ Pinning::PinningData::Disposition::ReadOnly };
        auto evaluator = pinningData.CreatePinStateEvaluator(pinBehavior, m_nodePackageInstalledVersion);

        // Iterate versions from newest to oldest, looking for the first version that:
        //  - satisfies dependency.MinVersion
        //  - is not pinned (unless includePinned or force)
        //  - has at least one applicable installer according to ManifestComparator
        bool foundCandidate = false;
        IPackageVersion::Metadata installationMetadata;
        if (m_nodePackageInstalledVersion)
        {
            installationMetadata = m_nodePackageInstalledVersion->GetMetadata();
        }

        Manifest::ManifestComparator manifestComparator(GetManifestComparatorOptions(m_context, installationMetadata));

        auto versionKeys = availableVersions->GetVersionKeys();
        for (const auto& key : versionKeys)
        {
            auto candidateVersion = availableVersions->GetVersion(key);
            if (!candidateVersion)
            {
                continue;
            }

            // Skip pinned versions unless explicitly allowed by flags
            auto pinType = evaluator.EvaluatePinType(candidateVersion);
            if (pinType != Pinning::PinType::Unknown && !m_context.Args.Contains(Execution::Args::Type::IncludePinned) && !m_context.Args.Contains(Execution::Args::Type::Force))
            {
                // This version is pinned and the user did not request to include pinned versions
                AICLI_LOG(CLI, Info, << "Skipping pinned version " << candidateVersion->GetProperty(PackageVersionProperty::Version) << " for package " << packageId);
                continue;
            }

            // Check MinVersion constraint from the dependency node
            Utility::Version candidateVer(candidateVersion->GetProperty(PackageVersionProperty::Version));
            if (!dependencyNode.IsVersionOk(candidateVer))
            {
                // Candidate version is lower than required min version for this dependency
                AICLI_LOG(CLI, Info, << "Skipping version " << candidateVer.ToString() << " because it does not meet MinVersion for dependency " << dependencyNode.Id());
                continue;
            }

            // Load manifest for this version and attempt installer selection
            Manifest::Manifest manifest = candidateVersion->GetManifest();
            manifest.ApplyLocale();

            if (manifest.Installers.empty())
            {
                AICLI_LOG(CLI, Info, << "No installers in manifest for " << manifest.Id << " version " << manifest.Version);
                continue;
            }

            auto [installer, inapplicabilities] = manifestComparator.GetPreferredInstaller(manifest);

            if (!installer.has_value())
            {
                // No suitable installer for this manifest; keep searching older versions.
                AICLI_LOG(CLI, Info, << "No suitable installer found for manifest " << manifest.Id << " version " << manifest.Version);
                continue;
            }

            // Found a working candidate
            m_nodePackageLatestVersion = candidateVersion;
            m_nodeManifest = std::move(manifest);
            m_installer = installer.value();
            foundCandidate = true;
            break;
        }

        if (!foundCandidate)
        {
            error << Resource::String::DependenciesFlowNoSuitableInstallerFound(Utility::LocIndView{ Utility::Normalize(packageId) }, Utility::LocIndView{ Utility::LocIndString{ /* empty version to indicate search failed across versions */ "" } }) << std::endl;
            AICLI_LOG(CLI, Error, << "No suitable installer found for any available version of package " << packageId);
            return DependencyNodeProcessorResult::Error;
        }

        // Extract the dependency list from the chosen installer's dependencies
        m_dependenciesList = m_installer.Dependencies;
        return DependencyNodeProcessorResult::Success;
    }
}