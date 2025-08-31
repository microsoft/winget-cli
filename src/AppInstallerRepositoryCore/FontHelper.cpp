
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontHelper.h"
#include <winget/Fonts.h>
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Microsoft
{
    void FontHelper::PopulateIndex(SQLiteIndex& index, Manifest::ScopeEnum scope) const
    {
        const auto& fontPackages = AppInstaller::Fonts::GetInstalledFontPackages(scope);
        for (const auto& fontPackage : fontPackages)
        {
            try
            {
                Manifest::Manifest manifest;
                manifest.DefaultLocalization.Add<Manifest::Localization::Tags>({ "Font" });
                manifest.Id = ConvertToUTF8(fontPackage.PackageId);
                manifest.Version = ConvertToUTF8(fontPackage.PackageVersion);
                manifest.Installers.emplace_back();

                // We don't know the package name from the install information. The best we can do is reuse the id.
                manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(manifest.Id);

                std::optional<SQLiteIndex::IdType> manifestIdOpt;
                try
                {
                    // Use the ProductCode as a unique key for the path
                    manifestIdOpt = index.AddManifest(manifest);
                }
                catch (...)
                {
                    // Same package may show up under User and Machine install.
                    // Ignore errors if they occur, they are most likely a duplicate value
                }

                if (!manifestIdOpt)
                {
                    AICLI_LOG(Repo, Warning,
                        << "Ignoring duplicate font entry " << scope << '|' << manifest.Id << '|' << manifest.Version);
                    continue;
                }

                SQLiteIndex::IdType manifestId = manifestIdOpt.value();

                index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledScope, Manifest::ScopeToString(scope));
                index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledType, Manifest::InstallerTypeToString(Manifest::InstallerTypeEnum::Font));                
            }
            catch (...)
            {
                AICLI_LOG(Repo, Warning, << "Failed to process font package entry, ignoring it: " << scope << '|' << ConvertToUTF8(fontPackage.PackageId));
                LOG_CAUGHT_EXCEPTION();
            }
        }
    }
}
