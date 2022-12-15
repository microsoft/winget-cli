// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include "DependenciesTestSource.h"
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerStrings.h>
#include <Workflows/ArchiveFlow.h>
#include <Workflows/DependenciesFlow.h>
#include <Workflows/DownloadFlow.h>
#include <Workflows/ImportExportFlow.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/MsiInstallFlow.h>
#include <Workflows/MSStoreInstallerHandler.h>
#include <Workflows/PortableFlow.h>
#include <Workflows/PromptFlow.h>
#include <Workflows/ShellExecuteInstallerHandler.h>
#include <Workflows/ShowFlow.h>
#include <Workflows/SourceFlow.h>
#include <Workflows/UninstallFlow.h>
#include <Workflows/UpdateFlow.h>
#include <Workflows/WorkflowBase.h>
#include <Public/winget/RepositorySource.h>
#include <PortableInstaller.h>
#include <Commands/ExportCommand.h>
#include <Commands/ImportCommand.h>
#include <Commands/InstallCommand.h>
#include <Commands/ShowCommand.h>
#include <Commands/SettingsCommand.h>
#include <Commands/SearchCommand.h>
#include <Commands/UninstallCommand.h>
#include <Commands/UpgradeCommand.h>
#include <Commands/SourceCommand.h>
#include <winget/LocIndependent.h>
#include <winget/ManifestYamlParser.h>
#include <winget/PathVariable.h>
#include <winget/Archive.h>
#include <Resources.h>
#include <AppInstallerFileLogger.h>
#include <Commands/ValidateCommand.h>
#include <winget/Settings.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Logging;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Settings;
using namespace AppInstaller::CLI::Portable;

#define REQUIRE_TERMINATED_WITH(_context_,_hr_) \
    REQUIRE(_context_.IsTerminated()); \
    REQUIRE(_hr_ == _context_.GetTerminationHR())

struct WorkflowTestSource : public TestSource
{
    SearchResult Search(const SearchRequest& request) const override;
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

    SearchResult Search(const SearchRequest& request) const override;

private:
    TestSourceSearchOptions m_searchOptions;
};

struct TestContext;

struct WorkflowTaskOverride
{
    WorkflowTaskOverride(WorkflowTask::Func f, const std::function<void(TestContext&)>& o, int expectedUseCount = -1) :
        Target(f), Override(o), ExpectedUseCount(expectedUseCount) {}

    WorkflowTaskOverride(std::string_view n, const std::function<void(TestContext&)>& o, int expectedUseCount = -1) :
        Target(n), Override(o), ExpectedUseCount(expectedUseCount) {}

    WorkflowTaskOverride(const WorkflowTask& t, const std::function<void(TestContext&)>& o, int expectedUseCount = -1) :
        Target(t), Override(o), ExpectedUseCount(expectedUseCount) {}

    // -1 means no check on actual use count, as long as it's used.
    int ExpectedUseCount = -1;
    int UseCount = 0;
    WorkflowTask Target;
    std::function<void(TestContext&)> Override;
};

// Enables overriding the behavior of specific workflow tasks.
struct TestContext : public Context
{
    TestContext(std::ostream& out, std::istream& in);

    TestContext(std::ostream& out, std::istream& in, bool isClone, std::shared_ptr<std::vector<WorkflowTaskOverride>> overrides);

    ~TestContext();

    void Override(const WorkflowTaskOverride& wto);

    std::unique_ptr<Context> CreateSubContext() override;

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

void OverrideForShellExecute(TestContext& context, std::vector<Dependency>& installationLog);

void OverrideForPortableInstall(TestContext& context);

void OverrideForPortableInstallFlow(TestContext& context);

void OverridePortableInstaller(TestContext& context);

void OverrideForExtractInstallerFromArchive(TestContext& context);

void OverrideForVerifyAndSetNestedInstaller(TestContext& context);

void OverrideForMSIX(TestContext& context);

void OverrideForMSStore(TestContext& context, bool isUpdate);
