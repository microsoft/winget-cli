#pragma once

#include "pch.h"
#include "TestSource.h"
#include "TestCommon.h"
#include "AppInstallerVersions.h"
#include <Public/winget/RepositorySource.h>
#include <winget/ManifestYamlParser.h>

using namespace AppInstaller::Repository;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Utility;

namespace TestCommon
{
    namespace 
    {
        Manifest CreateFakeManifestWithDependencies(std::string input)
        {
            auto manifest = YamlParser::CreateFromPath(TestDataFile("Installer_Exe_Dependencies.yaml"));
            manifest.Id = input;
            manifest.Moniker = input;

            auto& installer = manifest.Installers.at(0);
            installer.ProductId = input;
            installer.Dependencies.Clear();

            string_t defaultFakeVersion("0.0.1-defaultFakeVersion");

            if (input == "withoutInstallers")
            {
                manifest.Installers.clear();
                return manifest;
            }

            /*
            * Dependencies:
            *   "A": Depends on the test
            *   B: NoDependency
            *   C: B
            *   D: E
            *   E: D
            *   F: B
            *   G: C
            *   H: G, B
            *
            *   installed1
            *   minVersion1.0
            *   minVersion1.5
            *   requires1.5: minVersion1.5
            *   minVersion2.0 //invalid version (not returned as result)
            */

            //-- predefined
            if (input == "C")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "B", defaultFakeVersion));
            }
            if (input == "D")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "E", defaultFakeVersion));
            }
            if (input == "E")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "D", defaultFakeVersion));
            }
            if (input == "F")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "B", defaultFakeVersion));
            }
            if (input == "G")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "C", defaultFakeVersion));
            }
            if (input == "H")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "G", defaultFakeVersion));
                installer.Dependencies.Add(Dependency(DependencyType::Package, "B", defaultFakeVersion));
            }
            if (input == "installed1")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "installed1Dep", defaultFakeVersion));
            }
            if (input == "minVersion1.0")
            {
                manifest.Id = "minVersion";
                manifest.Version = "1.0";
            }
            if (input == "minVersion1.5")
            {
                manifest.Id = "minVersion";
                manifest.Version = "1.5";
            }
            if (input == "requires1.5")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "minVersion", "1.5"));
            }

            // depends on test
            if (input == "StackOrderIsOk")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "C", defaultFakeVersion));
            }
            if (input == "NeedsToInstallBFirst")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "B", defaultFakeVersion));
                installer.Dependencies.Add(Dependency(DependencyType::Package, "C", defaultFakeVersion));
            }
            if (input == "EasyToSeeLoop")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "D", defaultFakeVersion));
            }
            if (input == "DependencyAlreadyInStackButNoLoop")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "C", defaultFakeVersion));
                installer.Dependencies.Add(Dependency(DependencyType::Package, "F", defaultFakeVersion));
            }
            if (input == "PathBetweenBranchesButNoLoop")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "C", defaultFakeVersion));
                installer.Dependencies.Add(Dependency(DependencyType::Package, "H", defaultFakeVersion));
            }
            if (input == "DependenciesInstalled")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "installed1", defaultFakeVersion));
            }
            if (input == "DependenciesValidMinVersions")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "minVersion", "1.0"));
            }
            if (input == "DependenciesValidMinVersionsMultiple")
            {
                installer.Dependencies.Add(Dependency(DependencyType::Package, "minVersion", "1.0"));
                installer.Dependencies.Add(Dependency(DependencyType::Package, "requires1.5"));
            }

            return manifest;
        }
    }

    struct DependenciesTestSource : public TestSource
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
            else if (!request.Filters.empty())
            {
                input = request.Filters[0].Value;
            }

            bool installed = false;
            if (input == "installed1")
            {
                installed  = true;
            }

            if (input == "NoMatches")
            {
                return result;
            }

            Manifest manifest;
            if (input == "MultipleDependenciesFromManifest")
            {
                manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_MultipleDependencies.yaml"));
            }
            else
            {
                manifest = CreateFakeManifestWithDependencies(input);
            }

            //TODO:
            // test for installed packages and packages that need upgrades
            // test for different min Version of dependencies
            if (installed)
            {
                //auto manifest2 = YamlParser::CreateFromPath(TestDataFile("UpdateFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            manifest,
                            TestCompositePackage::MetadataMap{ { PackageVersionMetadata::InstalledType, "Exe" } },
                            std::vector<Manifest>{ manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, manifest.Id)));
            }
            else
            {
                result.Matches.emplace_back(
                    ResultMatch(
                        TestCompositePackage::Make(
                            std::vector<Manifest>{ manifest },
                            shared_from_this()
                        ),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::CaseInsensitive, manifest.Id)));
            }

            return result;
        }
    };

}
