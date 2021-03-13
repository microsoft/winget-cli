// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionArgs.h"
#include <winget/ExperimentalFeature.h>
#include <AppInstallerRepositorySearch.h>

#include <string>
#include <string_view>


namespace AppInstaller::CLI::Execution
{
    struct Context;
}

namespace AppInstaller::CLI::Workflow
{
    // Values are ordered in a typical workflow stages
    enum class ExecutionStage : uint32_t
    {
        ParseArgs = 1000,
        Discovery = 2000,
        Download = 3000,
        PreExecution = 3500,
        Execution = 4000,
        PostExecution = 5000,
    };

    // A task in the workflow.
    struct WorkflowTask
    {
        using Func = void (*)(Execution::Context&);

        WorkflowTask(Func f) : m_isFunc(true), m_func(f) {}
        WorkflowTask(std::string_view name) : m_name(name) {}

        virtual ~WorkflowTask() = default;

        WorkflowTask(const WorkflowTask&) = default;
        WorkflowTask& operator=(const WorkflowTask&) = default;

        WorkflowTask(WorkflowTask&&) = default;
        WorkflowTask& operator=(WorkflowTask&&) = default;

        bool operator==(const WorkflowTask& other) const;

        virtual void operator()(Execution::Context& context) const;

        const std::string& GetName() const { return m_name; }

    private:
        bool m_isFunc = false;
        Func m_func = nullptr;
        std::string m_name;
    };

    // Creates the source object.
    // Required Args: None
    // Inputs: None
    // Outputs: Source
    void OpenSource(Execution::Context& context);

    // Creates a source object for a source specified by name, and adds it to the list of open sources.
    // Required Args: None
    // Inputs: Sources?
    // Outputs: Sources
    struct OpenNamedSourceForSources : public WorkflowTask
    {
        OpenNamedSourceForSources(std::string_view sourceName) : WorkflowTask("OpenNamedSourceForSources"), m_sourceName(sourceName) {}

        void operator()(Execution::Context& context) const override;

    private:
        std::string_view m_sourceName;
    };

    // Creates a source object for a predefined source.
    // Required Args: None
    // Inputs: None
    // Outputs: Source
    struct OpenPredefinedSource : public WorkflowTask
    {
        OpenPredefinedSource(Repository::PredefinedSource source) : WorkflowTask("OpenPredefinedSource"), m_predefinedSource(source) {}

        void operator()(Execution::Context& context) const override;

    private:
        Repository::PredefinedSource m_predefinedSource;
    };

    // Creates a composite source from the given predefined source and the existing source.
    // Required Args: None
    // Inputs: Source
    // Outputs: Source
    struct OpenCompositeSource : public WorkflowTask
    {
        OpenCompositeSource(Repository::PredefinedSource source) : WorkflowTask("OpenCompositeSource"), m_predefinedSource(source) {}

        void operator()(Execution::Context& context) const override;

    private:
        Repository::PredefinedSource m_predefinedSource;
    };

    // Performs a search on the source.
    // Required Args: None
    // Inputs: Source
    // Outputs: SearchResult
    void SearchSourceForMany(Execution::Context& context);

    // Performs a search on the source with the semantics of targeting a single package.
    // Required Args: None
    // Inputs: Source
    // Outputs: SearchResult
    void SearchSourceForSingle(Execution::Context& context);

    // Performs a search on the source with the semantics of targeting many packages,
    // but for completion purposes.
    // Required Args: None
    // Inputs: Source, CompletionData
    // Outputs: SearchResult
    void SearchSourceForManyCompletion(Execution::Context& context);

    // Performs a search on the source with the semantics of targeting a single package,
    // but for completion purposes.
    // Required Args: None
    // Inputs: Source, CompletionData
    // Outputs: SearchResult
    void SearchSourceForSingleCompletion(Execution::Context& context);

    // Searches the source for the specific field as a completion.
    // Required Args: None
    // Inputs: CompletionData, Source
    // Outputs: None
    struct SearchSourceForCompletionField : public WorkflowTask
    {
        SearchSourceForCompletionField(Repository::PackageMatchField field) : WorkflowTask("SearchSourceForCompletionField"), m_field(field) {}

        void operator()(Execution::Context& context) const override;

    private:
        Repository::PackageMatchField m_field;
    };

    // Outputs the search results.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void ReportSearchResult(Execution::Context& context);

    // Outputs the search results as the list command would show.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    struct ReportListResult : public WorkflowTask
    {
        ReportListResult(bool onlyShowUpgrades = false) : WorkflowTask("ReportListResult"), m_onlyShowUpgrades(onlyShowUpgrades) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_onlyShowUpgrades;
    };

    // Outputs the search results when multiple packages found but only one expected.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void ReportMultiplePackageFoundResult(Execution::Context& context);

    // Ensures that there is at least one result in the search.
    // Required Args: bool indicating if the search result is from installed source
    // Inputs: SearchResult
    // Outputs: None
    struct EnsureMatchesFromSearchResult : public WorkflowTask
    {
        EnsureMatchesFromSearchResult(bool isFromInstalledSource) :
            WorkflowTask("EnsureMatchesFromSearchResult"), m_isFromInstalledSource(isFromInstalledSource) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_isFromInstalledSource;
    };

