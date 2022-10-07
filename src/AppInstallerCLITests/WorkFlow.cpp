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

namespace
{
    struct WorkflowTestSource : public TestSource
    {
        SearchResult Search(const SearchRequest& request) const override
        {
            SearchResult result;

            std::string input;

            if (request.Query)
            {
                input = request.Query->Value;
            }
            else if (!request.Inclusions.empty())
            {
                input = request.Inclusions[0].Value;
            }

            if (input == "TestQueryReturnOne")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(std::vector<Manifest>{ manifest }, shared_from_this()),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnOne")));
            }
            else if (input == "TestQueryReturnTwo")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(std::vector<Manifest>{ manifest }, shared_from_this()),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));

                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("Manifest-Good.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(std::vector<Manifest>{ manifest2 }, shared_from_this()),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));
            }

            return result;
        }
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

        SearchResult Search(const SearchRequest& request) const override
        {
            SearchResult result;

            std::string input;

            if (request.Query)
            {
                input = request.Query->Value;
            }
            else if (!request.Inclusions.empty())
            {
                input = request.Inclusions[0].Value;
            }
            else if (!request.Filters.empty())
            {
                input = request.Filters[0].Value;
            }

            // Empty query should return all exe, msix and msstore installer
            if (input.empty() || CaseInsensitiveEquals(input, "AppInstallerCliTest.TestExeInstaller"))
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                Manifest manifest3;
                switch (m_searchOptions)
                {
                case TestSourceSearchOptions::UpgradeUsesAgreements:
                    manifest3 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2_LicenseAgreement.yaml"));
                    break;
                default:
                    manifest3 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2.yaml"));
                    break;
                }

                auto testPackage =
                    TestPackage::Make(
                        manifest,
                        TestPackage::MetadataMap
                        {
                            { PackageVersionMetadata::InstalledType, "Exe" },
                            { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                            { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                        },
                        std::vector<Manifest>{ manifest3, manifest2, manifest },
                        shared_from_this()
                    );
                testPackage->IsSameOverride = [](const IPackage*, const IPackage*) { return true; };
                result.Matches.emplace_back(
                    ResultMatch(
                        testPackage,
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            if (input.empty() || input == "AppInstallerCliTest.TestPortableInstaller")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Portable.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Portable.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Portable" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestPortableInstaller")));
            }

            if (input.empty() || input == "AppInstallerCliTest.TestMsixInstaller")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Msix_StreamingFlow.yaml"));
                Manifest manifest2;
                switch (m_searchOptions)
                {
                case TestSourceSearchOptions::UpgradeUsesAgreements:
                    manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Msix_LicenseAgreement.yaml"));
                    break;
                case TestSourceSearchOptions::None:
                default:
                    manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Msix.yaml"));
                    break;
                }

                TestPackage::MetadataMap packageMetadata
                {
                    { PackageVersionMetadata::InstalledType, "Msix" },
                };

                if (m_searchOptions == TestSourceSearchOptions::UpgradeRequiresExplicit)
                {
                    packageMetadata[PackageVersionMetadata::PinnedState] = "PinnedByManifest";
                }

                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            packageMetadata,
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMsixInstaller")));
            }

            if (input.empty() || input == "AppInstallerCliTest.TestZipInstaller")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Zip_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Zip_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestZipInstaller")));
            }

            if (input.empty() || input == "AppInstallerCliTest.TestMSStoreInstaller")
            {
                auto installed = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_MSStore.yaml"));
                auto available = installed;
                // Override the installed version to not be Latest
                installed.Version = "1.0.0.0";
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            installed,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "MSStore" } },
                            std::vector<Manifest>{ available },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMSStoreInstaller")));
            }

            if (input == "TestExeInstallerWithLatestInstalled")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest2,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            if (input == "TestExeInstallerWithIncompatibleInstallerType")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Msix" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            if (input == "TestExeInstallerWithDifferentInstalledType")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_ARPInstallerType.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Msix" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            if (input == "TestExeInstallerWithUnsupportedArguments")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_UnsupportedArgs.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            if (input == "TestExeInstallerWithNothingInstalled")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            std::vector<Manifest>{ manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            if (input == "AppInstallerCliTest.TestExeInstaller.Dependencies")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("Installer_Exe_Dependencies.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_ExeDependencies.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap
                            {
                                { PackageVersionMetadata::InstalledType, "Exe" },
                                { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                                { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                            },
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                            ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller.Dependencies")));
            }

            if (input == "AppInstallerCliTest.TestMsixInstaller.WFDep")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("Installer_Msix_WFDependency.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            std::vector<Manifest>{ manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMsixInstaller.WFDep")));
            }

            if (input == "TestInstallerWithLicenseAgreement")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_LicenseAgreement.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2_LicenseAgreement.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestInstallerWithLicenseAgreement")));
            }

            if (input == "TestUpgradeAllWithDuplicateUpgradeItems")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                auto manifest3 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap
                            {
                                { PackageVersionMetadata::InstalledType, "Exe" },
                                { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                                { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                            },
                            std::vector<Manifest>{ manifest3, manifest2, manifest },
                            shared_from_this()
                            ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest2,
                            TestPackage::MetadataMap
                            {
                                { PackageVersionMetadata::InstalledType, "Exe" },
                                { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                                { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                            },
                            std::vector<Manifest>{ manifest3, manifest2, manifest },
                            shared_from_this()
                            ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            return result;
        }

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
        TestContext(std::ostream& out, std::istream& in) : TestContext(out, in, false, std::make_shared<std::vector<WorkflowTaskOverride>>())
        {
            WorkflowTaskOverride wto
            { RemoveInstaller, [](TestContext&)
                {
                    // Do nothing; we never want to remove the test files.
            } };

            // Mark this one as used so that it doesn't anger the destructor.
            wto.UseCount++;

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
                    itr->UseCount++;
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
                    if (wto.UseCount == 0)
                    {
                        FAIL_CHECK("Unused override " + wto.Target.GetName());
                    }
                    else if (wto.ExpectedUseCount != -1 && wto.ExpectedUseCount != wto.UseCount)
                    {
                        FAIL_CHECK("Used override count does not match expected " + wto.Target.GetName());
                    }
                }
            }
        }

        void Override(const WorkflowTaskOverride& wto)
        {
            m_overrides->emplace_back(wto);
        }

        std::unique_ptr<Context> CreateSubContext() override
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

void OverrideForOpenSource(TestContext& context)
{
    context.Override({ "OpenSource", [](TestContext& context)
    {
        context.Add<Execution::Data::Source>(Source{ std::make_shared<WorkflowTestSource>() });
    } });
}

void OverrideForCompositeInstalledSource(TestContext& context, TestSourceSearchOptions searchOptions = TestSourceSearchOptions::None)
{
    context.Override({ "OpenSource", [](TestContext&)
    {
    } });

    context.Override({ "OpenCompositeSource", [=](TestContext& context)
    {
        context.Add<Execution::Data::Source>(Source{ std::make_shared<WorkflowTestCompositeSource>(searchOptions) });
    } });
}

void OverrideForImportSource(TestContext& context, bool useTestCompositeSource = false)
{
    context.Override({ "OpenPredefinedSource", [=](TestContext& context)
    {
        auto installedSource = useTestCompositeSource ? std::make_shared<WorkflowTestCompositeSource>() : std::make_shared<TestSource>();
        context.Add<Execution::Data::Source>(Source{ installedSource });
    } });

    context.Override({ Workflow::OpenSourcesForImport, [](TestContext& context)
    {
        context.Add<Execution::Data::Sources>(std::vector<Source>{ Source{ std::make_shared<WorkflowTestCompositeSource>() } });
    } });
}

void OverrideOpenSourceForDependencies(TestContext& context)
{
    context.Override({ "OpenSource", [](TestContext& context)
    {
        context.Add<Execution::Data::Source>(Source{ std::make_shared<DependenciesTestSource>() });
    } });

    context.Override({ Workflow::OpenDependencySource, [](TestContext& context)
    {
        context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    } });
}

void OverrideDependencySource(TestContext& context)
{
    context.Override({ Workflow::OpenDependencySource, [](TestContext& context)
    {
        context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    } });
}

void OverrideForUpdateInstallerMotw(TestContext& context)
{
    context.Override({ UpdateInstallerFileMotwIfApplicable, [](TestContext&)
    {
    } });
}

void OverrideForCheckExistingInstaller(TestContext& context)
{
    context.Override({ CheckForExistingInstaller, [](TestContext&)
    {
    } });
}

void OverrideForShellExecute(TestContext& context, int expectedUseCount = -1)
{
    OverrideForCheckExistingInstaller(context);

    context.Override({ DownloadInstallerFile, [](TestContext& context)
    {
        context.Add<Data::HashPair>({ {}, {} });
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
    }, expectedUseCount });

    context.Override({ RenameDownloadedInstaller, [](TestContext&)
    {
    }, expectedUseCount });

    OverrideForUpdateInstallerMotw(context);
}

void OverrideForShellExecute(TestContext& context, std::vector<Dependency>& installationLog)
{
    context.Override({ DownloadInstallerFile, [&installationLog](TestContext& context)
    {
        context.Add<Data::HashPair>({ {}, {} });
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));

        auto dependency = Dependency(DependencyType::Package, context.Get<Execution::Data::Manifest>().Id, context.Get<Execution::Data::Manifest>().Version);
        installationLog.push_back(dependency);
    }});

    context.Override({ RenameDownloadedInstaller, [](TestContext&)
    {
    } });

    OverrideForUpdateInstallerMotw(context);
}

