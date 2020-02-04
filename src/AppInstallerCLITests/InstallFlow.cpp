// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Manifest/Manifest.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerStrings.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/ShellExecuteInstallerHandler.h"
#include "Workflows/MsixInstallerHandler.h"

using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Management::Deployment;
using namespace TestCommon;
using namespace AppInstaller::Workflow;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

class MsixInstallerHandlerTest : public MsixInstallerHandler
{
public:
    MsixInstallerHandlerTest(
        const ManifestInstaller& manifestInstaller,
        WorkflowReporter& reporter) : MsixInstallerHandler(manifestInstaller, reporter) {};

protected:

    std::future<void> ExecuteInstallerAsync(const Uri& uri) override
    {
        std::ofstream file("TestMsixInstalled.txt", std::ofstream::out);

        file << AppInstaller::Utility::ConvertToUTF8(uri.ToString());

        file.close();

        co_return;
    }
};

class ShellExecuteInstallerHandlerTest : public ShellExecuteInstallerHandler
{
public:
    ShellExecuteInstallerHandlerTest(
        const ManifestInstaller& manifestInstaller,
        WorkflowReporter& reporter) : ShellExecuteInstallerHandler(manifestInstaller, reporter) {};

    void Download() override
    {
        this->m_downloadedInstaller = TestDataFile("AppInstallerTestExeInstaller.exe");
    }

    void RenameDownloadedInstaller() override {};
};

class InstallFlowTest : public InstallFlow
{
public:
    InstallFlowTest(Manifest manifest, std::ostream& outStream, std::istream& inStream) :
        InstallFlow(manifest, outStream, inStream) {}

protected:
    std::unique_ptr<InstallerHandlerBase> GetInstallerHandler() override
    {
        switch (m_selectedInstaller.InstallerType)
        {
        case ManifestInstaller::InstallerTypeEnum::Exe:
            return std::make_unique<ShellExecuteInstallerHandlerTest>(m_selectedInstaller, m_reporter);
        case ManifestInstaller::InstallerTypeEnum::Msix:
            return std::make_unique<MsixInstallerHandlerTest>(m_selectedInstaller, m_reporter);
        default:
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }
};

TEST_CASE("ExeInstallFlowWithTestManifest", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled", ".txt", true, false, true);

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yml"));

    std::ostringstream installOutput;
    InstallFlowTest testFlow(manifest, installOutput, std::cin);
    testFlow.Install();
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/default") != std::string::npos);
}

TEST_CASE("InstallFlowWithNonApplicableArchitecture", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestExeInstalled", ".txt", true, false, true);

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_NoApplicableArchitecture.yml"));

    std::ostringstream installOutput;
    InstallFlowTest testFlow(manifest, installOutput, std::cin);
    REQUIRE_THROWS_WITH(testFlow.Install(), Catch::Contains("No installer with applicable architecture found."));
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(!std::filesystem::exists(installResultPath.GetPath()));
}

TEST_CASE("MsixInstallFlow_DownloadFlow", "[InstallFlow]")
{
    TestCommon::TempFile installResultPath("TestMsixInstalled", ".txt", true, false, true);

    // Todo: point to files from our repo when the repo goes public
    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Msix_DownloadFlow.yml"));

    std::ostringstream installOutput;
    InstallFlowTest testFlow(manifest, installOutput, std::cin);
    testFlow.Install();
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
    TestCommon::TempFile installResultPath("TestMsixInstalled", ".txt", true, false, true);

    // Todo: point to files from our repo when the repo goes public
    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Msix_StreamingFlow.yml"));

    std::ostringstream installOutput;
    InstallFlowTest testFlow(manifest, installOutput, std::cin);
    testFlow.Install();
    INFO(installOutput.str());

    // Verify Installer is called and a local file is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath.GetPath()));
    std::ifstream installResultFile(installResultPath.GetPath());
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("https://") != std::string::npos);
}