    // Ensures that there is only one result in the search.
    // Required Args: bool indicating if the search result is from installed source
    // Inputs: SearchResult
    // Outputs: None
    struct EnsureOneMatchFromSearchResult : public WorkflowTask
    {
        EnsureOneMatchFromSearchResult(bool isFromInstalledSource) :
            WorkflowTask("EnsureOneMatchFromSearchResult"), m_isFromInstalledSource(isFromInstalledSource) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_isFromInstalledSource;
    };

    // Gets the manifest from package.
    // Required Args: Version and channel; can be empty
    // Inputs: Package
    // Outputs: Manifest, PackageVersion
    struct GetManifestWithVersionFromPackage : public WorkflowTask
    {
        GetManifestWithVersionFromPackage(const Utility::VersionAndChannel& versionAndChannel) :
            WorkflowTask("GetManifestWithVersionFromPackage"), m_version(versionAndChannel.GetVersion().ToString()), m_channel(versionAndChannel.GetChannel().ToString()) {}

        GetManifestWithVersionFromPackage(std::string_view version, std::string_view channel) :
            WorkflowTask("GetManifestWithVersionFromPackage"), m_version(version), m_channel(channel) {}

        void operator()(Execution::Context& context) const override;

    private:
        std::string_view m_version;
        std::string_view m_channel;
    };

    // Gets the manifest from package.
    // Required Args: None
    // Inputs: Package
    // Outputs: Manifest, PackageVersion
    void GetManifestFromPackage(Execution::Context& context);

    // Ensures the file exists and is not a directory.
    // Required Args: the one given
    // Inputs: None
    // Outputs: None
    struct VerifyFile : public WorkflowTask
    {
        VerifyFile(Execution::Args::Type arg) : WorkflowTask("VerifyFile"), m_arg(arg) {}

        void operator()(Execution::Context& context) const override;

    private:
        Execution::Args::Type m_arg;
    };

    // Ensures the path exists.
    // Required Args: the one given
    // Inputs: None
    // Outputs: None
    struct VerifyPath : public WorkflowTask
    {
        VerifyPath(Execution::Args::Type arg) : WorkflowTask("VerifyPath"), m_arg(arg) {}

        void operator()(Execution::Context& context) const override;

    private:
        Execution::Args::Type m_arg;
    };

    // Opens the manifest file provided on the command line.
    // Required Args: Manifest
    // Inputs: None
    // Outputs: Manifest
    void GetManifestFromArg(Execution::Context& context);

    // Reports the search result's package identity.
    // Required Args: None
    // Inputs: Package
    // Outputs: None
    void ReportPackageIdentity(Execution::Context& context);

    // Reports the manifest's identity.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ReportManifestIdentity(Execution::Context& context);

    // Composite flow that produces a manifest; either from one given on the command line or by searching.
    // Required Args: None
    // Inputs: None
    // Outputs: Manifest
    void GetManifest(Execution::Context& context);

    // Selects the installer from the manifest, if one is applicable.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: Installer
    void SelectInstaller(Execution::Context& context);

    // Ensures that the process is running as admin.
    // Required Args: None
    // Inputs: None
    // Outputs: None
    void EnsureRunningAsAdmin(Execution::Context& context);

    // Ensures that the feature is enabled.
    // Required Args: the desired feature
    // Inputs: None
    // Outputs: None
    struct EnsureFeatureEnabled : public WorkflowTask
    {
        EnsureFeatureEnabled(Settings::ExperimentalFeature::Feature feature) : WorkflowTask("EnsureFeatureEnabled"), m_feature(feature) {}

        void operator()(Execution::Context& context) const override;

    private:
        Settings::ExperimentalFeature::Feature m_feature;
    };

    // Performs a search on the source with the semantics of targeting packages matching input manifest
    // Required Args: None
    // Inputs: Source, Manifest
    // Outputs: SearchResult
    void SearchSourceUsingManifest(Execution::Context& context);

    // Gets the installed package version
    // Required Args: None
    // Inputs: Package
    // Outputs: InstalledPackageVersion
    void GetInstalledPackageVersion(Execution::Context& context);

    // Reports execution stage in a workflow
    // Required Args: ExecutionStage
    // Inputs: ExecutionStage?
    // Outputs: ExecutionStage
    struct ReportExecutionStage : public WorkflowTask
    {
        ReportExecutionStage(ExecutionStage stage, bool allowBackward = false) : WorkflowTask("ReportExecutionStage"), m_stage(stage), m_allowBackward(allowBackward) {}

        void operator()(Execution::Context& context) const override;

    private:
        ExecutionStage m_stage;
        bool m_allowBackward;
    };
}

// Passes the context to the function if it has not been terminated; returns the context.
AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, AppInstaller::CLI::Workflow::WorkflowTask::Func f);

// Passes the context to the task if it has not been terminated; returns the context.
AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, const AppInstaller::CLI::Workflow::WorkflowTask& task);
