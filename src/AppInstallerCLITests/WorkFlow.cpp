// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"
#include <AppInstallerErrors.h>
#include <AppInstallerLogging.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerStrings.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/UninstallFlow.h>
#include <Workflows/UpdateFlow.h>
#include <Workflows/MSStoreInstallerHandler.h>
#include <Workflows/ShowFlow.h>
#include <Workflows/ShellExecuteInstallerHandler.h>
#include <Workflows/WorkflowBase.h>
#include <Public/AppInstallerRepositorySource.h>
#include <Public/AppInstallerRepositorySearch.h>
#include <Commands/InstallCommand.h>
#include <Commands/ShowCommand.h>
#include <Commands/UninstallCommand.h>
#include <Commands/UpgradeCommand.h>
#include <winget/LocIndependent.h>
#include <winget/ManifestYamlParser.h>
#include <Resources.h>
#include <AppInstallerFileLogger.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Logging;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;


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
                        TestPackage::Make(std::vector<Manifest>{ manifest }),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnOne")));
            }
            else if (input == "TestQueryReturnTwo")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(std::vector<Manifest>{ manifest }),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));

                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("Manifest-Good.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(std::vector<Manifest>{ manifest2 }),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));
            }

            return result;
        }
    };

    struct WorkflowTestCompositeSource : public TestSource
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

            // Empty query should return all exe, msix and msstore installer
            if (input.empty() || input == "AppInstallerCliTest.TestExeInstaller")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
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
                            std::vector<Manifest>{ manifest2, manifest }
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            if (input.empty() || input == "AppInstallerCliTest.TestMsixInstaller")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Msix_StreamingFlow.yaml"));
                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Msix.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Msix" } },
                            std::vector<Manifest>{ manifest2, manifest }
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestMsixInstaller")));
            }

            if (input.empty() || input == "AppInstallerCliTest.TestMSStoreInstaller")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_MSStore.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestPackage::Make(
                            manifest,
                            TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "MSStore" } },
                            std::vector<Manifest>{ manifest }
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
                            std::vector<Manifest>{ manifest2, manifest }
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
                            std::vector<Manifest>{ manifest2, manifest }
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeInstaller")));
            }

            return result;
        }
    };

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
        TestContext(std::ostream& out, std::istream& in) : m_out(out), m_in(in), Context(out, in)
        {
            m_overrides = std::make_shared<std::vector<WorkflowTaskOverride>>();

            WorkflowTaskOverride wto
            { RemoveInstaller, [](TestContext&)
                {
                    // Do nothing; we never want to remove the test files.
            } };

            // Mark this one as used so that it doesn't anger the destructor.
            wto.Used = true;

            Override(wto);
        }

        // For clone
        TestContext(std::ostream& out, std::istream& in, std::shared_ptr<std::vector<WorkflowTaskOverride>> overrides) :
            m_out(out), m_in(in), m_overrides(overrides), m_isClone(true), Context(out, in) {}

        ~TestContext()
        {
            if (!m_isClone)
            {
                for (const auto& wto : *m_overrides)
                {
                    if (!wto.Used)
                    {
                        FAIL("Unused override");
                    }
                }
            }
        }

        bool ShouldExecuteWorkflowTask(const Workflow::WorkflowTask& task) override
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
        }

        void Override(const WorkflowTaskOverride& wto)
        {
            m_overrides->emplace_back(wto);
        }

        std::unique_ptr<Context> Clone() override
        {
            auto clone = std::make_unique<TestContext>(m_out, m_in, m_overrides);
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
    context.Override({ Workflow::OpenSource, [](TestContext& context)
    {
        context.Add<Execution::Data::Source>(std::make_shared<WorkflowTestSource>());
    } });
}

void OverrideForCompositeInstalledSource(TestContext& context)
{
    context.Override({ Workflow::OpenSource, [](TestContext&)
    {
    } });

    context.Override({ "OpenCompositeSource", [](TestContext& context)
    {
        context.Add<Execution::Data::Source>(std::make_shared<WorkflowTestCompositeSource>());
    } });
}

void OverrideForUpdateInstallerMotw(TestContext& context)
{
    context.Override({ UpdateInstallerFileMotwIfApplicable, [](TestContext&)
    {
    } });
}

