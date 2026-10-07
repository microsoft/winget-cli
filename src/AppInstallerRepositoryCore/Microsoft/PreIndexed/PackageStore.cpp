// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/PreIndexed/PackageStore.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"

#include <AppInstallerDownloader.h>
#include <AppInstallerRuntime.h>
#include <AppInstallerStrings.h>

using namespace std::string_literals;

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    std::string_view GetSlotName(PackageSlot slot)
    {
        switch (slot)
        {
        case PackageSlot::FullIndex: return "fullIndex"sv;
        case PackageSlot::Delta: return "delta"sv;
        case PackageSlot::Baseline: return "baseline"sv;
        }

        THROW_HR(E_UNEXPECTED);
    }

    AcquiredPackage::AcquiredPackage(AcquiredPackage&& other) noexcept
    {
        *this = std::move(other);
    }

    AcquiredPackage& AcquiredPackage::operator=(AcquiredPackage&& other) noexcept
    {
        if (this != &other)
        {
            Key = std::move(other.Key);
            Path = std::move(other.Path);
            DownloadedBytes = std::move(other.DownloadedBytes);
            FileLock = std::move(other.FileLock);
            m_isTemporary = other.m_isTemporary;

            // The moved from object must not also remove the file.
            other.m_isTemporary = false;
            other.Path.clear();
        }

        return *this;
    }

    AcquiredPackage::~AcquiredPackage()
    {
        if (m_isTemporary && !Path.empty())
        {
            // Release the lock before attempting to remove the file that it holds.
            FileLock.reset();

            try
            {
                std::filesystem::remove(Path);
            }
            catch (...)
            {
                AICLI_LOG(Repo, Info, << "Failed to remove acquired package file at: " << Path);
            }
        }
    }

    PackageStoreBase::PackageStoreBase(const SourceDetails& details) :
        m_sourceName(details.Name), m_trustLevel(details.TrustLevel)
    {
        // Gets the identity of the source itself, as distinct from the identity of any one of the
        // packages it is composed of.
        //
        // The cross process lock name and the local state directory are both derived from this,
        // which is why it must be stable: changing either one orphans an existing source's local
        // state, and -- worse -- lets an old and a new client take different locks over the same data.
        // 
        // The fallback exists because Identifier was not always stored; a source written by an old
        // enough client has only Data, which is the value such a client would have put there.
        m_sourceIdentity = details.Identifier.empty() ? details.Data : details.Identifier;
        THROW_HR_IF(E_UNEXPECTED, m_sourceIdentity.empty());
    }

    bool PackageStoreBase::RequireStoreOrigin() const
    {
        return WI_IsFlagSet(m_trustLevel, SourceTrustLevel::StoreOrigin);
    }

    bool PackageStoreBase::IsTrusted() const
    {
        return WI_IsFlagSet(m_trustLevel, SourceTrustLevel::Trusted);
    }

    bool PackageStoreBase::ValidateTrust(const Msix::WriteLockedMsixFile& package) const
    {
        return package.ValidateTrustInfo(RequireStoreOrigin());
    }

    std::optional<AcquiredPackage> PackageStoreBase::Acquire(const PackageKey& package, const std::string& location, IProgressCallback& progress)
    {
        AcquiredPackage result;
        result.Key = package;

        if (Utility::IsUrlRemote(location))
        {
            std::filesystem::path localFile = Runtime::GetPathTo(Runtime::PathName::Temp);
            localFile /= m_sourceIdentity + "." + std::string{ GetSlotName(package.Slot) } + ".msix";

            // Set the path before downloading so that a partial download is still cleaned up.
            result.Path = localFile;
            result.MarkTemporary();

            auto downloadResult = Utility::Download(location, localFile, Utility::DownloadType::Index, progress);
            result.DownloadedBytes = downloadResult.SizeInBytes;
        }
        else
        {
            // A local location is not ours, so it is used in place and never removed.
            result.Path = Utility::ConvertToUTF16(location);
            progress.OnProgress(100, 100, ProgressType::Percent);
        }

        if (progress.IsCancelledBy(CancelReason::Any))
        {
            AICLI_LOG(Repo, Info, << "Cancelling acquisition upon request");
            return std::nullopt;
        }

        // Hold the file against modification from the moment that it is validated, so that no
        // store can commit to something other than what was checked here.
        Msix::WriteLockedMsixFile fileLock{ result.Path };
        Msix::MsixInfo localMsixInfo{ result.Path };

        // The package should not be a bundle
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, localMsixInfo.GetIsBundle());

        std::string packageFamilyName = Msix::GetPackageFamilyNameFromFullName(localMsixInfo.GetPackageFullName());

        // A package whose identity is not the one that was asked for is an integrity failure.
        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, package.Identity != packageFamilyName);

        if (!ValidateTrust(fileLock))
        {
            AICLI_LOG(Repo, Error, << "Source update failed. Source package failed trust validation.");
            THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE);
        }

        result.FileLock = std::move(fileLock);

        return std::optional<AcquiredPackage>{ std::move(result) };
    }

    Synchronization::CrossProcessLock PackageStoreBase::Lock(IProgressCallback& progress, bool isBackground)
    {
        Synchronization::CrossProcessLock result("PreIndexedSourceCPL_"s + m_sourceIdentity);

        if (isBackground)
        {
            // If this is a background update, don't wait on the lock.
            result.TryAcquireNoWait();
        }
        else
        {
            result.Acquire(progress);
        }

        return result;
    }

    bool CanUseDeployedPackage()
    {
        return Runtime::IsRunningInPackagedContext() && Runtime::IsRunningAsInteractiveUser();
    }

    namespace anon
    {
        // A store over every store that this process can reach a source's packages through.
        //
        // Reading answers from whichever of them holds the most recently published copy, and
        // removing clears all of them. Acquiring and persisting throw: a store that spans
        // mechanisms has no single place to put a package, and that decision belongs to
        // CreateStore.
        struct CompositePackageStore : public IPackageStore
        {
            CompositePackageStore(const SourceDetails& details)
            {
                // The store that this process would write to comes first, so that an equally
                // recent copy keeps a read on the store that it maintains itself.
                if (CanUseDeployedPackage())
                {
                    m_stores.emplace_back(CreateDeployedPackageStore(details));
                    m_stores.emplace_back(CreateLocalFilePackageStore(details));
                }
                else
                {
                    m_stores.emplace_back(CreateLocalFilePackageStore(details));
                    if (Runtime::IsRunningInPackagedContext())
                    {
                        m_stores.emplace_back(CreateDeployedPackageStore(details));
                    }
                }
            }

            std::optional<AcquiredPackage> Acquire(const PackageKey&, const std::string&, IProgressCallback&) override
            {
                THROW_WIN32(ERROR_NOT_SUPPORTED);
            }

            void Persist(AcquiredPackage&&, IProgressCallback&) override
            {
                THROW_WIN32(ERROR_NOT_SUPPORTED);
            }

            std::optional<Msix::PackageVersion> GetVersion(const PackageKey& package) const override
            {
                return Resolve(package).GetVersion(package);
            }

            std::optional<ExtractedIndex> GetIndex(const PackageKey& package, IProgressCallback& progress) override
            {
                return Resolve(package).GetIndex(package, progress);
            }

            void Remove(const std::vector<PackageKey>& packages, IProgressCallback& progress) override
            {
                HRESULT firstStoreFailure = S_OK;
                bool firstStore = true;

                for (auto& store : m_stores)
                {
                    try
                    {
                        store->Remove(packages, progress);
                    }
                    catch (...)
                    {
                        HRESULT hr = LOG_CAUGHT_EXCEPTION();

                        // Only throw if the primary (first) store fails to remove.
                        if (firstStore)
                        {
                            firstStoreFailure = hr;
                        }
                    }

                    firstStore = false;
                }

                THROW_IF_FAILED(firstStoreFailure);
            }

            Synchronization::CrossProcessLock Lock(IProgressCallback& progress, bool isBackground = false) override
            {
                // The lock is named from the source rather than from the mechanism, so every store
                // for a source shares one and any of them can take it.
                return m_stores.front()->Lock(progress, isBackground);
            }

            bool AllowsUnlockedRead() const override
            {
                // Before anything has been asked about, the store this process maintains is the
                // one most likely to answer, and deferring to it keeps this identical to what a
                // single store would have reported. A later read that resolves elsewhere and
                // cannot be served unlocked fails, which the caller already retries under the
                // lock.
                return (m_resolved ? m_resolved : m_stores.front().get())->AllowsUnlockedRead();
            }

        private:
            IPackageStore& Resolve(const PackageKey& package) const
            {
                if (!m_resolved)
                {
                    m_resolved = m_stores.front().get();
                    std::optional<Msix::PackageVersion> bestVersion = m_resolved->GetVersion(package);

                    for (auto itr = m_stores.begin() + 1; itr != m_stores.end(); ++itr)
                    {
                        std::optional<Msix::PackageVersion> version = (*itr)->GetVersion(package);

                        // The store this process would write to is first, so only a strictly more
                        // recent copy moves it off the store that it maintains itself.
                        if (version && (!bestVersion || bestVersion.value() < version.value()))
                        {
                            bestVersion = std::move(version);
                            m_resolved = itr->get();
                        }
                    }
                }

                return *m_resolved;
            }

            std::vector<std::unique_ptr<IPackageStore>> m_stores;
            mutable IPackageStore* m_resolved = nullptr;
        };
    }

    std::unique_ptr<IPackageStore> CreateStore(const SourceDetails& details)
    {
        return CanUseDeployedPackage() ? CreateDeployedPackageStore(details) : CreateLocalFilePackageStore(details);
    }

    std::unique_ptr<IPackageStore> CreateCompositeStore(const SourceDetails& details)
    {
        return std::make_unique<anon::CompositePackageStore>(details);
    }
}