void OverrideForPortableInstall(TestContext& context)
{
    context.Override({ PortableInstall, [](TestContext&)
    {
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestPortableInstalled.txt";
        std::ofstream file(temp, std::ofstream::out);
        file.close();
    } });
}

void OverrideForPortableInstallFlow(TestContext& context)
{
    context.Override({ DownloadInstallerFile, [](TestContext& context)
    {
        context.Add<Data::HashPair>({ {}, {} });
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
    } });

    context.Override({ RenameDownloadedInstaller, [](TestContext&)
    {
    } });

    OverrideForUpdateInstallerMotw(context);
    OverrideForPortableInstall(context);
}

void OverridePortableInstaller(TestContext& context)
{
    context.Override({ DownloadInstallerFile, [](TestContext& context)
    {
        std::filesystem::path tempDirectory = std::filesystem::temp_directory_path();
        const auto& installerPath = TestDataFile("AppInstallerTestExeInstaller.exe").GetPath();
        const auto& tempInstallerPath = tempDirectory / "AppInstallerTestExeInstaller.exe";
        std::filesystem::copy(installerPath, tempInstallerPath, std::filesystem::copy_options::overwrite_existing);
        context.Add<Data::InstallerPath>(tempInstallerPath);

        std::ifstream inStream{ tempInstallerPath, std::ifstream::binary };
        SHA256::HashBuffer fileHash = SHA256::ComputeHash(inStream);
        context.Add<Data::HashPair>({ fileHash, fileHash });
    } });

    context.Override({ RenameDownloadedInstaller, [](TestContext&)
    {
    } });

    OverrideForUpdateInstallerMotw(context);
}

void OverrideForPortableUninstall(TestContext& context)
{
    context.Override({ PortableUninstallImpl, [](TestContext& context)
    {
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestPortableUninstalled.txt";
        std::ofstream file(temp, std::ofstream::out);
        file.close();

        context.Add<Execution::Data::OperationReturnCode>(0);
    } });
}

void OverrideForArchiveInstall(TestContext& context)
{
    context.Override({ ExtractFilesFromArchive, [](TestContext&)
    {
    } });

    context.Override({ VerifyAndSetNestedInstaller, [](TestContext&)
    {
    } });
}

void OverrideForExtractInstallerFromArchive(TestContext& context)
{
    context.Override({ ExtractFilesFromArchive, [](TestContext&)
    {
    } });
}

void OverrideForVerifyAndSetNestedInstaller(TestContext& context)
{
    context.Override({ VerifyAndSetNestedInstaller, [](TestContext&)
    {
    } });
}

void OverrideForDirectMsi(TestContext& context)
{
    OverrideForCheckExistingInstaller(context);

    context.Override({ DownloadInstallerFile, [](TestContext& context)
    {
        context.Add<Data::HashPair>({ {}, {} });
        // We don't have an msi installer for tests, but we won't execute it anyway
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
    } });
 
    context.Override({ RenameDownloadedInstaller, [](TestContext&)
    {
    } });

    OverrideForUpdateInstallerMotw(context);

    context.Override({ DirectMSIInstallImpl, [](TestContext& context)
    {
        // Write out the install command
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestMsiInstalled.txt";
        std::ofstream file(temp, std::ofstream::out);
        file << context.Get<Execution::Data::InstallerArgs>();
        file.close();

        context.Add<Execution::Data::OperationReturnCode>(0);
    } });
}

void OverrideForExeUninstall(TestContext& context)
{
    context.Override({ ShellExecuteUninstallImpl, [](TestContext& context)
    {
        // Write out the uninstall command
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestExeUninstalled.txt";
        std::ofstream file(temp, std::ofstream::out);
        file << context.Get<Execution::Data::UninstallString>();
        file.close();

        context.Add<Execution::Data::OperationReturnCode>(0);
    } });
}

void OverrideForMSIX(TestContext& context)
{
    context.Override({ MsixInstall, [](TestContext& context)
    {
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestMsixInstalled.txt";
        std::ofstream file(temp, std::ofstream::out);

        if (context.Contains(Execution::Data::InstallerPath))
        {
            file << context.Get<Execution::Data::InstallerPath>().u8string();
        }
        else
        {
            file << context.Get<Execution::Data::Installer>()->Url;
        }

        file.close();
    } });
}

void OverrideForMSIXUninstall(TestContext& context)
{
    context.Override({ MsixUninstall, [](TestContext& context)
    {
        // Write out the package full name
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestMsixUninstalled.txt";
        std::ofstream file(temp, std::ofstream::out);
        for (const auto& packageFamilyName : context.Get<Execution::Data::PackageFamilyNames>())
        {
            file << packageFamilyName << std::endl;
        }

        file.close();
    } });
}

void OverrideForMSStore(TestContext& context, bool isUpdate)
{
    if (isUpdate)
    {
        context.Override({ MSStoreUpdate, [](TestContext& context)
        {
            std::filesystem::path temp = std::filesystem::temp_directory_path();
            temp /= "TestMSStoreUpdated.txt";
            std::ofstream file(temp, std::ofstream::out);
            file << context.Get<Execution::Data::Installer>()->ProductId;
            file.close();
        } });
    }
    else
    {
        context.Override({ MSStoreInstall, [](TestContext& context)
        {
            std::filesystem::path temp = std::filesystem::temp_directory_path();
            temp /= "TestMSStoreInstalled.txt";
            std::ofstream file(temp, std::ofstream::out);
            file << context.Get<Execution::Data::Installer>()->ProductId;
            file.close();
        } });
    }

    context.Override({ Workflow::EnsureStorePolicySatisfied, [](TestContext&)
    {
    } });
}

void OverrideForSourceAddWithAgreements(TestContext& context, bool isAddExpected = true)
{
    context.Override({ EnsureRunningAsAdmin, [](TestContext&)
    {
    } });

    if (isAddExpected)
    {
        context.Override({ AddSource, [](TestContext&)
        {
        } });
    }

    context.Override({ CreateSourceForSourceAdd, [](TestContext& context)
    {
        auto testSource = std::make_shared<TestSource>();
        testSource->Information.SourceAgreementsIdentifier = "AgreementsIdentifier";
        testSource->Information.SourceAgreements.emplace_back("Agreement Label", "Agreement Text", "https://test");
        testSource->Information.RequiredPackageMatchFields.emplace_back("Market");
        testSource->Information.RequiredQueryParameters.emplace_back("Market");
        context << Workflow::HandleSourceAgreements(Source{ testSource });
    } });
}

void OverrideOpenDependencySource(TestContext& context)
{
    context.Override({ Workflow::OpenDependencySource, [](TestContext& context)
    {
        context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    } });
}

void OverrideForInstallMultiplePackages(TestContext& context)
{
    context.Override({ Workflow::InstallMultiplePackages(
        Resource::String::InstallAndUpgradeCommandsReportDependencies,
        APPINSTALLER_CLI_ERROR_INSTALL_DEPENDENCIES,
        {},
        false,
        true), [](TestContext&)
    {

    } });
}

TEST_CASE("ExeInstallFlowWithTestManifest", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_RenameFromEncodedUrl", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCheckExistingInstaller(context);
    context.Override({ DownloadInstallerFile, [](TestContext& context)
    {
        context.Add<Data::HashPair>({ {}, {} });
        auto installerPath = std::filesystem::temp_directory_path();
        installerPath /= "EncodedUrlTest.exe";
        std::filesystem::copy(TestDataFile("AppInstallerTestExeInstaller.exe"), installerPath, std::filesystem::copy_options::overwrite_existing);
        context.Add<Data::InstallerPath>(installerPath);
    } });
    OverrideForUpdateInstallerMotw(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_EncodedUrl.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/encodedUrl") != std::string::npos);
}

TEST_CASE("InstallFlowNonZeroExitCode", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_NonZeroExitCode.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/ExitCode 0x80070005") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_InstallationNotes", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InstallationNotes.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify installation notes are displayed
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find("testInstallationNotes") != std::string::npos);
}

TEST_CASE("InstallFlow_UnsupportedArguments_Warn", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_UnsupportedArguments.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Log, tempDirectory);

    InstallCommand install({});
    context.SetExecutingCommand(&install);
    install.Execute(context);
    INFO(installOutput.str());

    // Verify unsupported arguments warn message is shown
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::UnsupportedArgument).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("-o,--log") != std::string::npos);
}

