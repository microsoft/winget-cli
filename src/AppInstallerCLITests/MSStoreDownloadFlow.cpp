// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestHooks.h"
#include "TestRestRequestHandler.h"
#include "WorkflowCommon.h"
#include <AppInstallerStrings.h>
#include <AppInstallerSHA256.h>
#include <winget/JsonUtil.h>
#include <Commands/DownloadCommand.h>

using namespace TestCommon;
using namespace AppInstaller::CLI;
using namespace AppInstaller::CLI::Execution;
using namespace AppInstaller::Settings;
using namespace AppInstaller::Utility::literals;

utility::string_t TestDisplayCatalogResponse = _XPLATSTR(
    R"delimiter(
    {
      "Product": {
        "DisplaySkuAvailabilities": [
          {
            "Sku": {
              "SkuId": "0015",
              "Properties": {
                "Packages": [
                  {
                    "PackageId": "PackageEnglish",
                    "Architectures": [ "x64", "arm" ],
                    "Languages": [ "en-US", "en-GB" ],
                    "PackageFormat": "Appx",
                    "ContentId": "LicenseContentId",
                    "FulfillmentData": {
                      "WuCategoryId": "TestCategoryIdEnglish"
                    }
                  },
                  {
                    "PackageId": "PackageFrench",
                    "Architectures": [ "x64", "arm" ],
                    "Languages": [ "fr-FR" ],
                    "PackageFormat": "Appx",
                    "ContentId": "LicenseContentId",
                    "FulfillmentData": {
                      "WuCategoryId": "TestCategoryIdFrench"
                    }
                  }
                ]
              }
            }
          }
        ]
      }
    })delimiter");

utility::string_t TestLicensingResponseRaw = _XPLATSTR(
    R"delimiter(
    {
      "license": {
        "keys": [
          {
            "value": "<LicenseContent>"
          }
        ]
      }
    })delimiter");

std::string LicenseContent = "TestLicense";

utility::string_t TestLicensingResponse = AppInstaller::Utility::ReplaceWhileCopying(
    TestLicensingResponseRaw, L"<LicenseContent>",
    AppInstaller::Utility::ConvertToUTF16(AppInstaller::JSON::Base64Encode(std::vector<BYTE>{ LicenseContent.begin(), LicenseContent.end() })));

utility::string_t TestDisplayCatalogResponse_TargetSkuNotFound = _XPLATSTR(
    R"delimiter(
    {
      "Product": {
        "DisplaySkuAvailabilities": [
          {
            "Sku": {
              "SkuId": "0011",
              "Properties": {
                "Packages": [
                  {
                    "PackageId": "PackageEnglish",
                    "Architectures": [ "x64", "arm" ],
                    "Languages": [ "en-US", "en-GB" ],
                    "PackageFormat": "Appx",
                    "ContentId": "LicenseContentId",
                    "FulfillmentData": {
                      "WuCategoryId": "TestCategoryIdEnglish"
                    }
                  }
                ]
              }
            }
          }
        ]
      }
    })delimiter");

