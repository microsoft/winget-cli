// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ExecutionArgs.h"

#include <winget/ExperimentalFeature.h>
#include <string>
#include <string_view>


namespace AppInstaller::CLI::Execution
{
    struct Context;
}

namespace AppInstaller::CLI::Workflow
{
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

    // Performs a search on the source.
    // Required Args: None
    // Inputs: Source
    // Outputs: SearchResult
    void SearchSourceForMany(Execution::Context& context);

    // Performs a search on the source with the semantics of targeting a single application.
    // Required Args: None
    // Inputs: Source
    // Outputs: SearchResult
    void SearchSourceForSingle(Execution::Context& context);

    // Outputs the search results.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void ReportSearchResult(Execution::Context& context);

    // Ensures that there is at least one result in the search.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void EnsureMatchesFromSearchResult(Execution::Context& context);

    // Ensures that there is only one result in the search.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: None
    void EnsureOneMatchFromSearchResult(Execution::Context& context);

    // Gets the manifest from a search result.
    // Required Args: None
    // Inputs: SearchResult
    // Outputs: Manifest
    void GetManifestFromSearchResult(Execution::Context& context);

    // Ensures the the file exists and is not a directory.
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

    // Opens the manifest file provided on the command line.
    // Required Args: Manifest
    // Inputs: None
    // Outputs: Manifest
    void GetManifestFromArg(Execution::Context& context);

    // Reports the search result's identity.
    // Required Args: None
    // Inputs: SearchResult (only 1)
    // Outputs: None
    void ReportSearchResultIdentity(Execution::Context& context);

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
}

// Passes the context to the function if it has not been terminated; returns the context.
AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, AppInstaller::CLI::Workflow::WorkflowTask::Func f);

// Passes the context to the task if it has not been terminated; returns the context.
AppInstaller::CLI::Execution::Context& operator<<(AppInstaller::CLI::Execution::Context& context, const AppInstaller::CLI::Workflow::WorkflowTask& task);