TEST_CASE("InstallFlow_UnsupportedArguments_Error", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_UnsupportedArguments.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);

    InstallCommand install({});
    context.SetExecutingCommand(&install);
    install.Execute(context);
    INFO(installOutput.str());

    // Verify unsupported arguments error message is shown 
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT);
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::UnsupportedArgument).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("-l,--location") != std::string::npos);
}

TEST_CASE("InstallFlow_UnsupportedArguments_NotProvided")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_UnsupportedArguments.yaml").GetPath().u8string());

    InstallCommand install({});
    context.SetExecutingCommand(&install);
    install.Execute(context);
    INFO(installOutput.str());

    // Verify unsupported arguments error message is not shown when not provided
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::UnsupportedArgument).get()) == std::string::npos);
    REQUIRE(installOutput.str().find("-o,--log") == std::string::npos);
    REQUIRE(installOutput.str().find("-l,--location") == std::string::npos);
}

TEST_CASE("InstallFlow_ExpectedReturnCodes", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_ExpectedReturnCodes.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Override, "/ExitCode 8"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify install failed with the right message
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INSTALL_CONTACT_SUPPORT);
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallFlowReturnCodeContactSupport).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("https://TestReturnResponseUrl") != std::string::npos);
}

TEST_CASE("InstallFlowWithNonApplicableArchitecture", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_NoApplicableArchitecture.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("InstallFlow_Zip_Exe", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFZipInstall>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideForExtractInstallerFromArchive(context);
    OverrideForVerifyAndSetNestedInstaller(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    bool overrideArchiveScanResult = true;
    AppInstaller::Archive::TestHook_SetScanArchiveResult_Override(&overrideArchiveScanResult);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_BadRelativePath", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFZipInstall>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideForExtractInstallerFromArchive(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    bool overrideArchiveScanResult = true;
    AppInstaller::Archive::TestHook_SetScanArchiveResult_Override(&overrideArchiveScanResult);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NESTEDINSTALLER_NOT_FOUND);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::NestedInstallerNotFound).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_MissingNestedInstaller", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFZipInstall>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_MissingNestedInstaller.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::NestedInstallerNotSpecified).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_UnsupportedNestedInstaller", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFZipInstall>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_UnsupportedNestedInstaller.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, ERROR_NOT_SUPPORTED);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::NestedInstallerNotSupported).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_MultipleNonPortableNestedInstallers", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFZipInstall>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_MultipleNonPortableNestedInstallers.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INVALID_MANIFEST);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::MultipleNonPortableNestedInstallersSpecified).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_ArchiveScanFailed", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFZipInstall>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    bool overrideArchiveScanResult = false;
    AppInstaller::Archive::TestHook_SetScanArchiveResult_Override(&overrideArchiveScanResult);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_ARCHIVE_SCAN_FAILED);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ArchiveFailedMalwareScan).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_Zip_ArchiveScanOverride", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFZipInstall>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideForExtractInstallerFromArchive(context);
    OverrideForVerifyAndSetNestedInstaller(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::HashOverride);

    bool overrideArchiveScanResult = false;
    AppInstaller::Archive::TestHook_SetScanArchiveResult_Override(&overrideArchiveScanResult);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify override message is displayed to the user.
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ArchiveFailedMalwareScanOverridden).get()) != std::string::npos);

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("ExtractInstallerFromArchive_InvalidZip", "[InstallFlow][workflow]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Zip_Exe.yaml"));
    context.Add<Data::Manifest>(manifest);
    context.Add<Data::Installer>(manifest.Installers.at(0));
    // Provide an invalid zip file which should be handled appropriately.
    context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
    context << ExtractFilesFromArchive;
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_EXTRACT_ARCHIVE_FAILED);
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::ExtractArchiveFailed).get()) != std::string::npos);
}

TEST_CASE("MSStoreInstallFlowWithTestManifest", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMSStoreInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForMSStore(context, false);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_MSStore.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("9WZDNCRFJ364") != std::string::npos);
}

TEST_CASE("MsixInstallFlow_DownloadFlow", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForMSIX(context);
    OverrideForUpdateInstallerMotw(context);
    // Todo: point to files from our repo when the repo goes public
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Msix_DownloadFlow.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and a local file is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    Uri uri = Uri(ConvertToUTF16(installResultStr));
    REQUIRE(uri.SchemeName() == L"file");
}

TEST_CASE("MsixInstallFlow_StreamingFlow", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForMSIX(context);
    OverrideForCheckExistingInstaller(context);
    // Todo: point to files from our repo when the repo goes public
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Msix_StreamingFlow.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and a http address is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    Uri uri = Uri(ConvertToUTF16(installResultStr));
    REQUIRE(uri.SchemeName() == L"https");
}

TEST_CASE("MsiInstallFlow_DirectMsi", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMsiInstalled.txt");

    TestCommon::TestUserSettings testSettings;
    testSettings.Set<Setting::EFDirectMSI>(true);

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForDirectMsi(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallerArgTest_Msi_NoSwitches.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Silent);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/quiet") != std::string::npos);
}

TEST_CASE("InstallFlow_Portable", "[InstallFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestPortableInstallRoot", false);
    TestCommon::TempFile portableInstallResultPath("TestPortableInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Portable.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(std::filesystem::exists(portableInstallResultPath.GetPath()));
}

TEST_CASE("InstallFlow_Portable_SymlinkCreationFail", "[InstallFlow][workflow]")
{
    std::ostringstream installOutput;
    TestContext installContext{ installOutput, std::cin };
    auto PreviousThreadGlobals = installContext.SetForCurrentThread();
    OverridePortableInstaller(installContext);
    bool overrideCreateSymlinkStatus = false;
    AppInstaller::Filesystem::TestHook_SetCreateSymlinkResult_Override(&overrideCreateSymlinkStatus);
    installContext.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Portable.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(installContext);
    INFO(installOutput.str());

    // 'DefaultSource' is expected because we are installing from a local manifest.
    const auto& portableUserRoot = AppInstaller::Runtime::GetPathTo(AppInstaller::Runtime::PathName::PortablePackageUserRoot);
    const auto& portableTargetDirectory = portableUserRoot / "AppInstallerCliTest.TestPortableInstaller__DefaultSource";
    const auto& portableTargetPath = portableTargetDirectory / "AppInstallerTestExeInstaller.exe";
    REQUIRE(std::filesystem::exists(portableTargetPath));
    REQUIRE(AppInstaller::Registry::Environment::PathVariable(AppInstaller::Manifest::ScopeEnum::User).Contains(portableTargetDirectory));
}

TEST_CASE("PortableInstallFlow_UserScope", "[InstallFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestPortableInstallRoot", false);
    TestCommon::TempFile portableInstallResultPath("TestPortableInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Portable.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::InstallScope, "user"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(std::filesystem::exists(portableInstallResultPath.GetPath()));
}

