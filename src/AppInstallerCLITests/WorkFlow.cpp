// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <AppInstallerLogging.h>
#include <Manifest/Manifest.h>
#include <AppInstallerDownloader.h>
#include <AppInstallerStrings.h>
#include <Workflows/InstallFlow.h>
#include <Workflows/ShowFlow.h>
#include <Workflows/ShellExecuteInstallerHandler.h>
#include <Workflows/WorkflowBase.h>
#include <Public/AppInstallerRepositorySource.h>
#include <Public/AppInstallerRepositorySearch.h>

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::CLI::Workflow;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository;
using namespace AppInstaller::Utility;

class MsixInstallerHandlerTest : public MsixInstallerHandler
{
public:
    MsixInstallerHandlerTest(
        const ManifestInstaller& manifestInstaller,
        Execution::Context& context) : MsixInstallerHandler(manifestInstaller, context) {};

protected:

    void ExecuteInstallerAsync(const Uri& uri) override
    {
        std::filesystem::path temp = std::filesystem::temp_directory_path();
        temp /= "TestMsixInstalled.txt";
        std::ofstream file(temp, std::ofstream::out);

        file << AppInstaller::Utility::ConvertToUTF8(uri.ToString());

        file.close();
    }
};

class ShellExecuteInstallerHandlerTest : public ShellExecuteInstallerHandler
{
public:
    ShellExecuteInstallerHandlerTest() = default;

    void RenameDownloadedInstaller(Context&) override {};

    std::string TestInstallerArgs(Context& context)
    {
        return ShellExecuteInstallerHandler::GetInstallerArgs(context);
    }
};

struct TestSource : public ISource
{
    struct TestApplication : public IApplication
    {
        TestApplication(const Manifest manifest) : m_manifest(manifest) {}

        std::optional<Manifest> GetManifest(const NormalizedString&, const NormalizedString&) override
        {
            return m_manifest;
        }

        std::string GetId() override
        {
            return m_manifest.Id;
        }

        std::string GetName() override
        {
            return m_manifest.Name;
        }

        std::vector<VersionAndChannel> GetVersions() override
        {
            std::vector<VersionAndChannel> result;
            result.emplace_back(Version(m_manifest.Version), Channel(m_manifest.Channel));
            return result;
        }

        Manifest m_manifest;
    };

    SearchResult Search(const SearchRequest& request) override
    {
        SearchResult result;
        if (request.Query.has_value())
        {
            if (request.Query.value().Value == "TestQueryReturnOne")
            {
                auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        std::make_unique<TestApplication>(manifest),
                        ApplicationMatchFilter(ApplicationMatchField::Id, MatchType::Exact, "TestQueryReturnOne")));
            }
            else if (request.Query.value().Value == "TestQueryReturnTwo")
            {
                auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        std::make_unique<TestApplication>(manifest),
                        ApplicationMatchFilter(ApplicationMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));

                auto manifest2 = Manifest::CreateFromPath(TestDataFile("Manifest-Good.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        std::make_unique<TestApplication>(manifest2),
                        ApplicationMatchFilter(ApplicationMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));
            }
        }
        return result;
    }

    virtual const SourceDetails& GetDetails() const override { THROW_HR(E_NOTIMPL); }
};

