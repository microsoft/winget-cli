// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include <winget/ManifestCommon.h>
#include <ExecutionContext.h>
#include <Workflows/WorkflowBase.h>
#include <Public/winget/RepositorySearch.h>
#include <Public/winget/RepositorySource.h>

#define REQUIRE_TERMINATED_WITH(_context_,_hr_) \
    REQUIRE(_context_.IsTerminated()); \
    REQUIRE(_hr_ == _context_.GetTerminationHR())

namespace TestCommon
{
    using namespace std::string_view_literals;

    // Possible results returned when searching the WorkflowTestSource.
    // If the search query matches with this object or is empty, it adds to the search results.
    struct TestSourceResult
    {
        using AddResultsFunction = std::function<void(std::vector<AppInstaller::Repository::ResultMatch>&, std::weak_ptr<const AppInstaller::Repository::ISource>)>;
        TestSourceResult(std::string_view query, AddResultsFunction addResults) : Query(query), AddResults(addResults) {}

        std::string Query;
        AddResultsFunction AddResults;
    };

    namespace TSR
    {
        const extern TestSourceResult TestQuery_ReturnOne;
        const extern TestSourceResult TestQuery_ReturnTwo;
        const extern TestSourceResult TestInstaller_Exe;
        const extern TestSourceResult TestInstaller_Exe_Dependencies;
        const extern TestSourceResult TestInstaller_Exe_DifferentInstallerType;
        const extern TestSourceResult TestInstaller_Exe_ExpectedReturnCodes;
        const extern TestSourceResult TestInstaller_Exe_IncompatibleInstallerType;
        const extern TestSourceResult TestInstaller_Exe_LatestInstalled;
        const extern TestSourceResult TestInstaller_Exe_LicenseAgreement;
        const extern TestSourceResult TestInstaller_Exe_NothingInstalled;
        const extern TestSourceResult TestInstaller_Exe_UnknownVersion;
        const extern TestSourceResult TestInstaller_Exe_UnsupportedArguments;
        const extern TestSourceResult TestInstaller_Exe_UpgradeAllWithDuplicateUpgradeItems;
        const extern TestSourceResult TestInstaller_Exe_UpgradeUsesAgreements;
        const extern TestSourceResult TestInstaller_Msix;
        const extern TestSourceResult TestInstaller_Msix_UpgradeRequiresExplicit;
        const extern TestSourceResult TestInstaller_Msix_UpgradeUsesAgreements;
        const extern TestSourceResult TestInstaller_Msix_WFDependency;
        const extern TestSourceResult TestInstaller_MSStore;
        const extern TestSourceResult TestInstaller_Zip;
        const extern TestSourceResult TestInstaller_Portable;
    }

    struct WorkflowTestSource : public TestSource
    {
        WorkflowTestSource() {}
        WorkflowTestSource(std::vector<TestSourceResult>&& testSourceResults) : m_testSourceResults(std::move(testSourceResults)) {}

        AppInstaller::Repository::SearchResult Search(const AppInstaller::Repository::SearchRequest& request) const override;

        void AddResult(const TestSourceResult& testSourceResult);

    private:
        std::vector<TestSourceResult> m_testSourceResults;
    };

    std::shared_ptr<WorkflowTestSource> CreateTestSource(std::vector<TestSourceResult>&& testSourceResults);

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

    void OverrideForOpenSource(TestContext& context, std::shared_ptr<WorkflowTestSource> testSource, bool overrideOpenCompositeSource = false);

    void OverrideForCompositeInstalledSource(TestContext& context, std::shared_ptr<WorkflowTestSource> testSource);

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

    void OverrideOpenDependencySource(TestContext& context);

    void OverrideEnableWindowsFeaturesDependencies(TestContext& context);

    void OverrideRegisterStartupAfterReboot(TestContext& context);

    void OverrideDownloadInstallerFileForMSStoreDownload(TestContext& context);
}
