#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include "TestHooks.h"
#include "TestSettings.h"
#include "DependenciesTestSource.h"
#include <AppInstallerErrors.h>
#include <AppInstallerStrings.h>
#include <Workflows/ImportExportFlow.h>
#include <Workflows/DownloadFlow.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/DependenciesFlow.h>
#include <Workflows/WorkflowBase.h>
#include <Public/AppInstallerRepositorySource.h>
#include <Public/AppInstallerRepositorySearch.h>
#include <winget/ManifestYamlParser.h>
#include <Resources.h>

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

namespace
{
    struct TestContext;

    struct WorkflowTaskOverride
    {
        WorkflowTaskOverride(WorkflowTask::Func f, const std::function<void(TestContext&)>& o) :
            Target(f), Override(o) {}

        WorkflowTaskOverride(std::string_view n, const std::function<void(TestContext&)>& o) :
            Target(n), Override(o) {}

        WorkflowTaskOverride(const WorkflowTask& t, const std::function<void(TestContext&)>& o) :
            Target(t), Override(o) {}

        bool Used = false;
        WorkflowTask Target;
        std::function<void(TestContext&)> Override;
    };

    // Enables overriding the behavior of specific workflow tasks.
    struct TestContext : public Context
    {
        TestContext(std::ostream& out, std::istream& in) : TestContext(out, in, false, std::make_shared<std::vector<WorkflowTaskOverride>>())
        {
            WorkflowTaskOverride wto
            { RemoveInstaller, [](TestContext&)
                {
                    // Do nothing; we never want to remove the test files.
            } };

            // Mark this one as used so that it doesn't anger the destructor.
            wto.Used = true;

            Override(wto);
        }

        TestContext(std::ostream& out, std::istream& in, bool isClone, std::shared_ptr<std::vector<WorkflowTaskOverride>> overrides) :
            m_out(out), m_in(in), m_overrides(overrides), m_isClone(isClone), Context(out, in)
        {
            m_shouldExecuteWorkflowTask = [this](const Workflow::WorkflowTask& task)
            {
                auto itr = std::find_if(m_overrides->begin(), m_overrides->end(), [&](const WorkflowTaskOverride& wto) { return wto.Target == task; });

                if (itr == m_overrides->end())
                {
                    return true;
                }
                else
                {
                    itr->Used = true;
                    itr->Override(*this);
                    return false;
                }
            };
        }

        ~TestContext()
        {
            if (!m_isClone)
            {
                for (const auto& wto : *m_overrides)
                {
                    if (!wto.Used)
                    {
                        FAIL_CHECK("Unused override " + wto.Target.GetName());
                    }
                }
            }
        }

        void Override(const WorkflowTaskOverride& wto)
        {
            m_overrides->emplace_back(wto);
        }

        std::unique_ptr<Context> Clone() override
        {
            auto clone = std::make_unique<TestContext>(m_out, m_in, true, m_overrides);
            clone->SetFlags(this->GetFlags());
            return clone;
        }

    private:
        std::shared_ptr<std::vector<WorkflowTaskOverride>> m_overrides;
        std::ostream& m_out;
        std::istream& m_in;
        bool m_isClone = false;
    };
}

void OverrideOpenSourceForDependencies(TestContext& context)
{
    context.Override({ Workflow::OpenDependencySource, [](TestContext& context)
    {
        context.Add<Execution::Data::DependencySource>(std::make_shared<DependenciesTestSource>());
    } });
}

void OverrideForInstallMultiplePacakages(TestContext& context)
{
    context.Override({ Workflow::InstallMultiplePackages(
        Resource::String::InstallAndUpgradeCommandsReportDependencies,
        APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES,
        {},
        false,
        true), [](TestContext& )
    {
        
    } });
}

TEST_CASE("DependencyGraph_BFirst", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    std::vector<Dependency> installationOrder;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    Manifest manifest = CreateFakeManifestWithDependencies("NeedsToInstallBFirst");
    OverrideOpenSourceForDependencies(context);
    OverrideForInstallMultiplePacakages(context);

    context.Add<Execution::Data::DependencySource>(std::make_shared<DependenciesTestSource>());
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    context << ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies);

    std::vector<Execution::PackageToInstall> installers = context.Get<Execution::Data::PackagesToInstall>();

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);

    // Verify installers are called in order
    REQUIRE(installers.size() == 2);
    REQUIRE(installers.at(0).Manifest.Id == "B");
    REQUIRE(installers.at(1).Manifest.Id == "C");
}

TEST_CASE("DependencyGraph_SkipInstalled", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    Manifest manifest = CreateFakeManifestWithDependencies("DependenciesInstalled");
    OverrideOpenSourceForDependencies(context);
    OverrideForInstallMultiplePacakages(context);

    context.Add<Execution::Data::DependencySource>(std::make_shared<DependenciesTestSource>());
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    context << ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies);

    std::vector<Execution::PackageToInstall> installers = context.Get<Execution::Data::PackagesToInstall>();
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);
    REQUIRE(installers.size() == 0);
}

TEST_CASE("DependencyGraph_validMinVersions", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    Manifest manifest = CreateFakeManifestWithDependencies("DependenciesValidMinVersions");
    OverrideOpenSourceForDependencies(context);
    OverrideForInstallMultiplePacakages(context);

    context.Add<Execution::Data::DependencySource>(std::make_shared<DependenciesTestSource>());
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    context << ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies);

    std::vector<Execution::PackageToInstall> installers = context.Get<Execution::Data::PackagesToInstall>();

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);
    REQUIRE(installers.size() == 1);
    REQUIRE(installers.at(0).Manifest.Id == "minVersion");
    // minVersion 1.5 is available but this requires 1.0 so that version is installed
    REQUIRE(installers.at(0).Manifest.Version == "1.0");
}

TEST_CASE("DependencyGraph_PathNoLoop", "[InstallFlow][workflow][dependencyGraph][dependencies]", )
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    Manifest manifest = CreateFakeManifestWithDependencies("PathBetweenBranchesButNoLoop");
    OverrideOpenSourceForDependencies(context);
    OverrideForInstallMultiplePacakages(context);

    context.Add<Execution::Data::DependencySource>(std::make_shared<DependenciesTestSource>());
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    context << ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies);

    std::vector<Execution::PackageToInstall> installers = context.Get<Execution::Data::PackagesToInstall>();

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);

    // Verify installers are called in order
    REQUIRE(installers.size() == 4);
    REQUIRE(installers.at(0).Manifest.Id == "B");
    REQUIRE(installers.at(1).Manifest.Id == "C");
    REQUIRE(installers.at(2).Manifest.Id == "G");
    REQUIRE(installers.at(3).Manifest.Id == "H");
}

TEST_CASE("DependencyGraph_InStackNoLoop", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    Manifest manifest = CreateFakeManifestWithDependencies("DependencyAlreadyInStackButNoLoop");
    OverrideOpenSourceForDependencies(context);
    OverrideForInstallMultiplePacakages(context);

    context.Add<Execution::Data::DependencySource>(std::make_shared<DependenciesTestSource>());
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    context << ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies);

    std::vector<Execution::PackageToInstall> installers = context.Get<Execution::Data::PackagesToInstall>();

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);

    // Verify installers are called in order
    REQUIRE(installers.size() == 3);
    REQUIRE(installers.at(0).Manifest.Id == "B");
    REQUIRE(installers.at(1).Manifest.Id == "C");
    REQUIRE(installers.at(2).Manifest.Id == "F");
}