TEST_CASE("PortableInstallFlow_MachineScope", "[InstallFlow][workflow]")
{
    if (!AppInstaller::Runtime::IsRunningAsAdmin())
    {
        WARN("Test requires admin privilege. Skipped.");
        return;
    }

    TestCommon::TempDirectory tempDirectory("TestPortableInstallRoot", false);
    TestCommon::TempFile portableInstallResultPath("TestPortableInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Portable.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::InstallScope, "machine"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());
    REQUIRE(std::filesystem::exists(portableInstallResultPath.GetPath()));
}

TEST_CASE("ShellExecuteHandlerInstallerArgs", "[InstallFlow][workflow]")
{
    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Default Msi type with no args passed in, no switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Msi_NoSwitches.yaml"));
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/passive") != std::string::npos);
        REQUIRE(installerArgs.find(FileLogger::DefaultPrefix()) != std::string::npos);
        REQUIRE(installerArgs.find(manifest.Id) != std::string::npos);
        REQUIRE(installerArgs.find(manifest.Version) != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Msi type with /silent and /log and /custom and /installlocation, no switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Msi_NoSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/quiet") != std::string::npos);
        REQUIRE(installerArgs.find("/log \"MyLog.log\"") != std::string::npos);
        REQUIRE(installerArgs.find("TARGETDIR=\"MyDir\"") != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Msi type with /silent and /log and /custom and /installlocation, switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Msi_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mycustom") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Default Inno type with no args passed in, no switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_NoSwitches.yaml"));
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/SILENT") != std::string::npos);
        REQUIRE(installerArgs.find(FileLogger::DefaultPrefix()) != std::string::npos);
        REQUIRE(installerArgs.find(manifest.Id) != std::string::npos);
        REQUIRE(installerArgs.find(manifest.Version) != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Inno type with /silent and /log and /custom and /installlocation, no switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_NoSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/VERYSILENT") != std::string::npos);
        REQUIRE(installerArgs.find("/LOG=\"MyLog.log\"") != std::string::npos);
        REQUIRE(installerArgs.find("/DIR=\"MyDir\"") != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Inno type with /silent and /log and /custom and /installlocation, switches specified in manifest
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mycustom") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
    }

    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        // Override switch specified. The whole arg passed to installer is overridden.
        auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log"sv);
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir"sv);
        context.Args.AddArg(Execution::Args::Type::Override, "/OverrideEverything"sv);
        context.Add<Data::Manifest>(manifest);
        context.Add<Data::Installer>(manifest.Installers.at(0));
        context << GetInstallerArgs;
        std::string installerArgs = context.Get<Data::InstallerArgs>();
        REQUIRE(installerArgs == "/OverrideEverything"); // Use value specified in override switch
    }
}

TEST_CASE("InstallFlow_SearchAndInstall", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnOne"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/custom") != std::string::npos);
    REQUIRE(installResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("InstallFlow_SearchFoundNoApp", "[InstallFlow][workflow]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnZero"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify proper message is printed
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::NoPackageFound).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_SearchFoundMultipleApp", "[InstallFlow][workflow]")
{
    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnTwo"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify proper message is printed
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::MultiplePackagesFound).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_LicenseAgreement", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_LicenseAgreement.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify agreements are shown
    REQUIRE(installOutput.str().find("Agreement with text") != std::string::npos);
    REQUIRE(installOutput.str().find("This is the text of the agreement.") != std::string::npos);
    REQUIRE(installOutput.str().find("Agreement with URL") != std::string::npos);
    REQUIRE(installOutput.str().find("https://TestAgreementUrl") != std::string::npos);

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("InstallFlow_LicenseAgreement_Prompt", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    // Accept the agreements by saying "Yes" at the prompt
    std::istringstream installInput{ "y" };

    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_LicenseAgreement.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt was shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsPrompt).get()) != std::string::npos);

    // Verify agreements are shown
    REQUIRE(installOutput.str().find("Agreement with text") != std::string::npos);
    REQUIRE(installOutput.str().find("This is the text of the agreement.") != std::string::npos);
    REQUIRE(installOutput.str().find("Agreement with URL") != std::string::npos);
    REQUIRE(installOutput.str().find("https://TestAgreementUrl") != std::string::npos);

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("InstallFlow_LicenseAgreement_NotAccepted", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    // Say "No" at the agreements prompt
    std::istringstream installInput{ "n" };

    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_LicenseAgreement.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify agreements are shown
    REQUIRE(installOutput.str().find("Agreement with text") != std::string::npos);
    REQUIRE(installOutput.str().find("This is the text of the agreement.") != std::string::npos);
    REQUIRE(installOutput.str().find("Agreement with URL") != std::string::npos);
    REQUIRE(installOutput.str().find("https://TestAgreementUrl") != std::string::npos);

    // Verify installation failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    REQUIRE_FALSE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsNotAgreedTo).get()) != std::string::npos);
}

TEST_CASE("ShowFlow_SearchAndShowAppInfo", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnOne"sv);

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify AppInfo is printed
    REQUIRE(showOutput.str().find("AppInstallerCliTest.TestExeInstaller") != std::string::npos);
    REQUIRE(showOutput.str().find("AppInstaller Test Exe Installer") != std::string::npos);
    REQUIRE(showOutput.str().find("1.0.0.0") != std::string::npos);
    REQUIRE(showOutput.str().find("https://ThisIsNotUsed") != std::string::npos);
}

TEST_CASE("ShowFlow_SearchAndShowAppVersion", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForOpenSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnOne"sv);
    context.Args.AddArg(Execution::Args::Type::ListVersions);

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify App version is printed
    REQUIRE(showOutput.str().find("1.0.0.0") != std::string::npos);
    // No manifest info is printed
    REQUIRE(showOutput.str().find("  Download Url: https://ThisIsNotUsed") == std::string::npos);
}

TEST_CASE("ShowFlow_Dependencies", "[ShowFlow][workflow][dependencies]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-AllDependencyTypes.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({true});

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify all types of dependencies are printed
    REQUIRE(showOutput.str().find("Dependencies") != std::string::npos);
    REQUIRE(showOutput.str().find("WindowsFeaturesDep") != std::string::npos);
    REQUIRE(showOutput.str().find("WindowsLibrariesDep") != std::string::npos);
    // PackageDep1 has minimum version (1.0), PackageDep2 doesn't (shouldn't show [>=...])
    REQUIRE(showOutput.str().find("Package.Dep1-x64 [>= 1.0]") != std::string::npos);
    REQUIRE(showOutput.str().find("Package.Dep2-x64") != std::string::npos);
    REQUIRE(showOutput.str().find("Package.Dep2-x64 [") == std::string::npos);
    REQUIRE(showOutput.str().find("ExternalDep") != std::string::npos);
}

TEST_CASE("ShowFlow_InstallerType", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify that just the base installed type is shown;
    REQUIRE(showOutput.str().find(Resource::LocString(Resource::String::ShowLabelInstallerType)) != std::string::npos);
    REQUIRE(showOutput.str().find("exe") != std::string::npos);

    // If the base installer is incorrectly shown, an open parenthesis would appear after the effective installer type
    REQUIRE(showOutput.str().find("exe (") == std::string::npos);
}

TEST_CASE("ShowFlow_NestedInstallerType", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Zip_Exe.yaml").GetPath().u8string());

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify that both the effective and base installer types are shown
    REQUIRE(showOutput.str().find(Resource::LocString(Resource::String::ShowLabelInstallerType)) != std::string::npos);
    REQUIRE(showOutput.str().find("exe (zip)") != std::string::npos);
}

TEST_CASE("DependencyGraph_SkipInstalled", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    Manifest manifest = CreateFakeManifestWithDependencies("DependenciesInstalled");
    OverrideOpenDependencySource(context);
    OverrideForInstallMultiplePackages(context);

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    context << ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies);

    auto& dependencyPackages = context.Get<Execution::Data::PackagesToInstall>();
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);
    REQUIRE(dependencyPackages.size() == 0);
}

TEST_CASE("DependencyGraph_validMinVersions", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    Manifest manifest = CreateFakeManifestWithDependencies("DependenciesValidMinVersions");
    OverrideOpenDependencySource(context);
    OverrideForInstallMultiplePackages(context);

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    context << ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies);

    auto& dependencyPackages = context.Get<Execution::Data::PackagesToInstall>();

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);
    REQUIRE(dependencyPackages.size() == 1);
    REQUIRE(dependencyPackages.at(0)->Get<Execution::Data::Manifest>().Id == "minVersion");
    // minVersion 1.5 is available but this requires 1.0 so that version is installed
    REQUIRE(dependencyPackages.at(0)->Get<Execution::Data::Manifest>().Version == "1.0");
}

TEST_CASE("DependencyGraph_PathNoLoop", "[InstallFlow][workflow][dependencyGraph][dependencies]", )
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    Manifest manifest = CreateFakeManifestWithDependencies("PathBetweenBranchesButNoLoop");
    OverrideOpenDependencySource(context);
    OverrideForInstallMultiplePackages(context);

    context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
    context.Add<Execution::Data::Manifest>(manifest);
    context.Add<Execution::Data::Installer>(manifest.Installers[0]);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    context << ManagePackageDependencies(Resource::String::InstallAndUpgradeCommandsReportDependencies);

    auto& dependencyPackages = context.Get<Execution::Data::PackagesToInstall>();

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);

    // Verify installers are called in order
    REQUIRE(dependencyPackages.size() == 4);
    REQUIRE(dependencyPackages.at(0)->Get<Execution::Data::Manifest>().Id == "B");
    REQUIRE(dependencyPackages.at(1)->Get<Execution::Data::Manifest>().Id == "C");
    REQUIRE(dependencyPackages.at(2)->Get<Execution::Data::Manifest>().Id == "G");
    REQUIRE(dependencyPackages.at(3)->Get<Execution::Data::Manifest>().Id == "H");
}

