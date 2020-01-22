// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "Manifest/Manifest.h"
#include "AppInstallerDownloader.h"
#include "Workflows/InstallFlow.h"

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
        this->m_downloadedInstaller = std::filesystem::current_path().append("AppInstallerTestExeInstaller.exe");
    }
};

TEST_CASE("InstallFlowWithTestManifest", "[InstallFlow]")
{
    auto installResultPath = std::filesystem::current_path().append("TestExeInstalled.txt");

    std::filesystem::remove(installResultPath);

    auto manifest = Manifest::CreateFromPath("InstallFlowTest.yml");

    InstallFlowTest testFlow(manifest, std::cout, std::cin);
    testFlow.Install();

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

    auto manifest = Manifest::CreateFromPath("InstallFlowTest_NoApplicableArchitecture.yml");

    InstallFlowTest testFlow(manifest, std::cout, std::cin);
    REQUIRE_THROWS_WITH(testFlow.Install(), Catch::Contains("No installer with applicable architecture found."));

    // Verify Installer is called and parameters are passed in.
    REQUIRE(!std::filesystem::exists(installResultPath));
}