// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "SQLiteIndexTestCommon.h"
#include "TestCommon.h"

#include <AppInstallerSHA256.h>
#include <AppInstallerStrings.h>

#include <algorithm>

using namespace std::string_literals;
using namespace AppInstaller::Manifest;
using namespace AppInstaller::Repository::Microsoft;
using namespace AppInstaller::Utility;

namespace TestCommon
{
    SQLiteIndex CreateTestIndex(const std::string& filePath, std::optional<SQLiteVersion> version)
    {
        // If no specific version requested, then use generator to run against the last 3 versions.
        if (!version)
        {
            SQLiteVersion latestVersion{ 2, 1 };
            SQLiteVersion versionMinus1 = SQLiteVersion{ 2, 0 };
            SQLiteVersion versionMinus2 = SQLiteVersion{ 1, 7 };

            version = GENERATE_COPY(SQLiteVersion{ versionMinus2 }, SQLiteVersion{ versionMinus1 }, SQLiteVersion{ latestVersion });
        }

        return SQLiteIndex::CreateNew(filePath, version.value());
    }

    std::string GetPathFromManifest(Manifest& manifest)
    {
        auto publisher = manifest.Id;
        AppInstaller::Utility::FindAndReplace(publisher, ".", "/");

        return AppInstaller::Utility::ToLower(publisher).append("/").append(manifest.Version);
    }

    void CreateFakeManifest(Manifest& manifest, string_t publisher, string_t version)
    {
        manifest.Installers.push_back({});
        manifest.Id = publisher.append(".").append("Id");
        manifest.DefaultLocalization.Add<Localization::PackageName>(publisher.append(" Name"));
        manifest.Moniker = "testmoniker";
        manifest.Version = version;
        manifest.Channel = "test";
        manifest.DefaultLocalization.Add<Localization::Tags>({ "t1", "t2" });
        manifest.Installers[0].Commands = { "test1", "test2" };
    }

    void CreateFakeManifestAndPath(
        ManifestAndPath& manifestAndPath,
        const string_t& publisher,
        std::string_view version,
        std::optional<std::string_view> arpMinVersion,
        std::optional<std::string_view> arpMaxVersion)
    {
        CreateFakeManifest(manifestAndPath.Manifest, publisher, string_t{ version });
        manifestAndPath.Path = ConvertToUTF8(CreateNewGuidNameWString());
        manifestAndPath.Manifest.StreamSha256 = AppInstaller::Utility::SHA256::ComputeHash(manifestAndPath.Path);

        if (arpMinVersion)
        {
            manifestAndPath.Manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
            manifestAndPath.Manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
            manifestAndPath.Manifest.Installers[0].AppsAndFeaturesEntries.back().DisplayVersion = arpMinVersion.value();
        }

        if (arpMaxVersion)
        {
            manifestAndPath.Manifest.Installers[0].BaseInstallerType = InstallerTypeEnum::Exe;
            manifestAndPath.Manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
            manifestAndPath.Manifest.Installers[0].AppsAndFeaturesEntries.back().DisplayVersion = arpMaxVersion.value();
        }
    }

    void ApplyIndexFields(Manifest& manifest, const IndexFields& fields)
    {
        manifest.Id = fields.Id;
        manifest.DefaultLocalization.Add<Localization::PackageName>(fields.Name);
        manifest.DefaultLocalization.Add<Localization::Publisher>(fields.Publisher);
        manifest.Moniker = fields.Moniker;
        manifest.Version = fields.Version;
        manifest.DefaultLocalization.Add<Localization::Tags>(fields.Tags);

        manifest.Installers.resize(std::max(fields.PackageFamilyNames.size(), fields.ProductCodes.size()));

        if (manifest.Installers.size() == 0)
        {
            manifest.Installers.push_back({});
        }

        manifest.Channel = fields.Channel;
        manifest.Installers[0].Commands = fields.Commands;

        for (size_t i = 0; i < fields.PackageFamilyNames.size(); ++i)
        {
            manifest.Installers[i].PackageFamilyName = fields.PackageFamilyNames[i];
        }

        for (size_t i = 0; i < fields.ProductCodes.size(); ++i)
        {
            manifest.Installers[i].ProductCode = fields.ProductCodes[i];
        }

        if (!fields.ArpName.empty() || !fields.ArpPublisher.empty())
        {
            manifest.Installers[0].AppsAndFeaturesEntries.push_back({});
            manifest.Installers[0].AppsAndFeaturesEntries[0].DisplayName = fields.ArpName;
            manifest.Installers[0].AppsAndFeaturesEntries[0].Publisher = fields.ArpPublisher;
        }
    }

    Manifest CreateManifest(const IndexFields& fields)
    {
        Manifest manifest;
        ApplyIndexFields(manifest, fields);
        return manifest;
    }

    SQLiteIndex SearchTestSetup(const std::string& filePath, std::initializer_list<IndexFields> data, std::optional<SQLiteVersion> version)
    {
        SQLiteIndex index = CreateTestIndex(filePath, version);

        // A single manifest is carried across the whole set, matching what this helper has always
        // done. Anything the fields do not overwrite therefore persists from the previous entry.
        Manifest manifest;

        for (const auto& d : data)
        {
            ApplyIndexFields(manifest, d);
            index.AddManifest(manifest, d.Path);
        }

        return index;
    }
}
