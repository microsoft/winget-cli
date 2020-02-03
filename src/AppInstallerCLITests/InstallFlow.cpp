// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Manifest/Manifest.h"
#include "AppInstallerDownloader.h"
#include "AppInstallerStrings.h"
#include "Workflows/InstallFlow.h"
#include "Workflows/ExecutableInstallerHandler.h"
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

class ExecutableInstallerHandlerTest : public ExecutableInstallerHandler
{
public:
    ExecutableInstallerHandlerTest(
        const ManifestInstaller& manifestInstaller,
        WorkflowReporter& reporter) : ExecutableInstallerHandler(manifestInstaller, reporter) {};

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
        if (m_selectedInstaller.InstallerType == ManifestInstaller::InstallerTypeEnum::Exe)
        {
            return std::make_unique<ExecutableInstallerHandlerTest>(m_selectedInstaller, m_reporter);
        }
        else if (m_selectedInstaller.InstallerType == ManifestInstaller::InstallerTypeEnum::Msix)
        {
            return std::make_unique<MsixInstallerHandlerTest>(m_selectedInstaller, m_reporter);
        }
        else
        {
            THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }
};

TEST_CASE("ExeInstallFlowWithTestManifest", "[InstallFlow]")
{
    auto installResultPath = std::filesystem::current_path().append("TestExeInstalled.txt");

    std::filesystem::remove(installResultPath);

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yml"));

    std::ostringstream installOutput;
    InstallFlowTest testFlow(manifest, installOutput, std::cin);
    testFlow.Install();
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(std::filesystem::exists(installResultPath));
    std::ifstream installResultFile(installResultPath);
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("/default") != std::string::npos);
}

TEST_CASE("InstallFlowWithNonApplicableArchitecture", "[InstallFlow]")
{
    auto installResultPath = std::filesystem::current_path().append("TestExeInstalled.txt");

    std::filesystem::remove(installResultPath);

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_NoApplicableArchitecture.yml"));

    std::ostringstream installOutput;
    InstallFlowTest testFlow(manifest, installOutput, std::cin);
    REQUIRE_THROWS_WITH(testFlow.Install(), Catch::Contains("No installer with applicable architecture found."));
    INFO(installOutput.str());

    // Verify Installer is called and parameters are passed in.
    REQUIRE(!std::filesystem::exists(installResultPath));
}

TEST_CASE("MsixInstallFlow_DownloadFlow", "[InstallFlow]")
{
    auto installResultPath = std::filesystem::current_path().append("TestMsixInstalled.txt");

    std::filesystem::remove(installResultPath);

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Msix_DownloadFlow.yml"));

    std::ostringstream installOutput;
    InstallFlowTest testFlow(manifest, installOutput, std::cin);
    testFlow.Install();
    INFO(installOutput.str());

    // Verify Installer is called and a local file is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath));
    std::ifstream installResultFile(installResultPath);
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("file://") != std::string::npos);
}

TEST_CASE("MsixInstallFlow_StreamingFlow", "[InstallFlow]")
{
    auto installResultPath = std::filesystem::current_path().append("TestMsixInstalled.txt");

    std::filesystem::remove(installResultPath);

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest_Msix_StreamingFlow.yml"));

    std::ostringstream installOutput;
    InstallFlowTest testFlow(manifest, installOutput, std::cin);
    testFlow.Install();
    INFO(installOutput.str());

    // Verify Installer is called and a local file is used as package Uri.
    REQUIRE(std::filesystem::exists(installResultPath));
    std::ifstream installResultFile(installResultPath);
    REQUIRE(installResultFile.is_open());
    std::string installResultStr;
    std::getline(installResultFile, installResultStr);
    REQUIRE(installResultStr.find("https://") != std::string::npos);
}