// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "ExecutionContext.h"
#include "Public/AppInstallerRepositorySearch.h"
#include "Public/AppInstallerRepositorySource.h"

namespace AppInstaller::Workflow
{
    class WorkflowBase
    {
    protected:
        WorkflowBase(AppInstaller::CLI::Execution::Context& context) :
            m_contextRef(context), m_reporterRef(context.Reporter), m_argsRef(context.Args) {}

        AppInstaller::CLI::Execution::Context& m_contextRef;
        AppInstaller::CLI::Execution::Reporter& m_reporterRef;
        const AppInstaller::CLI::Execution::Args& m_argsRef;

        virtual void OpenIndexSource();

        bool IndexSearch();

        void ReportSearchResult();

        std::shared_ptr<AppInstaller::Repository::ISource> m_source;
        AppInstaller::Repository::SearchResult m_searchResult;
    };

    // A workflow that requires a single manifest to operate properly.
    class SingleManifestWorkflow : public WorkflowBase
    {
    protected:
        using WorkflowBase::WorkflowBase;

        bool EnsureOneMatchFromSearchResult();
        bool GetManifest();
        void SelectInstaller();

        AppInstaller::Manifest::Manifest m_manifest;
        AppInstaller::Manifest::ManifestInstaller m_selectedInstaller;
    };
}