class InstallFlowTest : public InstallFlow
{
public:
    InstallFlowTest(Execution::Context& context) : InstallFlow(context) {}

protected:
    std::unique_ptr<InstallerHandlerBase> GetInstallerHandler() override
    {
        switch (m_selectedInstaller.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
            return std::make_unique<ShellExecuteInstallerHandlerTest>(m_selectedInstaller, m_contextRef);
        case ManifestInstaller::InstallerTypeEnum::Msix:
            return std::make_unique<MsixInstallerHandlerTest>(m_selectedInstaller, m_contextRef);
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void OpenIndexSource() override
    {
        m_contextRef.Add<Execution::Data::Source>(std::make_shared<TestSource>());
    }
};

class ShowFlowTest : public ShowFlow
{
public:
    ShowFlowTest(Execution::Context& context) : ShowFlow(context) {}

protected:

    void OpenIndexSource() override
    {
        m_contextRef.Add<Execution::Data::Source>(std::make_shared<TestSource>());
    }
};

TEST_CASE("ExeInstallFlowWithTestManifest", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    Execution::Context context{ installOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Exe.yaml").GetPath().u8string());

    context <<
        GetManifestFromArg <<
        SelectInstaller <<
        [](Context& context) { context.Add<Data::InstallerPath>(TestDataFile("AppInstallerTestExeInstaller.exe")); } <<
        [](Context& context) {  };

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

TEST_CASE("InstallFlowWithNonApplicableArchitecture", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    Execution::Context context{ installOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_NoApplicableArchitecture.yaml").GetPath().u8string());
    InstallFlowTest testFlow(context);
    REQUIRE_THROWS_WITH(testFlow.Execute(), Catch::Contains("No installer with applicable architecture found."));
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("MsixInstallFlow_DownloadFlow", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled.txt");

    std::ostringstream installOutput;
    Execution::Context context{ installOutput, std::cin };
    // Todo: point to files from our repo when the repo goes public
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Msix_DownloadFlow.yaml").GetPath().u8string());
    InstallFlowTest testFlow(context);
    testFlow.Execute();
    INFO(installOutput.str());

    // Verify Installer is called and a local file is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("file://") != std::string::npos);
}

TEST_CASE("MsixInstallFlow_StreamingFlow", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled.txt");

    std::ostringstream installOutput;
    Execution::Context context{ installOutput, std::cin };
    // Todo: point to files from our repo when the repo goes public
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("InstallFlowTest_Msix_StreamingFlow.yaml").GetPath().u8string());
    InstallFlowTest testFlow(context);
    testFlow.Execute();
    INFO(installOutput.str());

    // Verify Installer is called and a http address is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("https://") != std::string::npos);
}

TEST_CASE("ShellExecuteHandlerInstallerArgs", "[InstallFlow]")
{
    {
        std::ostringstream installOutput;
        Execution::Context context{ installOutput, std::cin };
        // Default Msi type with no args passed in, no switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Msi_NoSwitches.yaml"));
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), context);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/passive") != std::string::npos);
        REQUIRE(installerArgs.find("AppInstallerTestExeInstaller.exe.log") != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        Execution::Context context{ installOutput, std::cin };
        // Msi type with /silent and /log and /custom and /installlocation, no switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Msi_NoSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log");
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), context);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/quiet") != std::string::npos);
        REQUIRE(installerArgs.find("/log \"MyLog.log\"") != std::string::npos);
        REQUIRE(installerArgs.find("TARGETDIR=\"MyDir\"") != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        Execution::Context context{ installOutput, std::cin };
        // Msi type with /silent and /log and /custom and /installlocation, switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Msi_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log");
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), context);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mycustom") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
    }

    {
        std::ostringstream installOutput;
        Execution::Context context{ installOutput, std::cin };
        // Default Inno type with no args passed in, no switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Inno_NoSwitches.yaml"));
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), context);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/SILENT") != std::string::npos);
        REQUIRE(installerArgs.find("AppInstallerTestExeInstaller.exe.log") != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        Execution::Context context{ installOutput, std::cin };
        // Inno type with /silent and /log and /custom and /installlocation, no switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Inno_NoSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log");
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), context);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/VERYSILENT") != std::string::npos);
        REQUIRE(installerArgs.find("/LOG=\"MyLog.log\"") != std::string::npos);
        REQUIRE(installerArgs.find("/DIR=\"MyDir\"") != std::string::npos);
    }

    {
        std::ostringstream installOutput;
        Execution::Context context{ installOutput, std::cin };
        // Inno type with /silent and /log and /custom and /installlocation, switches specified in manifest
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log");
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), context);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs.find("/mysilent") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mylog=\"MyLog.log\"") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/mycustom") != std::string::npos); // Use declaration in manifest
        REQUIRE(installerArgs.find("/myinstalldir=\"MyDir\"") != std::string::npos); // Use declaration in manifest
    }

    {
        std::ostringstream installOutput;
        Execution::Context context{ installOutput, std::cin };
        // Override switch specified. The whole arg passed to installer is overrided.
        auto manifest = Manifest::CreateFromPath(TestDataFile("InstallerArgTest_Inno_WithSwitches.yaml"));
        context.Args.AddArg(Execution::Args::Type::Silent);
        context.Args.AddArg(Execution::Args::Type::Log, "MyLog.log");
        context.Args.AddArg(Execution::Args::Type::InstallLocation, "MyDir");
        context.Args.AddArg(Execution::Args::Type::Override, "/OverrideEverything");
        ShellExecuteInstallerHandlerTest testhandler(manifest.Installers.at(0), context);
        std::string installerArgs = testhandler.TestInstallerArgs();
        REQUIRE(installerArgs == "/OverrideEverything"); // Use value specified in override switch
    }
}

TEST_CASE("InstallFlow_SearchAndInstall", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled.txt");

    std::ostringstream installOutput;
    Execution::Context context{ installOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnOne");
    InstallFlowTest testFlow(context);
    testFlow.Execute();
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

TEST_CASE("InstallFlow_SearchFoundNoApp", "[InstallFlow]")
{
    std::ostringstream installOutput;
    Execution::Context context{ installOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnZero");
    InstallFlowTest testFlow(context);
    testFlow.Execute();
    INFO(installOutput.str());

    // Verify proper message is printed
    REQUIRE(installOutput.str().find("No app found matching input criteria.") != std::string::npos);
}

TEST_CASE("InstallFlow_SearchFoundMultipleApp", "[InstallFlow]")
{
    std::ostringstream installOutput;
    Execution::Context context{ installOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnTwo");
    InstallFlowTest testFlow(context);
    testFlow.Execute();
    INFO(installOutput.str());

    // Verify proper message is printed
    REQUIRE(installOutput.str().find("Multiple apps found matching input criteria. Please refine the input.") != std::string::npos);
}

TEST_CASE("InstallFlow_SearchAndShowAppInfo", "[ShowFlow]")
{
    std::ostringstream showOutput;
    Execution::Context context{ showOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnOne");
    ShowFlowTest testFlow(context);
    testFlow.Execute();
    INFO(showOutput.str());

    // Verify AppInfo is printed
    REQUIRE(showOutput.str().find("Id: AppInstallerCliTest.TestInstaller") != std::string::npos);
    REQUIRE(showOutput.str().find("Name: AppInstaller Test Installer") != std::string::npos);
    REQUIRE(showOutput.str().find("Version: 1.0.0.0") != std::string::npos);
    REQUIRE(showOutput.str().find("--Installer Download Url: https://ThisIsNotUsed") != std::string::npos);
}

TEST_CASE("InstallFlow_SearchAndShowAppVersion", "[ShowFlow]")
{
    std::ostringstream showOutput;
    Execution::Context context{ showOutput, std::cin };
    context.Args.AddArg(Execution::Args::Type::Query, "TestQueryReturnOne");
    context.Args.AddArg(Execution::Args::Type::ListVersions);
    ShowFlowTest testFlow(context);
    testFlow.Execute();
    INFO(showOutput.str());

    // Verify App version is printed
    REQUIRE(showOutput.str().find("1.0.0.0") != std::string::npos);
    // No manifest info is printed
    REQUIRE(showOutput.str().find("--Installer Download Url: https://ThisIsNotUsed") == std::string::npos);
}