std::vector<SFS::AppContent> GetSfsAppContentsOverrideFunction(std::string_view wuCategoryId)
{
    std::string wuCategoryIdStr{ wuCategoryId };

    std::vector<SFS::AppContent> result;

    std::unique_ptr<SFS::ContentId> contentId;
    std::vector<SFS::AppPrerequisiteContent> dependencies;
    std::vector<SFS::AppFile> packages;
    std::unique_ptr<SFS::AppContent> appContent;

    std::vector<BYTE> sha256Bytes = AppInstaller::Utility::SHA256::ConvertToBytes("69D84CA8899800A5575CE31798223CD4FEBAB1D734A07C2E51E56A28E0DF8123");
    std::string base64EncodedSha256 = AppInstaller::JSON::Base64Encode(sha256Bytes);

    {
        // Create dependencies content
        std::unique_ptr<SFS::ContentId> dependencyContentId;
        std::vector<SFS::AppFile> dependencyPackages;
        std::unique_ptr<SFS::AppPrerequisiteContent> dependencyContent;

        std::ignore = SFS::ContentId::Make("testDependency", "testDependency", "1.0.0.0", dependencyContentId);

        std::unique_ptr<SFS::AppFile> dependencyX64;
        std::ignore = SFS::AppFile::Make(
            wuCategoryIdStr + ".appx",
            "https://NotUsed/" + wuCategoryIdStr + "/dependency/x64",
            100,
            { { SFS::HashType::Sha256, base64EncodedSha256 } },
            { SFS::Architecture::Amd64 },
            { "Universal=10.0.0.0" },
            wuCategoryIdStr + ".Dependency_1.2.3.4_x64__8wekyb3d8bbwe",
            dependencyX64);
        dependencyPackages.emplace_back(std::move(*dependencyX64));

        std::unique_ptr<SFS::AppFile> dependencyArm;
        std::ignore = SFS::AppFile::Make(
            wuCategoryIdStr + ".appx",
            "https://NotUsed/" + wuCategoryIdStr + "/dependency/arm",
            100,
            { { SFS::HashType::Sha256, base64EncodedSha256 } },
            { SFS::Architecture::Arm },
            { "Universal=10.0.0.0" },
            wuCategoryIdStr + ".Dependency_1.2.3.4_arm__8wekyb3d8bbwe",
            dependencyArm);
        dependencyPackages.emplace_back(std::move(*dependencyArm));

        std::ignore = SFS::AppPrerequisiteContent::Make(std::move(dependencyContentId), std::move(dependencyPackages), dependencyContent);

        dependencies.emplace_back(std::move(*dependencyContent));
    }

    {
        // Create main packages

        // Good candidate x64
        std::unique_ptr<SFS::AppFile> packageX64;
        std::ignore = SFS::AppFile::Make(
            wuCategoryIdStr + ".appx",
            "https://NotUsed/" + wuCategoryIdStr + "/x64",
            100,
            { { SFS::HashType::Sha256, base64EncodedSha256 } },
            { SFS::Architecture::Amd64 },
            { "Desktop=10.0.0.0" },
            wuCategoryIdStr + "_1.0.0.0_x64__8wekyb3d8bbwe",
            packageX64);
        packages.emplace_back(std::move(*packageX64));

        // Good candidate arm
        std::unique_ptr<SFS::AppFile> packageArm;
        std::ignore = SFS::AppFile::Make(
            wuCategoryIdStr + ".appx",
            "https://NotUsed/" + wuCategoryIdStr + "/arm",
            100,
            { { SFS::HashType::Sha256, base64EncodedSha256 } },
            { SFS::Architecture::Arm },
            { "Desktop=10.0.0.0" },
            wuCategoryIdStr + "_1.0.0.0_arm__8wekyb3d8bbwe",
            packageArm);
        packages.emplace_back(std::move(*packageArm));

        // Good candidate IoT
        std::unique_ptr<SFS::AppFile> packageIoT;
        std::ignore = SFS::AppFile::Make(
            wuCategoryIdStr + ".appx",
            "https://NotUsed/" + wuCategoryIdStr + "/IoT/arm",
            100,
            { { SFS::HashType::Sha256, base64EncodedSha256 } },
            { SFS::Architecture::Arm },
            { "IoT=10.0.0.0" },
            wuCategoryIdStr + ".IoT_1.0.0.0_arm__8wekyb3d8bbwe",
            packageIoT);
        packages.emplace_back(std::move(*packageIoT));

        // Good candidate IoT has newer version
        std::unique_ptr<SFS::AppFile> packageIoT2;
        std::ignore = SFS::AppFile::Make(
            wuCategoryIdStr + ".appx",
            "https://NotUsed/" + wuCategoryIdStr + "/IoT/arm/2.0",
            100,
            { { SFS::HashType::Sha256, base64EncodedSha256 } },
            { SFS::Architecture::Arm },
            { "IoT=10.0.0.0" },
            wuCategoryIdStr + ".IoT_2.0.0.0_arm__8wekyb3d8bbwe",
            packageIoT2);
        packages.emplace_back(std::move(*packageIoT2));

        // Candidate unsupported platform
        std::unique_ptr<SFS::AppFile> packageXbox;
        std::ignore = SFS::AppFile::Make(
            wuCategoryIdStr + ".appx",
            "https://NotUsed/" + wuCategoryIdStr + "/Xbox/arm",
            100,
            { { SFS::HashType::Sha256, base64EncodedSha256 } },
            { SFS::Architecture::Arm },
            { "Xbox=10.0.0.0" },
            wuCategoryIdStr + ".Xbox_1.0.0.0_arm__8wekyb3d8bbwe",
            packageXbox);
        packages.emplace_back(std::move(*packageXbox));

        // Candidate unsupported filetype
        std::unique_ptr<SFS::AppFile> packageData;
        std::ignore = SFS::AppFile::Make(
            wuCategoryIdStr + ".cab",
            "https://NotUsed/" + wuCategoryIdStr + "/cab",
            100,
            { { SFS::HashType::Sha256, base64EncodedSha256 } },
            { SFS::Architecture::Arm },
            { "Desktop=10.0.0.0" },
            wuCategoryIdStr + ".Data_1.0.0.0_arm__8wekyb3d8bbwe",
            packageData);
        packages.emplace_back(std::move(*packageData));
    }

    std::ignore = SFS::ContentId::Make("test", "test", "1.0.0.0", contentId);
    std::ignore = SFS::AppContent::Make(std::move(contentId), "updateId", std::move(dependencies), std::move(packages), appContent);

    result.emplace_back(std::move(*appContent));

    return result;
}

