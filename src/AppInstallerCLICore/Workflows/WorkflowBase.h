// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionArgs.h"
#include "ExecutionReporter.h"
#include <winget/ExperimentalFeature.h>
#include <winget/RepositorySearch.h>
#include <winget/RepositorySource.h>
#include <winget/Authentication.h>

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
        Initial = 0,
        ParseArgs = 1000,
        Discovery = 2000,
        Download = 3000,
        PreExecution = 3500,
        Execution = 4000,
        PostExecution = 5000,
    };

    enum class OperationType
    {
        Completion,
        Export,
        Install,
        List,
        Pin,
        Search,
        Show,
        Uninstall,
        Upgrade,
        Download,
        Repair,
    };

    // A task in the workflow.
    struct WorkflowTask
    {
        using Func = void (*)(Execution::Context&);

        WorkflowTask(Func f) : m_isFunc(true), m_func(f) {}
        WorkflowTask(std::string_view name, bool executeAlways = false) : m_name(name), m_executeAlways(executeAlways) {}

        virtual ~WorkflowTask() = default;

        WorkflowTask(const WorkflowTask&) = default;
        WorkflowTask& operator=(const WorkflowTask&) = default;

        WorkflowTask(WorkflowTask&&) = default;
        WorkflowTask& operator=(WorkflowTask&&) = default;

        bool operator==(const WorkflowTask& other) const;

        virtual void operator()(Execution::Context& context) const;

        const std::string& GetName() const { return m_name; }
        bool IsFunction() const { return m_isFunc; }
        Func Function() const { return m_func; }
        bool ExecuteAlways() const { return m_executeAlways; }
        void Log() const;

    private:
        bool m_isFunc = false;
        Func m_func = nullptr;
        std::string m_name;
        bool m_executeAlways = false;
    };

    // Helper to determine installed source to use based on context input.
    Repository::PredefinedSource DetermineInstalledSource(const Execution::Context& context);

    // Helper to create authentication arguments from context input.
    Authentication::AuthenticationArguments GetAuthenticationArguments(const Execution::Context& context);

    // Helper to report exceptions and return the HRESULT.
    // If context is null, no output will be attempted.
    HRESULT HandleException(Execution::Context* context, std::exception_ptr exception);

    // Helper to report exceptions and return the HRESULT.
    HRESULT HandleException(Execution::Context& context, std::exception_ptr exception);

    // Creates the source object.
    // Required Args: None
    // Inputs: None
    // Outputs: Source
    struct OpenSource : public WorkflowTask
    {
        OpenSource(bool forDependencies = false) : WorkflowTask("OpenSource"), m_forDependencies(forDependencies) {}

        void operator()(Execution::Context& context) const override;
    
    private:
        bool m_forDependencies;
    };

    // Creates a source object for a source specified by name, and adds it to the list of open sources.
    // Required Args: None
    // Inputs: Sources?
    // Outputs: Sources
    struct OpenNamedSourceForSources : public WorkflowTask
    {
        OpenNamedSourceForSources(std::string_view sourceName) : WorkflowTask("OpenNamedSourceForSources"), m_sourceName(sourceName) {}

        void operator()(Execution::Context& context) const override;

    private:
        Utility::LocIndView m_sourceName;
    };

    // Creates a source object for a predefined source.
    // Required Args: None
    // Inputs: None
    // Outputs: Source
    struct OpenPredefinedSource : public WorkflowTask
    {
        OpenPredefinedSource(Repository::PredefinedSource source, bool forDependencies = false) : WorkflowTask("OpenPredefinedSource"), m_predefinedSource(source), m_forDependencies(forDependencies) {}

        void operator()(Execution::Context& context) const override;

    private:
        Repository::PredefinedSource m_predefinedSource;
        bool m_forDependencies;
    };

    // Creates a composite source from the given predefined source and the existing source.
    // Required Args: None
    // Inputs: Source
    // Outputs: Source
    struct OpenCompositeSource : public WorkflowTask
    {
        OpenCompositeSource(
            Repository::PredefinedSource source,
            bool forDependencies = false,
            Repository::CompositeSearchBehavior searchBehavior = Repository::CompositeSearchBehavior::Installed) :
            WorkflowTask("OpenCompositeSource"), m_predefinedSource(source), m_forDependencies(forDependencies), m_searchBehavior(searchBehavior) {}

        void operator()(Execution::Context& context) const override;

    private:
        Repository::PredefinedSource m_predefinedSource;
        bool m_forDependencies;
        Repository::CompositeSearchBehavior m_searchBehavior;
    };

    // Performs a search on the source.
    // Required Args: None
    // Inputs: Source
    // Outputs: SearchResult
    void SearchSourceForMany(Execution::Context& context);

    // Creates a search request object with the semantics of targeting a single package.
    // Required Args: None
    // Inputs: Query, search filters (Id, Name, etc.)
    // Outputs: SearchRequest
    void GetSearchRequestForSingle(Execution::Context& context);

    // Performs a search on the source with the semantics of targeting a single package.
    // Required Args: None
    // Inputs: Source, SearchRequest
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

    // Handles failures in the SearchResult either by warning or failing.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void HandleSearchResultFailures(Execution::Context& context);

    // Outputs the search results when multiple packages found but only one expected.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void ReportMultiplePackageFoundResult(Execution::Context& context);

    // Outputs the search results when multiple packages found but only one expected.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void ReportMultiplePackageFoundResultWithSource(Execution::Context& context);

    // Ensures that there is at least one result in the search.
    // Required Args: bool indicating if the search result is from installed source
    // Inputs: SearchResult
    // Outputs: None
    struct EnsureMatchesFromSearchResult : public WorkflowTask
    {
        EnsureMatchesFromSearchResult(OperationType operation) :
            WorkflowTask("EnsureMatchesFromSearchResult"), m_operationType(operation) {}

        void operator()(Execution::Context& context) const override;

    private:
        OperationType m_operationType;
    };

    // Ensures that there is only one result in the search.
    // Required Args: bool indicating if the search result is from installed source
    // Inputs: SearchResult
    // Outputs: Package
    struct EnsureOneMatchFromSearchResult : public WorkflowTask
    {
        EnsureOneMatchFromSearchResult(OperationType operation) :
            WorkflowTask("EnsureOneMatchFromSearchResult"), m_operationType(operation) {}

        void operator()(Execution::Context& context) const override;

    private:
        OperationType m_operationType;
    };

    // Gets the manifest from package.
    // Required Args: Version and channel; can be empty. A flag indicating whether to consider package pins
    // Inputs: Package
    // Outputs: Manifest, PackageVersion
    struct GetManifestWithVersionFromPackage : public WorkflowTask
    {
        GetManifestWithVersionFromPackage(std::string_view version, std::string_view channel, bool considerPins) :
            WorkflowTask("GetManifestWithVersionFromPackage"), m_version(version), m_channel(channel), m_considerPins(considerPins) {}

        GetManifestWithVersionFromPackage(const Utility::VersionAndChannel& versionAndChannel, bool considerPins) :
            GetManifestWithVersionFromPackage(versionAndChannel.GetVersion().ToString(), versionAndChannel.GetChannel().ToString(), considerPins) {}

        void operator()(Execution::Context& context) const override;

    private:
        std::string_view m_version;
        std::string_view m_channel;
        bool m_considerPins;
    };

    // Gets the manifest from package.
    // Required Args: A value indicating whether to consider pins
    // Inputs: Package. Optionally Version and Channel
    // Outputs: Manifest, PackageVersion
    struct GetManifestFromPackage : public WorkflowTask
    {
        GetManifestFromPackage(bool considerPins) : WorkflowTask("GetManifestFromPackage"), m_considerPins(considerPins) {}

        void operator()(Execution::Context& context) const override;

    private:
        bool m_considerPins;
    };

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

    // Ensures the local file exists and is not a directory. Or it's a Uri. Default only https is supported at the moment.
    // Required Args: the one given
    // Inputs: None
    // Outputs: None
    struct VerifyFileOrUri : public WorkflowTask
    {
        VerifyFileOrUri(Execution::Args::Type arg, std::vector<std::wstring> supportedSchemes = { L"https" }) :
            WorkflowTask("VerifyFileOrUri"), m_arg(arg), m_supportedSchemes(std::move(supportedSchemes)) {}

        void operator()(Execution::Context& context) const override;

    private:
        Execution::Args::Type m_arg;
        std::vector<std::wstring> m_supportedSchemes;
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

    // Reports the installed package version identity.
    // Required Args: None
    // Inputs: InstalledPackageVersion
    // Outputs: None
    void ReportInstalledPackageVersionIdentity(Execution::Context& context);

    // Reports the manifest's identity.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    void ReportManifestIdentity(Execution::Context& context);

    // Reports the manifest's identity with version.
    // Required Args: None
    // Inputs: Manifest
    // Outputs: None
    struct ReportManifestIdentityWithVersion : public WorkflowTask
    {
        ReportManifestIdentityWithVersion(Utility::LocIndView prefix, Execution::Reporter::Level level = Execution::Reporter::Level::Info) :
            WorkflowTask("ReportManifestIdentityWithVersion"), m_prefix(prefix), m_level(level) {}
        ReportManifestIdentityWithVersion(Resource::StringId label = Resource::String::ReportIdentityFound, Execution::Reporter::Level level = Execution::Reporter::Level::Info) :
            WorkflowTask("ReportManifestIdentityWithVersion"), m_label(label), m_level(level) {}

        void operator()(Execution::Context& context) const override;

    private:
        Utility::LocIndView m_prefix;
        std::optional<Resource::StringId> m_label;
        Execution::Reporter::Level m_level;
    };

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

    // Shows all versions for an application.
    // Required Args: None
    // Inputs: SearchResult [only operates on first match]
    // Outputs: None
    void ShowAppVersions(Execution::Context& context);

    // Reports execution stage in a workflow
    // Required Args: ExecutionStage
    // Inputs: ExecutionStage?
    // Outputs: ExecutionStage
    struct ReportExecutionStage : public WorkflowTask
    {
        ReportExecutionStage(ExecutionStage stage) : WorkflowTask("ReportExecutionStage"), m_stage(stage) {}

        void operator()(Execution::Context& context) const override;

    private:
        ExecutionStage m_stage;
    };
}

// Passes the context to the function if it has not been terminated; returns the context.
AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, AppInstaller::CLI::Workflow::WorkflowTask::Func f);

// Passes the context to the task if it has not been terminated; returns the context.
AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, const AppInstaller::CLI::Workflow::WorkflowTask& task);