TEST_CASE("UpdateFlow_UpdateWithManifest", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("UpdateFlowTest_Exe.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("/update") != std::string::npos);
    REQUIRE(updateResultStr.find("/silentwithprogress") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdateWithManifestMSStore", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestMSStoreUpdated.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForMSStore(context, true);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_MSStore.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("9WZDNCRFJ364") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdateWithManifestAppNotInstalled", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallerArgTest_Inno_NoSwitches.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::NoInstalledPackageFound).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
}

TEST_CASE("UpdateFlow_UpdateWithManifestVersionAlreadyInstalled", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpdateNotApplicable).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExe", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("/update") != std::string::npos);
    REQUIRE(updateResultStr.find("/silence") != std::string::npos);
    REQUIRE(updateResultStr.find("/ver3.0.0.0") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdateZip_Exe", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForShellExecute(context);
    OverrideForExtractInstallerFromArchive(context);
    OverrideForVerifyAndSetNestedInstaller(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestZipInstaller"sv);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("/custom") != std::string::npos);
    REQUIRE(updateResultStr.find("/silence") != std::string::npos);
    REQUIRE(updateResultStr.find("/ver2.0.0.0") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdatePortable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestPortableInstaller"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_Portable_SymlinkCreationFail", "[UpdateFlow][workflow]")
{
    // Update portable with symlink creation failure verify that it succeeds.
    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto PreviousThreadGlobals = context.SetForCurrentThread();
    bool overrideCreateSymlinkStatus = false;
    AppInstaller::Filesystem::TestHook_SetCreateSymlinkResult_Override(&overrideCreateSymlinkStatus);
    OverridePortableInstaller(context);
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestPortableInstaller"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());
    const auto& portableTargetDirectory = AppInstaller::Runtime::GetPathTo(AppInstaller::Runtime::PathName::PortablePackageUserRoot) / "AppInstallerCliTest.TestPortableInstaller__TestSource";
    const auto& portableTargetPath = portableTargetDirectory / "AppInstallerTestExeInstaller.exe";
    REQUIRE(std::filesystem::exists(portableTargetPath));
    REQUIRE(AppInstaller::Registry::Environment::PathVariable(AppInstaller::Manifest::ScopeEnum::User).Contains(portableTargetDirectory));

    // Perform uninstall
    std::ostringstream uninstallOutput;
    TestContext uninstallContext{ uninstallOutput, std::cin };
    auto previousThreadGlobals = uninstallContext.SetForCurrentThread();
    OverrideForCompositeInstalledSource(uninstallContext);
    uninstallContext.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestPortableInstaller"sv);

    UninstallCommand uninstall({});
    uninstall.Execute(uninstallContext);
    INFO(uninstallOutput.str());

    REQUIRE_FALSE(std::filesystem::exists(portableTargetPath));
    REQUIRE_FALSE(AppInstaller::Registry::Environment::PathVariable(AppInstaller::Manifest::ScopeEnum::User).Contains(portableTargetDirectory));
}

TEST_CASE("UpdateFlow_UpdateExeWithUnsupportedArgs", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");
    TestCommon::TempDirectory tempDirectory("TempDirectory", false);

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestExeInstallerWithUnsupportedArguments"sv);
    context.Args.AddArg(Execution::Args::Type::InstallLocation, tempDirectory);

    UpgradeCommand update({});
    context.SetExecutingCommand(&update);
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify unsupported arguments error message is shown 
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UNSUPPORTED_ARGUMENT);
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UnsupportedArgument).get()) != std::string::npos);
    REQUIRE(updateOutput.str().find("-l,--location") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdatePortableWithManifest", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForPortableInstallFlow(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("UpdateFlowTest_Portable.yaml").GetPath().u8string());

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateMsix", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestMsixInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForMSIX(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestMsixInstaller"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateMSStore", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestMSStoreUpdated.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForMSStore(context, true);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestMSStoreInstaller"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
    std::ifstream updateResultFile(updateResultPath.GetPath());
    REQUIRE(updateResultFile.is_open());
    std::string updateResultStr;
    std::getline(updateResultFile, updateResultStr);
    REQUIRE(updateResultStr.find("9WZDNCRFJ364") != std::string::npos);
}

TEST_CASE("UpdateFlow_UpdateExeLatestAlreadyInstalled", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestExeInstallerWithLatestInstalled"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpdateNotApplicable).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExeInstallerTypeNotApplicable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestExeInstallerWithIncompatibleInstallerType"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeDifferentInstallTechnologyInNewerVersions).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExeInstallerTypeNotApplicableSpecificVersion", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestExeInstallerWithIncompatibleInstallerType"sv);
    context.Args.AddArg(Execution::Args::Type::Version, "2.0.0.0"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeDifferentInstallTechnology).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExeWithDifferentInstalledType", "[UpdateFlow][workflow]")
{
    // Tests installer applicability when installed type is different but listed in the manifest
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestExeInstallerWithDifferentInstalledType"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is called.
    REQUIRE(context.GetTerminationHR() == S_OK);
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpdateExeSpecificVersionNotFound", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);
    context.Args.AddArg(Execution::Args::Type::Version, "1.2.3.4"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::GetManifestResultVersionNotFound).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_NO_MANIFEST_FOUND);
}

TEST_CASE("UpdateFlow_UpdateExeSpecificVersionNotApplicable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestExeInstallerWithIncompatibleInstallerType"sv);
    context.Args.AddArg(Execution::Args::Type::Version, "1.0.0.0"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpdateNotApplicable).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateAllApplicable", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");
    TestCommon::TempFile updateMSStoreResultPath("TestMSStoreUpdated.txt");
    TestCommon::TempFile updatePortableResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForShellExecute(context);
    OverrideForMSIX(context);
    OverrideForMSStore(context, true);
    OverrideForPortableInstall(context);
    context.Args.AddArg(Execution::Args::Type::All);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify installers are called.
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updatePortableResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_UpgradeWithDuplicateUpgradeItemsFound", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    // Installer should only be run once since the 2 upgrade items are same.
    OverrideForShellExecute(context, 1);
    context.Args.AddArg(Execution::Args::Type::Query, "TestUpgradeAllWithDuplicateUpgradeItems"sv);
    context.Args.AddArg(Execution::Args::Type::All);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify installers are called.
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_Dependencies", "[UpdateFlow][workflow][dependencies]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller.Dependencies"sv);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    std::string updateResultStr = updateOutput.str();

    // Verify dependencies are informed
    REQUIRE(updateResultStr.find(Resource::LocString(Resource::String::InstallAndUpgradeCommandsReportDependencies).get()) != std::string::npos);
    REQUIRE(updateResultStr.find("PreviewIIS") != std::string::npos);
    REQUIRE(updateResultStr.find("Preview VC Runtime") != std::string::npos);
}

TEST_CASE("UpdateFlow_LicenseAgreement", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestInstallerWithLicenseAgreement"sv);
    context.Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify agreements are shown
    REQUIRE(updateOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the EXE") != std::string::npos);

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(updateResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_LicenseAgreement_NotAccepted", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    // Say "No" at the agreements prompt
    std::istringstream updateInput{ "n" };

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, updateInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestInstallerWithLicenseAgreement"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify agreements are shown
    REQUIRE(updateOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the EXE") != std::string::npos);

    // Verify Installer is not called.
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    REQUIRE_FALSE(std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsNotAgreedTo).get()) != std::string::npos);
}

TEST_CASE("UpdateFlow_All_LicenseAgreement", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");
    TestCommon::TempFile updateMSStoreResultPath("TestMSStoreUpdated.txt");
    TestCommon::TempFile updatePortableResultPath("TestPortableInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, TestSourceSearchOptions::UpgradeUsesAgreements);
    OverrideForShellExecute(context);
    OverrideForMSIX(context);
    OverrideForMSStore(context, true);
    OverrideForPortableInstall(context);
    context.Args.AddArg(Execution::Args::Type::All);
    context.Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify agreements are shown
    REQUIRE(updateOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("Agreement for MSIX") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the MSIX") != std::string::npos);

    // Verify installers are called.
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updatePortableResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_All_LicenseAgreement_NotAccepted", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");
    TestCommon::TempFile updateMSStoreResultPath("TestMSStoreUpdated.txt");

    // Say "No" at the agreements prompt
    std::istringstream updateInput{ "n" };

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, updateInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context, TestSourceSearchOptions::UpgradeUsesAgreements);
    context.Args.AddArg(Execution::Args::Type::All);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify agreements are shown
    REQUIRE(updateOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the EXE") != std::string::npos);
    REQUIRE(updateOutput.str().find("Agreement for MSIX") != std::string::npos);
    REQUIRE(updateOutput.str().find("This is the agreement for the MSIX") != std::string::npos);

    // Verify installers are not called.
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    REQUIRE_FALSE(std::filesystem::exists(updateExeResultPath.GetPath()));
    REQUIRE_FALSE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE_FALSE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
}

TEST_CASE("UninstallFlow_UninstallPortable", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestPortableUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForPortableUninstall(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestPortableInstaller"sv);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());
    REQUIRE(std::filesystem::exists(uninstallResultPath.GetPath()));
}

TEST_CASE("UpdateFlow_RequireExplicit", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateExeResultPath("TestExeInstalled.txt");
    TestCommon::TempFile updateMsixResultPath("TestMsixInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();

    // Msix package has an update that requires explicit upgrade.
    // Exe, Portable, MSStore, Zip are also listed with an available upgrade.
    OverrideForCompositeInstalledSource(context, TestSourceSearchOptions::UpgradeRequiresExplicit);

    SECTION("List available upgrades")
    {
        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        // The package that requires explicit upgrade is listed below the header for pinned packages
        REQUIRE(updateOutput.str().find("AppInstallerCliTest.TestExeInstaller") != std::string::npos);

        auto pinnedPackagesHeaderPosition = updateOutput.str().find(Resource::LocString(Resource::String::UpgradeAvailableForPinned));
        auto pinnedPackageLinePosition = updateOutput.str().find("AppInstallerCliTest.TestMsixInstaller");
        REQUIRE(pinnedPackagesHeaderPosition != std::string::npos);
        REQUIRE(pinnedPackageLinePosition != std::string::npos);
        REQUIRE(pinnedPackagesHeaderPosition < pinnedPackageLinePosition);
        REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeRequireExplicitCount)) == std::string::npos);
    }

    SECTION("Upgrade all except pinned")
    {
        context.Args.AddArg(Args::Type::All);
        OverrideForMSStore(context, true);
        OverrideForPortableInstall(context);
        OverrideForShellExecute(context);
        OverrideForExtractInstallerFromArchive(context);
        OverrideForVerifyAndSetNestedInstaller(context);

        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        auto s = updateOutput.str();

        // Verify message is printed for skipped package
        REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpgradeRequireExplicitCount)) != std::string::npos);

        // Verify package is not installed, but all others are
        REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
        REQUIRE(!std::filesystem::exists(updateMsixResultPath.GetPath()));
    }

    SECTION("Upgrade explicitly")
    {
        context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestMsixInstaller"sv);
        OverrideForMSIX(context);

        UpgradeCommand update({});
        update.Execute(context);
        INFO(updateOutput.str());

        REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    }

    // Command should always succeed
    REQUIRE(context.GetTerminationHR() == S_OK);
}

