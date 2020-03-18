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
        WorkflowBase(AppInstaller::CLI::ExecutionContext& context) :
            m_contextRef(context), m_reporterRef(context.Reporter), m_argsRef(context.Args) {}

        AppInstaller::CLI::ExecutionContext& m_contextRef;
        AppInstaller::CLI::ExecutionReporter& m_reporterRef;
        const AppInstaller::CLI::ExecutionArgs& m_argsRef;

        virtual void OpenIndexSource();

        bool IndexSearch();

        bool EnsureOneMatchFromSearchResult();

        void ReportSearchResult();

        std::shared_ptr<AppInstaller::Repository::ISource> m_source;
        AppInstaller::Repository::SearchResult m_searchResult;
    };
}