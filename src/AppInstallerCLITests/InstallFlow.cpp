// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "Manifest/Manifest.h"
#include "AppInstallerDownloader.h"
#include "Workflows/InstallFlow.h"

using namespace TestCommon;
using namespace AppInstaller::Workflow;
using namespace AppInstaller::Utility;
using namespace AppInstaller::Manifest;

class InstallFlowTest : public InstallFlow
{
public:
    InstallFlowTest(Manifest manifest, std::ostream& outStream, std::istream& inStream) :
        InstallFlow(manifest, outStream, inStream) {}

protected:
    void DownloadInstaller() override
    {
        this->m_downloadedInstaller = TestDataFile("AppInstallerTestExeInstaller.exe");
    }
};

TEST_CASE("InstallFlowWithTestManifest", "[InstallFlow]")
{
    auto installResultPath = std::filesystem::current_path().append("TestExeInstalled.txt");

    std::filesystem::remove(installResultPath);

    auto manifest = Manifest::CreateFromPath(TestDataFile("InstallFlowTest.yml"));

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