TEST_CASE("UninstallFlow_UninstallExe", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestExeUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForExeUninstall(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestExeInstaller"sv);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());

    // Verify Uninstaller is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(uninstallResultPath.GetPath()));
    std::ifstream uninstallResultFile(uninstallResultPath.GetPath());
    REQUIRE(uninstallResultFile.is_open());
    std::string uninstallResultStr;
    std::getline(uninstallResultFile, uninstallResultStr);
    REQUIRE(uninstallResultStr.find("uninstall.exe") != std::string::npos);
    REQUIRE(uninstallResultStr.find("/silence") != std::string::npos);
}

TEST_CASE("UninstallFlow_UninstallMsix", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestMsixUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForMSIXUninstall(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestMsixInstaller"sv);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());

    // Verify Uninstaller is called with the package full name.
    REQUIRE(std::filesystem::exists(uninstallResultPath.GetPath()));
    std::ifstream uninstallResultFile(uninstallResultPath.GetPath());
    REQUIRE(uninstallResultFile.is_open());
    std::string uninstallResultStr;
    std::getline(uninstallResultFile, uninstallResultStr);
    REQUIRE(uninstallResultStr.find("20477fca-282d-49fb-b03e-371dca074f0f_8wekyb3d8bbwe") != std::string::npos);
}

TEST_CASE("UninstallFlow_UninstallMSStore", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestMsixUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    OverrideForMSIXUninstall(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.TestMSStoreInstaller"sv);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());

    // Verify Uninstaller is called with the package full name
    REQUIRE(std::filesystem::exists(uninstallResultPath.GetPath()));
    std::ifstream uninstallResultFile(uninstallResultPath.GetPath());
    REQUIRE(uninstallResultFile.is_open());
    std::string uninstallResultStr;
    std::getline(uninstallResultFile, uninstallResultStr);
    REQUIRE(uninstallResultStr.find("microsoft.skypeapp_kzf8qxf38zg5c") != std::string::npos);
}

TEST_CASE("UninstallFlow_UninstallExeNotFound", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestExeUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "AppInstallerCliTest.MissingApp"sv);
    context.Args.AddArg(Execution::Args::Type::Silent);

    UninstallCommand uninstall({});
    uninstall.Execute(context);
    INFO(uninstallOutput.str());

    // Verify Uninstaller is not called.
    REQUIRE(!std::filesystem::exists(uninstallResultPath.GetPath()));
    REQUIRE(uninstallOutput.str().find(Resource::LocString(Resource::String::NoInstalledPackageFound).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_NO_APPLICATIONS_FOUND);
}

TEST_CASE("ExportFlow_ExportAll", "[ExportFlow][workflow]")
{
    TestCommon::TempFile exportResultPath("TestExport.json");

    std::ostringstream exportOutput;
    TestContext context{ exportOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::OutputFile, exportResultPath);

    ExportCommand exportCommand({});
    exportCommand.Execute(context);
    INFO(exportOutput.str());

    // Verify contents of exported collection
    const auto& exportedCollection = context.Get<Execution::Data::PackageCollection>();
    REQUIRE(exportedCollection.Sources.size() == 1);
    REQUIRE(exportedCollection.Sources[0].Details.Identifier == "*TestSource");

    const auto& exportedPackages = exportedCollection.Sources[0].Packages;
    REQUIRE(exportedPackages.size() == 5);
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestExeInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestMsixInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestMSStoreInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestPortableInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestZipInstaller" && p.VersionAndChannel.GetVersion().ToString().empty();
        }));
}

TEST_CASE("ExportFlow_ExportAll_WithVersions", "[ExportFlow][workflow]")
{
    TestCommon::TempFile exportResultPath("TestExport.json");

    std::ostringstream exportOutput;
    TestContext context{ exportOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::OutputFile, exportResultPath);
    context.Args.AddArg(Execution::Args::Type::IncludeVersions);

    ExportCommand exportCommand({});
    exportCommand.Execute(context);
    INFO(exportOutput.str());

    // Verify contents of exported collection
    const auto& exportedCollection = context.Get<Execution::Data::PackageCollection>();
    REQUIRE(exportedCollection.Sources.size() == 1);
    REQUIRE(exportedCollection.Sources[0].Details.Identifier == "*TestSource");

    const auto& exportedPackages = exportedCollection.Sources[0].Packages;
    REQUIRE(exportedPackages.size() == 5);
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestExeInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestMsixInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestMSStoreInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestPortableInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
    REQUIRE(exportedPackages.end() != std::find_if(exportedPackages.begin(), exportedPackages.end(), [](const auto& p)
        {
            return p.Id == "AppInstallerCliTest.TestZipInstaller" && p.VersionAndChannel.GetVersion().ToString() == "1.0.0.0";
        }));
}

TEST_CASE("ImportFlow_Successful", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");
    TestCommon::TempFile msixInstallResultPath("TestMsixInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForMSIX(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify all packages were installed
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(msixInstallResultPath.GetPath()));
}

TEST_CASE("ImportFlow_PackageAlreadyInstalled", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context, true);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-AlreadyInstalled.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Exe should not have been installed again
    REQUIRE(!std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::ImportPackageAlreadyInstalled).get()) != std::string::npos);
}

TEST_CASE("ImportFlow_IgnoreVersions", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-AlreadyInstalled.json").GetPath().string());
    context.Args.AddArg(Execution::Args::Type::IgnoreVersions);

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Specified version is already installed. It should have been updated since we ignored the version.
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
}

TEST_CASE("ImportFlow_MissingSource", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-UnknownSource.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Installer should not be called
    REQUIRE(!std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::ImportSourceNotInstalled).get()) != std::string::npos);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_SOURCE_NAME_DOES_NOT_EXIST);
}

TEST_CASE("ImportFlow_MissingPackage", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-UnknownPackage.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Installer should not be called
    REQUIRE(!std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::ImportSearchFailed).get()) != std::string::npos);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND);
}

TEST_CASE("ImportFlow_IgnoreMissingPackage", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-UnknownPackage.json").GetPath().string());
    context.Args.AddArg(Execution::Args::Type::IgnoreUnavailable);

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify installer was called for the package that was available.
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::ImportSearchFailed).get()) != std::string::npos);
}

TEST_CASE("ImportFlow_MissingVersion", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-UnknownPackageVersion.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Installer should not be called
    REQUIRE(!std::filesystem::exists(exeInstallResultPath.GetPath()));
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::ImportSearchFailed).get()) != std::string::npos);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NOT_ALL_PACKAGES_FOUND);
}

TEST_CASE("ImportFlow_MalformedJsonFile", "[ImportFlow][workflow]")
{
    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-Malformed.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Command should have failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
}

TEST_CASE("ImportFlow_InvalidJsonFile", "[ImportFlow][workflow]")
{
    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Bad-Invalid.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Command should have failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_JSON_INVALID_FILE);
}

TEST_CASE("ImportFlow_MachineScope", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-MachineScope.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify all packages were installed
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
    std::ifstream installResultFile(exeInstallResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/scope=machine") != std::string::npos);
}

TEST_CASE("ImportFlow_Dependencies", "[ImportFlow][workflow][dependencies]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");
    TestCommon::TempFile msixInstallResultPath("TestMsixInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForMSIX(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-Dependencies.json").GetPath().string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());
    
    // Verify dependencies for all packages are informed
    REQUIRE(importOutput.str().find(Resource::LocString(Resource::String::ImportCommandReportDependencies).get()) != std::string::npos);
    REQUIRE(importOutput.str().find("PreviewIIS") != std::string::npos);
    REQUIRE(importOutput.str().find("Preview VC Runtime") != std::string::npos);
    REQUIRE(importOutput.str().find("Hyper-V") != std::string::npos);
}

