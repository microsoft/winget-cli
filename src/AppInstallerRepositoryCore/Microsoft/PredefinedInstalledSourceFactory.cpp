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
            const Utility::VersionAndChannel* versionKey = nullptr;

            for (const auto& key : versionKeys)
            {
                if (key.GetVersion() == version)
                {
                    versionKey = &key;
                    break;
                }
            }

            if (!versionKey)
            {
                return std::nullopt;
            }

            auto manifestId = cacheData.GetManifestIdByKey(searchResult.Matches[0].first, versionKey->GetVersion().ToString(), versionKey->GetChannel().ToString());

            if (!manifestId)
            {
                // This is very unexpected, but just log it and carry on
                AICLI_LOG(Repo, Warning, << "Did not find manifest id for: " << id << ", " << versionKey->GetVersion().ToString() << ", " << versionKey->GetChannel().ToString());
                return std::nullopt;
            }

            return cacheData.GetPropertyByManifestId(manifestId.value(), PackageVersionProperty::Name);
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
                packages = packageManager.FindPackagesForUserWithPackageTypes({}, PackageTypes::Main);
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
                Utility::NormalizedString familyName = Utility::ConvertToUTF8(packageId.FamilyName());

                manifest.Id = familyName;

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
                            Utility::ConvertToUTF8(hre.message()) << "] exception thrown when getting DisplayName for " << familyName);
                    }
                    catch (...)
                    {
                        AICLI_LOG(Repo, Warning, << "Unknown exception thrown when getting DisplayName for " << familyName);
                    }
                }

                if (!isPackageNameSet)
                {
                    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(Utility::ConvertToUTF8(packageId.Name()));
                }

                manifest.Installers[0].PackageFamilyName = familyName;

                // Use the full name as a unique key for the path
                auto manifestId = index.AddManifest(manifest, std::filesystem::path{ packageId.FullName().c_str() });

                index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledType, 
                    Manifest::InstallerTypeToString(Manifest::InstallerTypeEnum::Msix));
            }
        }

        SQLiteIndex CreateAndPopulateIndex(PredefinedInstalledSourceFactory::Filter filter)
        {
            AICLI_LOG(Repo, Verbose, << "Creating PredefinedInstalledSource with filter [" << PredefinedInstalledSourceFactory::FilterToString(filter) << ']');

            // Create an in memory index
            SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, SQLite::Version::Latest());

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
            CachedInstalledIndex() :
                m_index(CreateAndPopulateIndex(PredefinedInstalledSourceFactory::Filter::None))
            {
            }

            void UpdateIndexIfNeeded()
            {
                // Update index if last write time is more than X ago.
                // This prevents rapid fire updates while still being reasonably in sync.
                constexpr auto c_updateCheckTimeout = 1s;
                if (CheckLastWriteAgainst(c_updateCheckTimeout))
                {
                    auto lock = m_lock.lock_exclusive();

                    if (CheckLastWriteAgainst(c_updateCheckTimeout))
                    {
                        // TODO: To support servicing, the initial implementation of update will simply leverage
                        //       some data from the existing index to speed up the MSIX populate function.
                        //       In a larger update, we may want to make it possible to actually update the cache directly.
                        //       We may even persist the cache to disk to speed things up further.

                        // Populate from ARP using standard mechanism.
                        SQLiteIndex update = CreateAndPopulateIndex(PredefinedInstalledSourceFactory::Filter::ARP);

                        // Populate from MSIX, using localization data from the existing index if applicable.
                        PopulateIndexFromMSIX(update, Manifest::ScopeEnum::User, &m_index);

                        m_index = std::move(update);
                    }
                }
            }

            SQLiteIndex GetCopy()
            {
                auto lock = m_lock.lock_shared();
                return SQLiteIndex::CopyFrom(SQLITE_MEMORY_DB_CONNECTION_TARGET, m_index);
            }

        private:
            bool CheckLastWriteAgainst(std::chrono::milliseconds timeout)
            {
                auto lastWrite = m_index.GetLastWriteTime();
                auto now = std::chrono::system_clock::now();
                auto duration = now - lastWrite;

                return duration > timeout;
            }

            SQLiteIndex m_index;
            wil::srwlock m_lock;
        };

        struct PredefinedInstalledSourceReference : public ISourceReference
        {
            PredefinedInstalledSourceReference(const SourceDetails& details) : m_details(details)
            {
                m_details.Identifier = "*PredefinedInstalledSource";
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
                if (filter == PredefinedInstalledSourceFactory::Filter::None)
                {
                    static CachedInstalledIndex s_installedIndex;
                    s_installedIndex.UpdateIndexIfNeeded();
                    return std::make_shared<SQLiteIndexSource>(m_details, s_installedIndex.GetCopy(), true);
                }
                else
                {
                    return std::make_shared<SQLiteIndexSource>(m_details, CreateAndPopulateIndex(filter), true);
                }
            }

        private:
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