void OverrideForShellExecute(TestContext& context)
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

    context.Override({ "EnsureFeatureEnabled", [](TestContext&)
    {
    } });

    context.Override({ Workflow::EnsureStorePolicySatisfied, [](TestContext&)
    {
    } });
}

TEST_CASE("ExeInstallFlowWithTestManifest", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
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

TEST_CASE("InstallFlowWithNonApplicableArchitecture", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_NoApplicableArchitecture.yaml").GetPath().u8string());

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_INSTALLER);

    // Verify Installer was not called
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("MSStoreInstallFlowWithTestManifest", "[InstallFlow][workflow]")
{
    TestCommon::TempFile installResultPath("TestMSStoreInstalled.txt");

    std::ostringstream installOutput;
    TestContext context{ installOutput, std::cin };
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
    OverrideForMSIX(context);
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

TEST_CASE("ShellExecuteHandlerInstallerArgs", "[InstallFlow][workflow]")
{
    {
        std::ostringstream installOutput;
        TestContext context{ installOutput, std::cin };
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
    OverrideForOpenSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnTwo"sv);

    InstallCommand install({});
    install.Execute(context);
    INFO(installOutput.str());

    // Verify proper message is printed
    REQUIRE(installOutput.str().find(Resource::LocString(Resource::String::MultiplePackagesFound).get()) != std::string::npos);
}

TEST_CASE("InstallFlow_SearchAndShowAppInfo", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
    OverrideForOpenSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnOne"sv);

    ShowCommand show({});
    show.Execute(context);
    INFO(showOutput.str());

    // Verify AppInfo is printed
    REQUIRE(showOutput.str().find("AppInstallerCliTest.TestExeInstaller") != std::string::npos);
    REQUIRE(showOutput.str().find("AppInstaller Test Installer") != std::string::npos);
    REQUIRE(showOutput.str().find("1.0.0.0") != std::string::npos);
    REQUIRE(showOutput.str().find("https://ThisIsNotUsed") != std::string::npos);
}

TEST_CASE("InstallFlow_SearchAndShowAppVersion", "[ShowFlow][workflow]")
{
    std::ostringstream showOutput;
    TestContext context{ showOutput, std::cin };
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

TEST_CASE("UpdateFlow_UpdateWithManifest", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
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
}

TEST_CASE("UpdateFlow_UpdateMsix", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestMsixInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
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
    OverrideForCompositeInstalledSource(context);
    context.Args.AddArg(Execution::Args::Type::Query, "TestExeInstallerWithIncompatibleInstallerType"sv);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify Installer is not called.
    REQUIRE(!std::filesystem::exists(updateResultPath.GetPath()));
    REQUIRE(updateOutput.str().find(Resource::LocString(Resource::String::UpdateNotApplicable).get()) != std::string::npos);
    REQUIRE(context.GetTerminationHR() == APPINSTALLER_CLI_ERROR_UPDATE_NOT_APPLICABLE);
}

TEST_CASE("UpdateFlow_UpdateExeSpecificVersionNotFound", "[UpdateFlow][workflow]")
{
    TestCommon::TempFile updateResultPath("TestExeInstalled.txt");

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
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

    std::ostringstream updateOutput;
    TestContext context{ updateOutput, std::cin };
    OverrideForCompositeInstalledSource(context);
    OverrideForShellExecute(context);
    OverrideForMSIX(context);
    OverrideForMSStore(context, true);
    context.Args.AddArg(Execution::Args::Type::All);

    UpgradeCommand update({});
    update.Execute(context);
    INFO(updateOutput.str());

    // Verify installers are called.
    REQUIRE(std::filesystem::exists(updateExeResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMsixResultPath.GetPath()));
    REQUIRE(std::filesystem::exists(updateMSStoreResultPath.GetPath()));
}

TEST_CASE("UninstallFlow_UninstallExe", "[UninstallFlow][workflow]")
{
    TestCommon::TempFile uninstallResultPath("TestExeUninstalled.txt");

    std::ostringstream uninstallOutput;
    TestContext context{ uninstallOutput, std::cin };
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