TEST_CASE("ImportFlow_LicenseAgreement", "[ImportFlow][workflow]")
{
    TestCommon::TempFile exeInstallResultPath("TestExeInstalled.txt");

    std::ostringstream importOutput;
    TestContext context{ importOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-WithLicenseAgreement.json").GetPath().string());
    context.Args.AddArg(Execution::Args::Type::AcceptPackageAgreements);

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify agreements are shown
    REQUIRE(importOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(importOutput.str().find("This is the agreement for the EXE") != std::string::npos);

    // Verify all packages were installed
    REQUIRE(std::filesystem::exists(exeInstallResultPath.GetPath()));
}

TEST_CASE("ImportFlow_LicenseAgreement_NotAccepted", "[ImportFlow][workflow]")
{
    // Say "No" at the agreements prompt
    std::istringstream importInput{ "n" };

    std::ostringstream importOutput;
    TestContext context{ importOutput, importInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForImportSource(context);
    context.Args.AddArg(Execution::Args::Type::ImportFile, TestDataFile("ImportFile-Good-WithLicenseAgreement.json").GetPath().string());

    ImportCommand importCommand({});
    importCommand.Execute(context);
    INFO(importOutput.str());

    // Verify agreements are shown
    REQUIRE(importOutput.str().find("Agreement for EXE") != std::string::npos);
    REQUIRE(importOutput.str().find("This is the agreement for the EXE") != std::string::npos);

    // Command should have failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
}

void VerifyMotw(const std::filesystem::path& testFile, DWORD zone)
{
    std::filesystem::path motwFile(testFile);
    motwFile += ":Zone.Identifier:$data";
    std::ifstream motwStream(motwFile);
    std::stringstream motwContent;
    motwContent << motwStream.rdbuf();
    std::string motwContentStr = motwContent.str();
    motwStream.close();
    REQUIRE(motwContentStr.find("ZoneId=" + std::to_string(zone)) != std::string::npos);
}

TEST_CASE("VerifyInstallerTrustLevelAndUpdateInstallerFileMotw", "[DownloadInstaller][workflow]")
{
    TestCommon::TempFile testInstallerPath("TestInstaller.txt");

    std::ofstream ofile(testInstallerPath, std::ofstream::out);
    ofile << "test";
    ofile.close();

    ApplyMotwIfApplicable(testInstallerPath, URLZONE_INTERNET);
    VerifyMotw(testInstallerPath, 3);

    std::ostringstream updateMotwOutput;
    TestContext context{ updateMotwOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Add<Data::HashPair>({ {}, {} });
    context.Add<Data::InstallerPath>(testInstallerPath);
    auto packageVersion = std::make_shared<TestPackageVersion>(Manifest{});
    auto testSource = std::make_shared<TestSource>();
    testSource->Details.TrustLevel = SourceTrustLevel::Trusted;
    packageVersion->Source = testSource;
    context.Add<Data::PackageVersion>(packageVersion);
    ManifestInstaller installer;
    installer.Url = "http://NotTrusted.com";
    context.Add<Data::Installer>(std::move(installer));

    context << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
    REQUIRE(WI_IsFlagSet(context.GetFlags(), ContextFlag::InstallerTrusted));
    VerifyMotw(testInstallerPath, 2);

    testSource->Details.TrustLevel = SourceTrustLevel::None;
    context.ClearFlags(ContextFlag::InstallerTrusted);
    context << VerifyInstallerHash << UpdateInstallerFileMotwIfApplicable;
    REQUIRE_FALSE(WI_IsFlagSet(context.GetFlags(), ContextFlag::InstallerTrusted));
    VerifyMotw(testInstallerPath, 3);

    INFO(updateMotwOutput.str());
}

TEST_CASE("InstallFlowMultiLocale_RequirementNotSatisfied", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-MultiLocale.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("InstallFlowMultiLocale_RequirementSatisfied", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-MultiLocale.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::Locale, "fr-FR"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/fr-FR") != std::string::npos);
}

TEST_CASE("InstallFlowMultiLocale_PreferenceNoBetterLocale", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-MultiLocale.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::InstallLocalePreference>({ "zh-CN" });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/unknown") != std::string::npos);
}

TEST_CASE("InstallFlowMultiLocale_PreferenceWithBetterLocale", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Manifest-Good-MultiLocale.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::InstallLocalePreference>({ "en-US" });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/en-GB") != std::string::npos);
}

// TODO:
// add dependencies for installer tests to DependenciesTestSource (or a new one)
// add tests for min version dependency solving
// add tests that check for correct installation of dependencies (not only the order)

TEST_CASE("SourceAddFlow_Agreement", "[SourceAddFlow][workflow]")
{
    std::ostringstream sourceAddOutput;
    TestContext context{ sourceAddOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForSourceAddWithAgreements(context);
    context.Args.AddArg(Execution::Args::Type::SourceName, "TestSource"sv);
    context.Args.AddArg(Execution::Args::Type::SourceType, "Microsoft.Test"sv);
    context.Args.AddArg(Execution::Args::Type::SourceArg, "TestArg"sv);
    context.Args.AddArg(Execution::Args::Type::AcceptSourceAgreements);

    SourceAddCommand sourceAdd({});
    sourceAdd.Execute(context);
    INFO(sourceAddOutput.str());

    // Verify agreements are shown
    REQUIRE(sourceAddOutput.str().find("Agreement Label") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("Agreement Text") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("https://test") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find(Resource::LocString(Resource::String::SourceAgreementsMarketMessage).get()) != std::string::npos);

    // Verify Installer is called.
    REQUIRE(context.GetTerminationHR() == S_OK);
}

TEST_CASE("SourceAddFlow_Agreement_Prompt_Yes", "[SourceAddFlow][workflow]")
{
    // Accept the agreements by saying "Yes" at the prompt
    std::istringstream sourceAddInput{ "y" };
    std::ostringstream sourceAddOutput;
    TestContext context{ sourceAddOutput, sourceAddInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForSourceAddWithAgreements(context);
    context.Args.AddArg(Execution::Args::Type::SourceName, "TestSource"sv);
    context.Args.AddArg(Execution::Args::Type::SourceType, "Microsoft.Test"sv);
    context.Args.AddArg(Execution::Args::Type::SourceArg, "TestArg"sv);

    SourceAddCommand sourceAdd({});
    sourceAdd.Execute(context);
    INFO(sourceAddOutput.str());

    // Verify agreements are shown
    REQUIRE(sourceAddOutput.str().find("Agreement Label") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("Agreement Text") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("https://test") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find(Resource::LocString(Resource::String::SourceAgreementsMarketMessage).get()) != std::string::npos);

    // Verify Installer is called.
    REQUIRE(context.GetTerminationHR() == S_OK);
}

TEST_CASE("SourceAddFlow_Agreement_Prompt_No", "[SourceAddFlow][workflow]")
{
    // Accept the agreements by saying "No" at the prompt
    std::istringstream sourceAddInput{ "n" };
    std::ostringstream sourceAddOutput;
    TestContext context{ sourceAddOutput, sourceAddInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForSourceAddWithAgreements(context, false);
    context.Args.AddArg(Execution::Args::Type::SourceName, "TestSource"sv);
    context.Args.AddArg(Execution::Args::Type::SourceType, "Microsoft.Test"sv);
    context.Args.AddArg(Execution::Args::Type::SourceArg, "TestArg"sv);

    SourceAddCommand sourceAdd({});
    sourceAdd.Execute(context);
    INFO(sourceAddOutput.str());

    // Verify agreements are shown
    REQUIRE(sourceAddOutput.str().find("Agreement Label") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("Agreement Text") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find("https://test") != std::string::npos);
    REQUIRE(sourceAddOutput.str().find(Resource::LocString(Resource::String::SourceAgreementsMarketMessage).get()) != std::string::npos);

    // Verify Installer is called.
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_SOURCE_AGREEMENTS_NOT_ACCEPTED);
}

TEST_CASE("ValidateCommand_Dependencies", "[workflow][dependencies]")
{
    std::ostringstream validateOutput;
    TestContext context{ validateOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::ValidateManifest, TestDataFile("Manifest-Good-AllDependencyTypes.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    ValidateCommand validate({});
    validate.Execute(context);
    INFO(validateOutput.str());

    // Verify all types of dependencies are printed
    REQUIRE(validateOutput.str().find(Resource::LocString(Resource::String::ValidateCommandReportDependencies).get()) != std::string::npos);
    REQUIRE(validateOutput.str().find("WindowsFeaturesDep") != std::string::npos);
    REQUIRE(validateOutput.str().find("WindowsLibrariesDep") != std::string::npos);
    // PackageDep1 has minimum version (1.0), PackageDep2 doesn't (shouldn't show [>=...])
    REQUIRE(validateOutput.str().find("Package.Dep1-x64 [>= 1.0]") != std::string::npos);
    REQUIRE(validateOutput.str().find("Package.Dep2-x64") != std::string::npos);
    REQUIRE(validateOutput.str().find("Package.Dep2-x64 [") == std::string::npos);
    REQUIRE(validateOutput.str().find("ExternalDep") != std::string::npos);
}

TEST_CASE("DependencyGraph_StackOrderIsOk", "[InstallFlow][workflow][dependencyGraph][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    std::vector<Dependency> installationOrder;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideOpenSourceForDependencies(context);
    OverrideForShellExecute(context, installationOrder);

    context.Args.AddArg(Execution::Args::Type::Query, "StackOrderIsOk"sv);

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::DependenciesFlowContainsLoop)) == std::string::npos);

    // Verify installers are called in order
    REQUIRE(installationOrder.size() == 3);
    REQUIRE(installationOrder.at(0).Id == "B");
    REQUIRE(installationOrder.at(1).Id == "C");
    REQUIRE(installationOrder.at(2).Id == "StackOrderIsOk");
}

TEST_CASE("InstallerWithoutDependencies_RootDependenciesAreUsed", "[dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Installer_Exe_DependenciesOnRoot.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify root dependencies are shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallAndUpgradeCommandsReportDependencies).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("PreviewIISOnRoot") != std::string::npos);
}

TEST_CASE("DependenciesMultideclaration_InstallerDependenciesPreference", "[dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Installer_Exe_DependenciesMultideclaration.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify installer dependencies are shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallAndUpgradeCommandsReportDependencies).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("PreviewIIS") != std::string::npos);
    // and root dependencies are not
    REQUIRE(installOutput.str().find("PreviewIISOnRoot") == std::string::npos);
}

TEST_CASE("InstallFlow_Dependencies", "[InstallFlow][workflow][dependencies]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    OverrideDependencySource(context);

    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("Installer_Exe_Dependencies.yaml").GetPath().u8string());

    TestUserSettings settings;
    settings.Set<AppInstaller::Settings::Setting::EFDependencies>({ true });

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify all types of dependencies are printed
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallAndUpgradeCommandsReportDependencies).get()) != std::string::npos);
    REQUIRE(installOutput.str().find("PreviewIIS") != std::string::npos);
}

TEST_CASE("OpenSource_WithCustomHeader", "[OpenSource][CustomHeader]")
{
    SetSetting(Stream::UserSources, R"(Sources:)"sv);
    TestHook_ClearSourceFactoryOverrides();

    SourceDetails details;
    details.Name = "restsource";
    details.Type = "Microsoft.Rest";
    details.Arg = "thisIsTheArg";
    details.Data = "thisIsTheData";

    std::string customHeader = "Test custom header in Open source Flow";

    bool receivedCustomHeader = false;
    TestSourceFactory factory{
        [&](const SourceDetails& sd, std::optional<std::string> header)
        {
            receivedCustomHeader = header.value() == customHeader;
            return std::shared_ptr<ISource>(new TestSource(sd));
        } };
    TestHook_SetSourceFactoryOverride(details.Type, factory);

    TestProgress progress;
    AddSource(details, progress);

    std::ostringstream output;
    TestContext context{ output, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Query, "TestQuery"sv);
    context.Args.AddArg(Execution::Args::Type::CustomHeader, customHeader);
    context.Args.AddArg(Execution::Args::Type::Source, details.Name);

    AppInstaller::CLI::Workflow::OpenSource()(context);
    REQUIRE(receivedCustomHeader);
}

TEST_CASE("AdminSetting_LocalManifestFiles", "[LocalManifests][workflow]")
{
    RemoveSetting(Stream::AdminSettings);

    {
        // If there's no admin setting, using local manifest should fail.
        Execution::Args args;
        args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand({});
        REQUIRE_THROWS(installCommand.ValidateArguments(args));
    }

    {
        // Using settings command to enable local manifests
        std::ostringstream settingsOutput;
        TestContext context{ settingsOutput, std::cin };
        auto previousThreadGlobals = context.SetForCurrentThread();
        context.Args.AddArg(Execution::Args::Type::AdminSettingEnable, "LocalManifestFiles"sv);
        context.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
        SettingsCommand settings({});
        settings.Execute(context);
        INFO(settingsOutput.str());
    }

    {
        // Now using local manifests should succeed
        Execution::Args args2;
        args2.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand2({});
        REQUIRE_NOTHROW(installCommand2.ValidateArguments(args2));
    }

    {
        // Using settings command to disable local manifests
        std::ostringstream settingsOutput2;
        TestContext context2{ settingsOutput2, std::cin };
        auto previousThreadGlobals = context2.SetForCurrentThread();
        context2.Args.AddArg(Execution::Args::Type::AdminSettingDisable, "LocalManifestFiles"sv);
        context2.Override({ EnsureRunningAsAdmin, [](TestContext&) {} });
        SettingsCommand settings2({});
        settings2.Execute(context2);
        INFO(settingsOutput2.str());
    }

    {
        // Now using local manifests should fail
        Execution::Args args3;
        args3.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());
        InstallCommand installCommand3({});
        REQUIRE_THROWS(installCommand3.ValidateArguments(args3));
    }
}

TEST_CASE("PromptFlow_InteractivityDisabled", "[PromptFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");
    TestCommon::TestUserSettings testSettings;

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_LicenseAgreement.yaml").GetPath().u8string());

    SECTION("Disabled by setting")
    {
        testSettings.Set<Setting::InteractivityDisable>(true);
    }
    SECTION("Disabled by arg")
    {
        context.Args.AddArg(Execution::Args::Type::DisableInteractivity);
    }

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt is not shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsPrompt).get()) == std::string::npos);

    // Verify installation failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_PACKAGE_AGREEMENTS_NOT_ACCEPTED);
    REQUIRE_FALSE(std::filesystem::exists(installResultPath.GetPath()));
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::PackageAgreementsNotAgreedTo).get()) != std::string::npos);
}

