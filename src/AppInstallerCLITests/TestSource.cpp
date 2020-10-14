// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestSource.h"

namespace TestCommon
{
    // An ISource implementation for use across the test code.
    struct TestSource : public ISource
    {
        struct TestPackageVersion : public IPackageVersion
        {
            TestPackageVersion(const Manifest& manifest) : m_manifest(manifest) {}

            LocIndString GetProperty(PackageVersionProperty property) const override
            {
                switch (property)
                {
                case PackageVersionProperty::Id:
                    return LocIndString{ m_manifest.Id };
                case PackageVersionProperty::Name:
                    return LocIndString{ m_manifest.Name };
                case PackageVersionProperty::Version:
                    return LocIndString{ m_manifest.Version };
                case PackageVersionProperty::Channel:
                    return LocIndString{ m_manifest.Channel };
                default:
                    return {};
                }
            }

            std::vector<LocIndString> GetMultiProperty(PackageVersionMultiProperty property) const override
            {
                switch (property)
                {
                case PackageVersionMultiProperty::PackageFamilyName:
                case PackageVersionMultiProperty::ProductCode:
                default:
                    return {};
                }
            }

            Manifest GetManifest() const override
            {
                return m_manifest;
            }

            std::map<std::string, std::string> GetInstallationMetadata() const override
            {
                return {};
            }

            Manifest m_manifest;
        };

        struct TestPackage : public IPackage
        {
            TestPackage(const Manifest& manifest) : m_manifest(manifest) {}

            LocIndString GetProperty(PackageProperty property) const override
            {
                switch (property)
                {
                case PackageProperty::Id:
                    return LocIndString{ m_manifest.Id };
                case PackageProperty::Name:
                    return LocIndString{ m_manifest.Name };
                default:
                    return {};
                }
            }

            std::shared_ptr<IPackageVersion> GetInstalledVersion() const override
            {
                return {};
            }

            std::vector<PackageVersionKey> GetAvailableVersionKeys() const override
            {
                return { { "", m_manifest.Version, m_manifest.Channel } };
            }

            std::shared_ptr<IPackageVersion> GetLatestAvailableVersion() const override
            {
                return std::make_shared<TestPackageVersion>(m_manifest);
            }

            std::shared_ptr<IPackageVersion> GetAvailableVersion(const PackageVersionKey& versionKey) const override
            {
                if ((versionKey.Version.empty() || versionKey.Version == m_manifest.Version) &&
                    (versionKey.Channel.empty() || versionKey.Channel == m_manifest.Channel))
                {
                    return std::make_shared<TestPackageVersion>(m_manifest);
                }
                else
                {
                    return {};
                }
            }

            bool IsUpdateAvailable() const override
            {
                return false;
            }

            Manifest m_manifest;
        };

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
                        std::make_unique<TestPackage>(manifest),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnOne")));
            }
            else if (input == "TestQueryReturnTwo")
            {
                auto manifest = YamlParser::CreateFromPath(TestDataFile("InstallFlowTest_Exe.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        std::make_unique<TestPackage>(manifest),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));

                auto manifest2 = YamlParser::CreateFromPath(TestDataFile("Manifest-Good.yaml"));
                result.Matches.emplace_back(
                    ResultMatch(
                        std::make_unique<TestPackage>(manifest2),
                        PackageMatchFilter(PackageMatchField::Id, MatchType::Exact, "TestQueryReturnTwo")));
            }

            return result;
        }

        const SourceDetails& GetDetails() const override { THROW_HR(E_NOTIMPL); }

        const std::string& GetIdentifier() const override { THROW_HR(E_NOTIMPL); }
    };
}
