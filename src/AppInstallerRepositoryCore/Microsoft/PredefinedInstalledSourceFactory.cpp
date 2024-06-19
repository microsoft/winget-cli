// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/ARPHelper.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Microsoft/SQLiteIndex.h"
#include "Microsoft/SQLiteIndexSource.h"
#include <winget/ManifestInstaller.h>

#include <winget/Registry.h>
#include <AppInstallerArchitecture.h>
#include <winget/ExperimentalFeature.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        std::optional<std::string> GetCachedMSIXName(const Utility::NormalizedString& id, const Utility::Version& version, SQLiteIndex& cacheData)
        {
            SearchRequest searchRequest;
            searchRequest.Inclusions.emplace_back(PackageMatchField::Id, MatchType::Exact, id);

            SQLiteIndex::SearchResult searchResult = cacheData.Search(searchRequest);

            if (searchResult.Matches.empty())
            {
                return std::nullopt;
            }

            if (searchResult.Matches.size() != 1)
            {
                // This is very unexpected, but just log it and carry on
                AICLI_LOG(Repo, Warning, << "Found multiple (" << searchResult.Matches.size() << ") cache entries for: " << id);
                return std::nullopt;
            }

            auto versionKeys = cacheData.GetVersionKeysById(searchResult.Matches[0].first);
            const SQLiteIndex::VersionKey* versionKey = nullptr;

            for (const auto& key : versionKeys)
            {
                if (key.VersionAndChannel.GetVersion() == version)
                {
                    versionKey = &key;
                    break;
                }
            }

            if (!versionKey)
            {
                return std::nullopt;
            }

            return cacheData.GetPropertyByPrimaryId(versionKey->ManifestId, PackageVersionProperty::Name);
        }

        // Populates the index with the entries from MSIX.
        void PopulateIndexFromMSIX(SQLiteIndex& index, Manifest::ScopeEnum scope, SQLiteIndex* cacheData = nullptr)
        {
            using namespace winrt::Windows::ApplicationModel;
            using namespace winrt::Windows::Management::Deployment;
            using namespace winrt::Windows::Foundation::Collections;

            AICLI_LOG(Repo, Verbose, << "Examining MSIX entries for " << ScopeToString(scope));

            IIterable<Package> packages;
            PackageManager packageManager;
            if (scope == Manifest::ScopeEnum::Machine)
            {
                packages = packageManager.FindProvisionedPackages();
            }
            else
            {
                // TODO: Consider if Optional packages should also be enumerated
                packages = packageManager.FindPackagesForUserWithPackageTypes({}, PackageTypes::Main | PackageTypes::Framework);
            }

            // Reuse the same manifest object, as we will be setting the same values every time.
            Manifest::Manifest manifest;
            // Add one installer for storing the package family name.
            manifest.Installers.emplace_back();
            // Every package will have the same tags currently.
            manifest.DefaultLocalization.Add<Manifest::Localization::Tags>({ "msix" });

            // Fields in the index but not populated:
            //  AppMoniker - Not sure what we would put.
            //  Channel - We don't know this information here.
            //  Commands - We could open the manifest and look for these eventually.
            //  Tags - Not sure what else we could put in here.
            for (const auto& package : packages)
            {
                // System packages are part of the OS, and cannot be managed by the user.
                // Filter them out as there is no point in showing them in a package manager.
                auto signatureKind = package.SignatureKind();
                if (signatureKind == PackageSignatureKind::System)
                {
                    continue;
                }

                auto packageId = package.Id();
                Utility::NormalizedString fullName = Utility::ConvertToUTF8(packageId.FullName());
                Utility::NormalizedString familyName = Utility::ConvertToUTF8(packageId.FamilyName());

                manifest.Id = "MSIX\\" + fullName;

                // Get version
                std::ostringstream strstr;
                auto packageVersion = packageId.Version();
                strstr << packageVersion.Major << '.' << packageVersion.Minor << '.' << packageVersion.Build << '.' << packageVersion.Revision;

                manifest.Version = strstr.str();

                // Determine package name
                bool isPackageNameSet = false;

                // Look for the name in the cache data first
                if (cacheData)
                {
                    std::optional<std::string> cachedName = GetCachedMSIXName(manifest.Id, manifest.Version, *cacheData);

                    if (cachedName)
                    {
                        manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(cachedName.value());
                        isPackageNameSet = true;
                    }
                }

                // Attempt to get the DisplayName. Since this will retrieve the localized value, it has a chance to fail.
                // Rather than completely skip this package in that case, we will simply fall back to using the package name below.
                if (!isPackageNameSet && !Runtime::IsRunningAsSystem())
                {
                    try
                    {
                        auto displayName = Utility::ConvertToUTF8(package.DisplayName());
                        if (!displayName.empty())
                        {
                            manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(displayName);
                            isPackageNameSet = true;
                        }
                    }
                    catch (const winrt::hresult_error& hre)
                    {
                        AICLI_LOG(Repo, Warning, << "winrt::hresult_error[0x" << Logging::SetHRFormat << hre.code() << ": " <<
                            Utility::ConvertToUTF8(hre.message()) << "] exception thrown when getting DisplayName for " << fullName);
                    }
                    catch (...)
                    {
                        AICLI_LOG(Repo, Warning, << "Unknown exception thrown when getting DisplayName for " << fullName);
                    }
                }

                if (!isPackageNameSet)
                {
                    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(Utility::ConvertToUTF8(packageId.Name()));
                }

                manifest.Installers[0].PackageFamilyName = familyName;

                // Use the full name as a unique key for the path
                auto manifestId = index.AddManifest(manifest);

                index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledType,
                    Manifest::InstallerTypeToString(Manifest::InstallerTypeEnum::Msix));

                auto architecture = Utility::ConvertToArchitectureEnum(packageId.Architecture());
                if (architecture)
                {
                    index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledArchitecture,
                        ToString(architecture.value()));
                }
            }
        }

        SQLiteIndex CreateAndPopulateIndex(PredefinedInstalledSourceFactory::Filter filter)
        {
            AICLI_LOG(Repo, Verbose, << "Creating PredefinedInstalledSource with filter [" << PredefinedInstalledSourceFactory::FilterToString(filter) << ']');

            // Create an in memory index
            SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, SQLite::Version::Latest(), SQLiteIndex::CreateOptions::SupportPathless);

            // Put installed packages into the index
            if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::ARP ||
                filter == PredefinedInstalledSourceFactory::Filter::User || filter == PredefinedInstalledSourceFactory::Filter::Machine)
            {
                ARPHelper arpHelper;
                if (filter != PredefinedInstalledSourceFactory::Filter::User)
                {
                    arpHelper.PopulateIndexFromARP(index, Manifest::ScopeEnum::Machine);
                }
                if (filter != PredefinedInstalledSourceFactory::Filter::Machine)
                {
                    arpHelper.PopulateIndexFromARP(index, Manifest::ScopeEnum::User);
                }
            }

            if (filter == PredefinedInstalledSourceFactory::Filter::None ||
                filter == PredefinedInstalledSourceFactory::Filter::MSIX ||
                filter == PredefinedInstalledSourceFactory::Filter::User)
            {
                PopulateIndexFromMSIX(index, Manifest::ScopeEnum::User);
            }
            else if (filter == PredefinedInstalledSourceFactory::Filter::Machine)
            {
                PopulateIndexFromMSIX(index, Manifest::ScopeEnum::Machine);
            }

            AICLI_LOG(Repo, Verbose, << " ... finished creating PredefinedInstalledSource");

            return index;
        }

        struct CachedInstalledIndex
        {
            CachedInstalledIndex()
            {
                ARPHelper arpHelper;
                m_registryWatchers = arpHelper.CreateRegistryWatchers(Manifest::ScopeEnum::Unknown,
                    [this](Manifest::ScopeEnum, Utility::Architecture, wil::RegistryChangeKind) { ForceNextUpdate(); });

                m_catalog = winrt::Windows::ApplicationModel::PackageCatalog::OpenForCurrentUser();
                m_eventRevoker = m_catalog.PackageStatusChanged(winrt::auto_revoke, [this](auto...) { ForceNextUpdate(); });
            }

            void UpdateIndexIfNeeded()
            {
                auto sharedLock = m_lock.lock_shared();
                if (CheckForUpdate())
                {
                    // Upgrade to exclusive
                    sharedLock.reset();
                    auto exclusiveLock = m_lock.lock_exclusive();

                    if (CheckForUpdate())
                    {
                        // TODO: To support servicing, the initial implementation of update will simply leverage
                        //       some data from the existing index to speed up the MSIX populate function.
                        //       In a larger update, we may want to make it possible to actually update the cache directly.
                        //       We may even persist the cache to disk to speed things up further.

                        // Set the update indicator to false before we start reading so that an external change can
                        // reindicate a need to update in the middle. But in the event that we error here, set it back to true
                        // to prevent an error from blocking further attempts.
                        m_forceNextUpdate = false;
                        auto scopeExit = wil::scope_exit([&]() { m_forceNextUpdate = true; });

                        // Populate from ARP using standard mechanism.
                        SQLiteIndex update = CreateAndPopulateIndex(PredefinedInstalledSourceFactory::Filter::ARP);

                        // Populate from MSIX, using localization data from the existing index if applicable.
                        PopulateIndexFromMSIX(update, Manifest::ScopeEnum::User, m_index.get());

                        m_index = std::make_unique<SQLiteIndex>(std::move(update));
                        scopeExit.release();
                    }
                }
            }

            SQLiteIndex GetCopy()
            {
                auto lock = m_lock.lock_shared();
                THROW_HR_IF(E_POINTER, !m_index);
                return SQLiteIndex::CopyFrom(SQLITE_MEMORY_DB_CONNECTION_TARGET, *m_index);
            }

            void ForceNextUpdate()
            {
                m_forceNextUpdate = true;
            }

        private:
            bool CheckForUpdate()
            {
                return (!m_index || m_forceNextUpdate.load());
            }

            wil::srwlock m_lock;
            std::atomic_bool m_forceNextUpdate{ false };
            std::unique_ptr<SQLiteIndex> m_index;
            std::vector<wil::unique_registry_watcher> m_registryWatchers;
            winrt::Windows::ApplicationModel::PackageCatalog m_catalog = nullptr;
            decltype(winrt::Windows::ApplicationModel::PackageCatalog{ nullptr }.PackageStatusChanged(winrt::auto_revoke, nullptr)) m_eventRevoker;
        };

        struct PredefinedInstalledSourceReference : public ISourceReference
        {
            PredefinedInstalledSourceReference(const SourceDetails& details) : m_details(details)
            {
                m_details.Identifier = "*PredefinedInstalledSource";

                if (PredefinedInstalledSourceFactory::StringToFilter(m_details.Arg) == PredefinedInstalledSourceFactory::Filter::NoneWithForcedCacheUpdate)
                {
                    GetCachedInstalledIndex().ForceNextUpdate();
                }
            }

            std::string GetIdentifier() override { return m_details.Identifier; }

            SourceDetails& GetDetails() override { return m_details; };

            std::shared_ptr<ISource> Open(IProgressCallback& progress) override
            {
                // TODO: Maybe we do need to use it?
                UNREFERENCED_PARAMETER(progress);

                // Determine the filter
                PredefinedInstalledSourceFactory::Filter filter = PredefinedInstalledSourceFactory::StringToFilter(m_details.Arg);

                // Only cache for the unfiltered install data
                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::NoneWithForcedCacheUpdate)
                {
                    CachedInstalledIndex& cachedIndex = GetCachedInstalledIndex();
                    cachedIndex.UpdateIndexIfNeeded();
                    return std::make_shared<SQLiteIndexSource>(m_details, cachedIndex.GetCopy(), true);
                }
                else
                {
                    return std::make_shared<SQLiteIndexSource>(m_details, CreateAndPopulateIndex(filter), true);
                }
            }

        private:
            CachedInstalledIndex& GetCachedInstalledIndex()
            {
                static CachedInstalledIndex s_installedIndex;
                return s_installedIndex;
            }

            SourceDetails m_details;
        };

        // The factory for the predefined installed source.
        struct Factory : public ISourceFactory
        {
            std::string_view TypeName() const override final
            {
                return PredefinedInstalledSourceFactory::Type();
            }

            std::shared_ptr<ISourceReference> Create(const SourceDetails& details) override final
            {
                THROW_HR_IF(E_INVALIDARG, details.Type != PredefinedInstalledSourceFactory::Type());

                return std::make_shared<PredefinedInstalledSourceReference>(details);
            }

            bool Add(SourceDetails&, IProgressCallback&) override final
            {
                // Add should never be needed, as this is predefined.
                THROW_HR(E_NOTIMPL);
            }

            bool Update(const SourceDetails&, IProgressCallback&) override final
            {
                // Update could be used later, but not for now.
                THROW_HR(E_NOTIMPL);
            }

            bool Remove(const SourceDetails&, IProgressCallback&) override final
            {
                // Similar to add, remove should never be needed.
                THROW_HR(E_NOTIMPL);
            }
        };
    }

    std::string_view PredefinedInstalledSourceFactory::FilterToString(Filter filter)
    {
        switch (filter)
        {
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::None:
            return "None"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::ARP:
            return "ARP"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::MSIX:
            return "MSIX"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::User:
            return "User"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::Machine:
            return "Machine"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::NoneWithForcedCacheUpdate:
            return "NoneWithForcedCacheUpdate"sv;
        default:
            return "Unknown"sv;
        }
    }

    PredefinedInstalledSourceFactory::Filter PredefinedInstalledSourceFactory::StringToFilter(std::string_view filter)
    {
        if (filter == FilterToString(Filter::ARP))
        {
            return Filter::ARP;
        }
        else if (filter == FilterToString(Filter::MSIX))
        {
            return Filter::MSIX;
        }
        else if (filter == FilterToString(Filter::User))
        {
            return Filter::User;
        }
        else if (filter == FilterToString(Filter::Machine))
        {
            return Filter::Machine;
        }
        else if (filter == FilterToString(Filter::NoneWithForcedCacheUpdate))
        {
            return Filter::NoneWithForcedCacheUpdate;
        }
        else
        {
            return Filter::None;
        }
    }

    std::unique_ptr<ISourceFactory> PredefinedInstalledSourceFactory::Create()
    {
        return std::make_unique<Factory>();
    }
}