TEST_CASE("PromptFlow_InstallerAbortsTerminal_Proceed", "[PromptFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    // Accept that the installer may abort the terminal by saying "Yes" at the prompt
    std::istringstream installInput{ "y" };

    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_AbortsTerminal.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt is shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallerAbortsTerminal).get()) != std::string::npos);

    // Verify Installer is called.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("PromptFlow_InstallerAbortsTerminal_Cancel", "[PromptFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    // Cancel the installation by saying "No" at the prompt that the installer may abort the terminal
    std::istringstream installInput{ "n" };

    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_AbortsTerminal.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt is shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallerAbortsTerminal).get()) != std::string::npos);

    // Verify installation failed
    REQUIRE_TERMINATED_WITH(context, E_ABORT);
    REQUIRE_FALSE(std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("PromptFlow_InstallLocationRequired", "[PromptFlow][workflow]")
{
    TestCommon::TempDirectory installLocation("TempDirectory");
    TestCommon::TestUserSettings testSettings;

    std::istringstream installInput;
    std::ostringstream installOutput;
    TestContext context{ installOutput, installInput };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideForShellExecute(context);
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InstallLocationRequired.yaml").GetPath().u8string());

    bool shouldShowPrompt = false;
    std::filesystem::path installResultPath = installLocation.GetPath() / "TestExeInstalled.txt";
    SECTION("From argument")
    {
        context.Args.AddArg(Execution::Args::Type::InstallLocation, installLocation.GetPath().string());
    }
    SECTION("From settings")
    {
        testSettings.Set<Setting::InstallDefaultRoot>(installLocation.GetPath().string());

        // When using the default location from settings, the Package ID is appended to the root
        auto installLocationWithPackageId = installLocation.GetPath() / "AppInstallerCliTest.TestInstaller";
        std::filesystem::create_directory(installLocationWithPackageId);
        installResultPath = installLocationWithPackageId / "TestExeInstalled.txt";
    }
    SECTION("From prompt")
    {
        installInput.str(installLocation.GetPath().string());
        shouldShowPrompt = true;
    }

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    bool promptShown = installOutput.str().find(Resource::LocString(Resource::String::InstallerRequiresInstallLocation).get()) != std::string::npos;
    REQUIRE(shouldShowPrompt == promptShown);

    // Verify Installer is called with the right parameters
    REQUIRE(std::filesystem::exists(installResultPath));
    std::ifstream installResultFile(installResultPath);
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    const auto installDirArgument = "/InstallDir " + installLocation.GetPath().string();
    REQUIRE(installResultStr.find(installDirArgument) != std::string::npos);
}

TEST_CASE("PromptFlow_InstallLocationRequired_Missing", "[PromptFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_InstallLocationRequired.yaml").GetPath().u8string());
    // Disable interactivity so that there is not prompt and we cannot get the required location
    context.Args.AddArg(Execution::Args::Type::DisableInteractivity);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify prompt is shown
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::InstallerRequiresInstallLocation).get()) != std::string::npos);

    // Verify installation failed
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_INSTALL_LOCATION_REQUIRED);
    REQUIRE_FALSE(std::filesystem::exists(installResultPath.GetPath()));
}