TEST_CASE("MSStoreDownloadFlow_Success", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideDownloadInstallerFileForMSStoreDownload(context);
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    INFO(downloadOutput.str());

    // Verify downloaded files
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_X64.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_X64.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish.IoT_2.0.0.0_IoT_Arm.appx"));

    // Verify license
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml"));
    std::ifstream licenseFile(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml");
    REQUIRE(licenseFile.is_open());
    std::string licenseFileStr;
    std::getline(licenseFile, licenseFileStr);
    REQUIRE(licenseFileStr == LicenseContent);

    // Verify unsupported packages filtered out
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish.IoT_1.0.0.0_IoT_Arm.appx"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish.Xbox_1.0.0.0_Xbox_Arm.appx"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish.Data_1.0.0.0_Desktop_Arm.cab"));
}

TEST_CASE("MSStoreDownloadFlow_Success_SkipDependencies", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideDownloadInstallerFileForMSStoreDownload(context);
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);
    context.Args.AddArg(Execution::Args::Type::SkipDependencies);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    INFO(downloadOutput.str());

    // Verify downloaded files
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath()));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_X64.appx"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_X64.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish.IoT_2.0.0.0_IoT_Arm.appx"));

    // Verify license
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml"));
    std::ifstream licenseFile(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml");
    REQUIRE(licenseFile.is_open());
    std::string licenseFileStr;
    std::getline(licenseFile, licenseFileStr);
    REQUIRE(licenseFileStr == LicenseContent);
}

TEST_CASE("MSStoreDownloadFlow_Success_SkipLicense", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideDownloadInstallerFileForMSStoreDownload(context);
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);
    context.Args.AddArg(Execution::Args::Type::SkipMicrosoftStorePackageLicense);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    INFO(downloadOutput.str());

    // Verify downloaded files
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_X64.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_X64.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish.IoT_2.0.0.0_IoT_Arm.appx"));

    // Verify license
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml"));
}

TEST_CASE("MSStoreDownloadFlow_Success_SpecificLocale", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideDownloadInstallerFileForMSStoreDownload(context);
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "fr-FR"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    INFO(downloadOutput.str());

    // Verify downloaded files
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdFrench.Dependency_1.2.3.4_Universal_X64.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdFrench.Dependency_1.2.3.4_Universal_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdFrench_1.0.0.0_Desktop_X64.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdFrench_1.0.0.0_Desktop_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdFrench.IoT_2.0.0.0_IoT_Arm.appx"));

    // Verify license
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml"));
    std::ifstream licenseFile(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml");
    REQUIRE(licenseFile.is_open());
    std::string licenseFileStr;
    std::getline(licenseFile, licenseFileStr);
    REQUIRE(licenseFileStr == LicenseContent);
}

TEST_CASE("MSStoreDownloadFlow_Success_SpecificArchitecture", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideDownloadInstallerFileForMSStoreDownload(context);
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);
    context.Args.AddArg(Execution::Args::Type::InstallerArchitecture, "x64"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    INFO(downloadOutput.str());

    // Verify downloaded files
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_x64.appx"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_X64.appx"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_Arm.appx"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish.IoT_2.0.0.0_IoT_Arm.appx"));

    // Verify license
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml"));
    std::ifstream licenseFile(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml");
    REQUIRE(licenseFile.is_open());
    std::string licenseFileStr;
    std::getline(licenseFile, licenseFileStr);
    REQUIRE(licenseFileStr == LicenseContent);
}

