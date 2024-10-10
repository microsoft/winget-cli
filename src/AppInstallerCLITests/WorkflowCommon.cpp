// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DependenciesTestSource.h"
#include "WorkflowCommon.h"
#include <winget/ManifestYamlParser.h>
#include <Workflows/ArchiveFlow.h>
#include <Workflows/DownloadFlow.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/MSStoreInstallerHandler.h>
#include <Workflows/DependenciesFlow.h>

using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

namespace TestCommon
{
    namespace TSR
    {
        const TestSourceResult TestQuery_ReturnOne(
            "TestQueryReturnOne"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(std::vector<Manifest>{ manifest }, source),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnOne")));
            });

        const TestSourceResult TestQuery_ReturnTwo(
            "TestQueryReturnTwo"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(std::vector<Manifest>{ manifest }, source),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));

                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("Manifest-Good.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(std::vector<Manifest>{ manifest2 }, source),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));
            });

        const TestSourceResult TestInstaller_Exe(
            "AppInstallerCliTest.TestExeInstaller"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                auto manifest3 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2.yaml"));

                auto testPackage =
                    TestCompositePackage::Make(
                        manifest,
                        TestCompositePackage::MetadataMap
                        {
                            { PackageVersionMetadata::InstalledType, "Exe" },
                            { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                            { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                        },
                        std::vector<Manifest>{ manifest3, manifest2, manifest },
                        source
                        );
                for (auto& availablePackage : testPackage->Available)
                {
                    availablePackage->IsSameOverride = [](const IPackage*, const IPackage*) { return true; };
                }
                matches.emplace_back(
                    ResultMatch(
                        testPackage,
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            });

        const TestSourceResult TestInstaller_Exe_UpgradeUsesAgreements(
            "AppInstallerCliTest.TestExeInstaller"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                auto manifest3 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2_LicenseAgreement.yaml"));

                auto testPackage =
                    TestCompositePackage::Make(
                        manifest,
                        TestCompositePackage::MetadataMap
                        {
                            { PackageVersionMetadata::InstalledType, "Exe" },
                            { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                            { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                        },
                        std::vector<Manifest>{ manifest3, manifest2, manifest },
                        source
                        );
                for (auto& availablePackage : testPackage->Available)
                {
                    availablePackage->IsSameOverride = [](const IPackage*, const IPackage*) { return true; };
                }
                matches.emplace_back(
                    ResultMatch(
                        testPackage,
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            });

        const TestSourceResult TestInstaller_Portable(
            "AppInstallerCliTest.TestPortableInstaller"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Portable.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Portable.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Portable" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestPortableInstaller")));
            });

        const TestSourceResult TestInstaller_Msix(
            "AppInstallerCliTest.TestMsixInstaller"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Msix_StreamingFlow.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Msix.yaml"));

                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Msix" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMsixInstaller")));
            });

        const TestSourceResult TestInstaller_Msix_UpgradeUsesAgreements(
            "AppInstallerCliTest.TestMsixInstaller"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Msix_StreamingFlow.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Msix_LicenseAgreement.yaml"));

                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Msix" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMsixInstaller")));
            });

        const TestSourceResult TestInstaller_Msix_UpgradeRequiresExplicit(
            "AppInstallerCliTest.TestMsixInstaller"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Msix_StreamingFlow.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Msix.yaml"));

                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap
                            {
                                { PackageVersionMetadata::InstalledType, "Msix" },
                                { PackageVersionMetadata::PinnedState, "PinnedByManifest" },
                            },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                            ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMsixInstaller")));
            });

        const TestSourceResult TestInstaller_Zip(
            "AppInstallerCliTest.TestZipInstaller"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Zip_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Zip_Exe.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestZipInstaller")));
            });

        const TestSourceResult TestInstaller_MSStore(
            "AppInstallerCliTest.TestMSStoreInstaller"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto installed = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_MSStore.yaml"));
                auto available = installed;
                // Override the installed version to not be Latest
                installed.Version = "1.0.0.0";
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            installed,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "MSStore" } },
                            std::vector<Manifest>{ available },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMSStoreInstaller")));
            });

        const TestSourceResult TestInstaller_Exe_ExpectedReturnCodes(
            "AppInstallerCliTest.ExpectedReturnCodes"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_ExpectedReturnCodes.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_ExpectedReturnCodes.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.ExpectedReturnCodes")));
            });

        const TestSourceResult TestInstaller_Exe_UnknownVersion(
            "TestExeInstallerWithUnknownVersion"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto installed = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_UnknownVersion.yaml"));
                auto available = installed;
                // Override the installed version to be unknown.
                installed.Version = "unknown";
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            installed,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ available },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeUnknownVersion")));
            });

        const TestSourceResult TestInstaller_Exe_LatestInstalled(
            "TestExeInstallerWithLatestInstalled"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest2,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            });

        const TestSourceResult TestInstaller_Exe_IncompatibleInstallerType(
            "TestExeInstallerWithIncompatibleInstallerType"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Msix" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            });

        const TestSourceResult TestInstaller_Exe_DifferentInstallerType(
            "TestExeInstallerWithDifferentInstalledType"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_ARPInstallerType.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Msix" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            });

        const TestSourceResult TestInstaller_Exe_UnsupportedArguments(
            "TestExeInstallerWithUnsupportedArguments"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_UnsupportedArgs.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            });

        const TestSourceResult TestInstaller_Exe_NothingInstalled(
            "TestExeInstallerWithNothingInstalled"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            std::vector<Manifest>{ manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            });

        const TestSourceResult TestInstaller_Exe_Dependencies(
            "AppInstallerCliTest.TestExeInstaller.Dependencies"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("Installer_Exe_Dependencies.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_ExeDependencies.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap
                            {
                                { PackageVersionMetadata::InstalledType, "Exe" },
                                { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                                { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                            },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                            ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller.Dependencies")));
            });

        const TestSourceResult TestInstaller_Msix_WFDependency(
            "AppInstallerCliTest.TestMsixInstaller.WFDep"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("Installer_Msix_WFDependency.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            std::vector<Manifest>{ manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMsixInstaller.WFDep")));
            });

        const TestSourceResult TestInstaller_Exe_LicenseAgreement(
            "TestInstallerWithLicenseAgreement"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_LicenseAgreement.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2_LicenseAgreement.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest2, manifest },
                            source
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestInstallerWithLicenseAgreement")));
            });

        const TestSourceResult TestInstaller_Exe_UpgradeAllWithDuplicateUpgradeItems(
            "TestUpgradeAllWithDuplicateUpgradeItems"sv,
            [](std::vector<ResultMatch>& matches, std::weak_ptr<const ISource> source) {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                auto manifest3 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe_2.yaml"));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap
                            {
                                { PackageVersionMetadata::InstalledType, "Exe" },
                                { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                                { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                            },
                            std::vector<Manifest>{ manifest3, manifest2, manifest },
                            source
                            ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
                matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest2,
                            TestCompositePackage::MetadataMap
                            {
                                { PackageVersionMetadata::InstalledType, "Exe" },
                                { PackageVersionMetadata::StandardUninstallCommand, "C:\\uninstall.exe" },
                                { PackageVersionMetadata::SilentUninstallCommand, "C:\\uninstall.exe /silence" },
                            },
                            std::vector<Manifest>{ manifest3, manifest2, manifest },
                            source
                            ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            });
    }

    SearchResult WorkflowTestSource::Search(const SearchRequest& request) const
    {
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

        SearchResult result;
        for (const auto& testSourceResult : m_testSourceResults)
        {
            if (input.empty() || CaseInsensitiveEquals(input, testSourceResult.Query))
            {
                testSourceResult.AddResults(result.Matches, shared_from_this());
            }
        }

        return result;
    }

    void WorkflowTestSource::AddResult(const TestSourceResult& testSourceResult)
    {
        m_testSourceResults.push_back(testSourceResult);
    }

    std::shared_ptr<WorkflowTestSource> CreateTestSource(std::vector<TestSourceResult>&& testSourceResults)
    {
        return std::make_shared<WorkflowTestSource>(std::move(testSourceResults));
    }

    TestContext::TestContext(std::ostream& out, std::istream& in) : TestContext(out, in, false, std::make_shared<std::vector<WorkflowTaskOverride>>())
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

    TestContext::TestContext(std::ostream& out, std::istream& in, bool isClone, std::shared_ptr<std::vector<WorkflowTaskOverride>> overrides) :
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

    TestContext::~TestContext()
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

    void TestContext::Override(const WorkflowTaskOverride& wto)
    {
        m_overrides->emplace_back(wto);
    }

    std::unique_ptr<Context> TestContext::CreateSubContext()
    {
        auto clone = std::make_unique<TestContext>(m_out, m_in, true, m_overrides);
        clone->SetFlags(this->GetFlags());
        CopyArgsToSubContext(clone.get());
        return clone;
    }

    void OverrideForOpenSource(TestContext& context, std::shared_ptr<WorkflowTestSource> testSource, bool overrideOpenCompositeSource)
    {
        context.Override({ "OpenSource", [=](TestContext& context)
        {
            context.Add<Execution::Data::Source>(Source{ testSource });
        } });

        if (overrideOpenCompositeSource)
        {
            context.Override({ "OpenCompositeSource", [](TestContext&)
            {
            } });
        }
    }

    void OverrideForCompositeInstalledSource(TestContext& context, std::shared_ptr<WorkflowTestSource> testSource)
    {
        context.Override({ "OpenSource", [](TestContext&)
        {
        } });

        context.Override({ "OpenCompositeSource", [=](TestContext& context)
        {
            context.Add<Execution::Data::Source>(Source{ testSource });
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

    void OverrideForShellExecute(TestContext& context, int expectedUseCount)
    {
        OverrideForCheckExistingInstaller(context);

        context.Override({ DownloadInstallerFile, [](TestContext& context)
        {
            context.Add<Data::DownloadHashInfo>({ {}, {} });
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
            context.Add<Data::DownloadHashInfo>({ {}, {} });
            context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe"));

            auto dependency = Dependency(DependencyType::Package, context.Get<Execution::Data::Manifest>().Id, context.Get<Execution::Data::Manifest>().Version);
            installationLog.push_back(dependency);
        } });

        context.Override({ RenameDownloadedInstaller, [](TestContext&)
        {
        } });

        OverrideForUpdateInstallerMotw(context);
    }

    void OverrideForPortableInstall(TestContext& context)
    {
        context.Override({ Workflow::details::PortableInstall, [](TestContext&)
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
            context.Add<Data::DownloadHashInfo>({ {}, {} });
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
            context.Add<Data::DownloadHashInfo>({ fileHash, DownloadResult{ fileHash } });
        } });

        context.Override({ RenameDownloadedInstaller, [](TestContext&)
        {
        } });

        OverrideForUpdateInstallerMotw(context);
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

    void OverrideForMSIX(TestContext& context)
    {
        context.Override({ Workflow::details::MsixInstall, [](TestContext& context)
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

    void OverrideOpenDependencySource(TestContext& context)
    {
        context.Override({ Workflow::OpenDependencySource, [](TestContext& context)
        {
            context.Add<Execution::Data::DependencySource>(Source{ std::make_shared<DependenciesTestSource>() });
        } });
    }

    void OverrideEnableWindowsFeaturesDependencies(TestContext& context)
    {
        context.Override({ Workflow::EnableWindowsFeaturesDependencies, [](TestContext&)
        {
        } });
    }

    void OverrideRegisterStartupAfterReboot(TestContext& context)
    {
        context.Override({ "RegisterStartupAfterReboot", [](TestContext&)
        {
        } });
    }

    void OverrideDownloadInstallerFileForMSStoreDownload(TestContext& context)
    {
        context.Override({ DownloadInstallerFile, [](TestContext& context)
        {
            const auto& installer = context.Get<Data::Installer>().value();
            const auto& installerPath = context.Get<Data::InstallerPath>();
            std::ofstream file(installerPath, std::ofstream::out | std::ofstream::trunc);
            file << installer.Url;
            file.close();
            context.Add<Data::DownloadHashInfo>({ {}, {} });
        } });
    }
}
