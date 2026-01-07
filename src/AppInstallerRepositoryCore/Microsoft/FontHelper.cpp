
// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FontHelper.h"
#include <winget/Fonts.h>
using namespace AppInstaller::Utility;

namespace AppInstaller::Repository::Microsoft
{
#ifndef AICLI_DISABLE_TEST_HOOKS
    using GetFontRegistryRootFunc = std::function<Registry::Key(Manifest::ScopeEnum)>;
    static GetFontRegistryRootFunc s_FontRegistryRoot_Override;

    void TestHook_SetGetFontRegistryRootFunc(GetFontRegistryRootFunc value)
    {
        s_FontRegistryRoot_Override = value;
    }
#endif

    void FontHelper::PopulateIndex(SQLiteIndex& index, Manifest::ScopeEnum scope) const
    {
        const auto& fontPackages = AppInstaller::Fonts::GetInstalledFontPackages(scope);
        for (const auto& fontPackage : fontPackages)
        {
            try
            {
                // Font packages are collections of fonts; the PackageId is treated as the productCode.
                std::string productCode = ConvertToUTF8(fontPackage.PackageId);

                Manifest::Manifest manifest;
                manifest.DefaultLocalization.Add<Manifest::Localization::Tags>({ "Font" });
                manifest.Id = ConvertToUTF8(fontPackage.PackageIdentifier);
                manifest.Version = ConvertToUTF8(fontPackage.PackageVersion);
                manifest.Moniker = productCode;
                manifest.Installers.emplace_back();
                manifest.Installers[0].ProductCode = productCode;

                // We don't know the package name from the install information. The best we can do is reuse the id.
                manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(productCode);

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

    void FontHelper::AddRegistryWatchers(Manifest::ScopeEnum scope, std::function<void(Manifest::ScopeEnum, wil::RegistryChangeKind)> callback, std::vector<wil::unique_registry_watcher>& watchers)
    {
        auto addToWatchers = [&](Manifest::ScopeEnum scopeToUse)
            {
                auto hive = scopeToUse == Manifest::ScopeEnum::Machine ? HKEY_LOCAL_MACHINE : HKEY_CURRENT_USER;
                auto root = Registry::Key::OpenIfExists(hive, AppInstaller::Fonts::GetFontRegistryRoot(), 0UL, KEY_READ);

#ifndef AICLI_DISABLE_TEST_HOOKS
                if (s_FontRegistryRoot_Override)
                {
                    root = s_FontRegistryRoot_Override(scopeToUse);
                }
#endif
                if (root)
                {
                    watchers.emplace_back(wil::make_registry_watcher(root, L"", true, [scopeToUse, callback](wil::RegistryChangeKind change) { callback(scopeToUse, change); }));
                }
            };

        if (scope == Manifest::ScopeEnum::Unknown)
        {
            addToWatchers(Manifest::ScopeEnum::User);
            addToWatchers(Manifest::ScopeEnum::Machine);
        }
        else
        {
            addToWatchers(scope);
        }
    }
}
