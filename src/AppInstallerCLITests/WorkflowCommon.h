// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include <winget/ManifestCommon.h>
#include <ExecutionContext.h>
#include <Workflows/WorkflowBase.h>
#include <Public/winget/RepositorySource.h>

//#include "TestHooks.h"
//#include "TestSettings.h"
//#include "DependenciesTestSource.h"
//#include <AppInstallerDownloader.h>
//#include <AppInstallerErrors.h>
//#include <AppInstallerFileLogger.h>
//#include <AppInstallerLogging.h>
//#include <AppInstallerStrings.h>
//#include <winget/Archive.h>
//#include <winget/LocIndependent.h>
//#include <winget/PathVariable.h>
//#include <winget/Settings.h>
//#include <Workflows/ArchiveFlow.h>
//#include <Workflows/DependenciesFlow.h>
//#include <Workflows/DownloadFlow.h>
//#include <Workflows/ImportExportFlow.h>
//#include <Workflows/InstallFlow.h>
//#include <Workflows/MsiInstallFlow.h>
//#include <Workflows/MSStoreInstallerHandler.h>
//#include <Workflows/PortableFlow.h>
//#include <Workflows/PromptFlow.h>
//#include <Workflows/ShellExecuteInstallerHandler.h>
//#include <Workflows/ShowFlow.h>
//#include <Workflows/SourceFlow.h>
//#include <Workflows/UninstallFlow.h>
//#include <Workflows/UpdateFlow.h>
//#include <Commands/ExportCommand.h>
//#include <Commands/ImportCommand.h>
//#include <Commands/InstallCommand.h>
//#include <Commands/SearchCommand.h>
//#include <Commands/SettingsCommand.h>
//#include <Commands/ShowCommand.h>
//#include <Commands/SourceCommand.h>
//#include <Commands/UninstallCommand.h>
//#include <Commands/UpgradeCommand.h>
//#include <Commands/ValidateCommand.h>
//#include <PortableInstaller.h>
//#include <Resources.h>

#define REQUIRE_TERMINATED_WITH(_context_,_hr_) \
    REQUIRE(_context_.IsTerminated()); \
    REQUIRE(_hr_ == _context_.GetTerminationHR())

namespace TestCommon
{
    struct WorkflowTestSource : public TestSource
    {
        AppInstaller::Repository::SearchResult Search(const AppInstaller::Repository::SearchRequest& request) const override;
    };

    enum TestSourceSearchOptions
    {
        None = 0,
        UpgradeUsesAgreements,
        UpgradeRequiresExplicit,
    };

    struct WorkflowTestCompositeSource : public TestSource
    {
        WorkflowTestCompositeSource(TestSourceSearchOptions searchOptions = TestSourceSearchOptions::None) : m_searchOptions(searchOptions) {}

        AppInstaller::Repository::SearchResult Search(const AppInstaller::Repository::SearchRequest& request) const override;

    private:
        TestSourceSearchOptions m_searchOptions;
    };

    struct TestContext;

    struct WorkflowTaskOverride
    {
        WorkflowTaskOverride(AppInstaller::CLI::Workflow::WorkflowTask::Func f, const std::function<void(TestContext&)>& o, int expectedUseCount = -1) :
            Target(f), Override(o), ExpectedUseCount(expectedUseCount) {}

        WorkflowTaskOverride(std::string_view n, const std::function<void(TestContext&)>& o, int expectedUseCount = -1) :
            Target(n), Override(o), ExpectedUseCount(expectedUseCount) {}

        WorkflowTaskOverride(const AppInstaller::CLI::Workflow::WorkflowTask& t, const std::function<void(TestContext&)>& o, int expectedUseCount = -1) :
            Target(t), Override(o), ExpectedUseCount(expectedUseCount) {}

        // -1 means no check on actual use count, as long as it's used.
        int ExpectedUseCount = -1;
        int UseCount = 0;
        AppInstaller::CLI::Workflow::WorkflowTask Target;
        std::function<void(TestContext&)> Override;
    };

    // Enables overriding the behavior of specific workflow tasks.
    struct TestContext : public AppInstaller::CLI::Execution::Context
    {
        TestContext(std::ostream& out, std::istream& in);

        TestContext(std::ostream& out, std::istream& in, bool isClone, std::shared_ptr<std::vector<WorkflowTaskOverride>> overrides);

        ~TestContext();

        void Override(const WorkflowTaskOverride& wto);

        std::unique_ptr<AppInstaller::CLI::Execution::Context> CreateSubContext() override;

    private:
        std::shared_ptr<std::vector<WorkflowTaskOverride>> m_overrides;
        std::ostream& m_out;
        std::istream& m_in;
        bool m_isClone = false;
    };

    void OverrideForOpenSource(TestContext& context, bool overrideOpenCompositeSource = false);

    void OverrideForCompositeInstalledSource(TestContext& context, TestSourceSearchOptions searchOptions = TestSourceSearchOptions::None);

    void OverrideForUpdateInstallerMotw(TestContext& context);

    void OverrideForCheckExistingInstaller(TestContext& context);

    void OverrideForShellExecute(TestContext& context, int expectedUseCount = -1);

    void OverrideForShellExecute(TestContext& context, std::vector<AppInstaller::Manifest::Dependency>& installationLog);

    void OverrideForPortableInstall(TestContext& context);

    void OverrideForPortableInstallFlow(TestContext& context);

    void OverridePortableInstaller(TestContext& context);

    void OverrideForExtractInstallerFromArchive(TestContext& context);

    void OverrideForVerifyAndSetNestedInstaller(TestContext& context);

    void OverrideForMSIX(TestContext& context);

    void OverrideForMSStore(TestContext& context, bool isUpdate);

}