TEST_CASE("MSStoreDownloadFlow_Success_SpecificPlatform", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideDownloadInstallerFileForMSStoreDownload(context);
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);
    context.Args.AddArg(Execution::Args::Type::Platform, "Windows.IoT"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE(context.GetTerminationHR() == S_OK);
    INFO(downloadOutput.str());

    // Verify downloaded files
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath()));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_X64.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"Dependencies" / L"TestCategoryIdEnglish.Dependency_1.2.3.4_Universal_Arm.appx"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_X64.appx"));
    REQUIRE_FALSE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish_1.0.0.0_Desktop_Arm.appx"));
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"TestCategoryIdEnglish.IoT_2.0.0.0_IoT_Arm.appx"));

    // Verify license
    REQUIRE(std::filesystem::exists(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml"));
    std::ifstream licenseFile(tempDirectory.GetPath() / L"9WZDNCRFJ364_License.xml");
    REQUIRE(licenseFile.is_open());
    std::string licenseFileStr;
    std::getline(licenseFile, licenseFileStr);
    REQUIRE(licenseFileStr == LicenseContent);
}

TEST_CASE("MSStoreDownloadFlow_Fail_TargetSkuNotFound", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse_TargetSkuNotFound));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE);
    INFO(downloadOutput.str());
}

TEST_CASE("MSStoreDownloadFlow_Fail_LocaleNotApplicable", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "ja-JP"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE);
    INFO(downloadOutput.str());
}

TEST_CASE("MSStoreDownloadFlow_Fail_ArchitectureNotApplicable", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);
    context.Args.AddArg(Execution::Args::Type::InstallerArchitecture, "arm64"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_DISPLAYCATALOG_PACKAGE);
    INFO(downloadOutput.str());
}

TEST_CASE("MSStoreDownloadFlow_Fail_PlatformNotApplicable", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestLicensingResponse));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);
    context.Args.AddArg(Execution::Args::Type::Platform, "Windows.Holographic"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_NO_APPLICABLE_SFSCLIENT_PACKAGE);
    INFO(downloadOutput.str());
}

TEST_CASE("MSStoreDownloadFlow_Fail_Licensing", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideDownloadInstallerFileForMSStoreDownload(context);
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::InternalError));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE_TERMINATED_WITH(context, MAKE_HRESULT(SEVERITY_ERROR, FACILITY_HTTP, web::http::status_codes::InternalError));
    INFO(downloadOutput.str());
}

TEST_CASE("MSStoreDownloadFlow_Fail_Licensing_Forbidden", "[MSStoreDownloadFlow][workflow]")
{
    TestCommon::TempDirectory tempDirectory("TestDownloadDirectory", false);

    std::ostringstream downloadOutput;
    TestContext context{ downloadOutput, std::cin };
    auto previousThreadGlobals = context.SetForCurrentThread();
    OverrideDownloadInstallerFileForMSStoreDownload(context);
    TestHook::SetDisplayCatalogHttpPipelineStage_Override displayCatalogOverride(GetTestRestRequestHandler(web::http::status_codes::OK, TestDisplayCatalogResponse));
    TestHook::SetSfsClientAppContents_Override sfsClientOverride({ &GetSfsAppContentsOverrideFunction });
    TestHook::SetLicensingHttpPipelineStage_Override licensingOverride(GetTestRestRequestHandler(web::http::status_codes::Forbidden));
    context.Args.AddArg(Execution::Args::Type::Manifest, TestDataFile("DownloadFlowTest_MSStore.yaml").GetPath().u8string());
    context.Args.AddArg(Execution::Args::Type::DownloadDirectory, tempDirectory);
    context.Args.AddArg(Execution::Args::Type::Locale, "en-US"sv);

    DownloadCommand download({});
    download.Execute(context);
    REQUIRE_TERMINATED_WITH(context, APPINSTALLER_CLI_ERROR_LICENSING_API_FAILED_FORBIDDEN);
    INFO(downloadOutput.str());
}
