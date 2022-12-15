// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WorkflowCommon.h"

SearchResult WorkflowTestSource::Search(const SearchRequest& request) const
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

SearchResult WorkflowTestCompositeSource::Search(const SearchRequest& request) const
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

    if (input.empty() || input == "TestExeInstallerWithUnknownVersion")
    {
        auto installed = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_UnknownVersion.yaml"));
        auto available = installed;
        // Override the installed version to be unknown.
        installed.Version = "unknown";
        result.Matches.emplace_back(
            ResultMatch(
                TestPackage::Make(
                    installed,
                    TestPackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                    std::vector<Manifest>{ available },
                    shared_from_this()
                ),
                PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "AppInstallerCliTest.TestExeUnknownVersion")));
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

~TestContext::TestContext()
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
    return clone;
}

void OverrideForOpenSource(TestContext& context, bool overrideOpenCompositeSource = false)
{
    context.Override({ "OpenSource", [](TestContext& context)
    {
        context.Add<Execution::Data::Source>(Source{ std::make_shared<WorkflowTestSource>() });
    } });

    if (overrideOpenCompositeSource)
    {
        context.Override({ "OpenCompositeSource", [](TestContext&)
        {
        } });
    }
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

void OverrideForShellExecute(TestContext& context, int expectedUseCount)
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
    } });

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
    context.Override({ GetUninstallInfo, [](TestContext&)
    {
    } });

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
