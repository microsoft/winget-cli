// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "SourceFactory.h"

#include <AppInstallerMsixInfo.h>
#include <AppInstallerProgress.h>
#include <AppInstallerSynchronization.h>
#include <AppInstallerVersions.h>
#include <winget/ManagedFile.h>

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    using namespace std::string_view_literals;

    // TODO: This being hard coded to force using the Public directory name is not ideal.
    static constexpr std::string_view s_IndexFilePath = "Public\\index.db"sv;

    // The packages that a source can hold locally.
    enum class PackageSlot
    {
        // The complete index, as acquired by a client that is not using a delta.
        FullIndex,

        // The changes since the baseline.
        Delta,

        // The index that the delta's changes apply to.
        Baseline,
    };

    // A short token for a slot, used in diagnostics and in temporary file names.
    std::string_view GetSlotName(PackageSlot slot);

    // Identifies one of a source's packages to a store.
    //
    // The two fields are the two ways that a store can find a package, and which one it uses is
    // the store's business: the local file store names its own files and so keys on the slot,
    // while the platform keys deployed packages on their identity.
    struct PackageKey
    {
        PackageSlot Slot = PackageSlot::FullIndex;

        // The package family name of this package.
        std::string Identity;
    };

    // Makes the index database of a package that we hold locally readable.
    //
    // The deployed store already has it on disk inside the installed package; the local file store
    // has to extract it from the package file, so the result may own a temporary file that must
    // outlive any read of the path.
    struct ExtractedIndex
    {
        std::filesystem::path Path;
        Utility::ManagedFile TemporaryFile;
    };

    // A package that has been fetched and validated for a key, but is not yet stored anywhere that
    // it can be found again.
    //
    // Acquisition is entirely store independent, which is why it is separated from Persist and
    // lives on the base rather than on either mechanism. Holding the validation lock here is what
    // makes committing to it safe: the file cannot be altered between the point at which it was
    // checked and the point at which a store takes it.
    //
    // Note that this does not currently support handing a refused package to another store.
    // Persist takes the package by rvalue reference and the mechanisms consume it -- the local
    // file store releases the lock and moves the file -- so a failed Persist leaves the caller
    // with nothing to retry. Supporting that would need Persist to be defined as leaving the
    // package untouched on failure, which neither mechanism does today.
    struct AcquiredPackage
    {
        AcquiredPackage() = default;

        AcquiredPackage(const AcquiredPackage&) = delete;
        AcquiredPackage& operator=(const AcquiredPackage&) = delete;

        AcquiredPackage(AcquiredPackage&& other) noexcept;
        AcquiredPackage& operator=(AcquiredPackage&& other) noexcept;

        // Removes the file when it was ours and no store took it.
        ~AcquiredPackage();

        // The package that this was acquired to be.
        PackageKey Key;

        // Where the validated package currently is.
        std::filesystem::path Path;

        // The bytes transferred, when it came from a remote location.
        std::optional<uint64_t> DownloadedBytes;

        // Holds the file against modification from the moment it was validated.
        // A store that needs to move the file must release this first.
        std::optional<Msix::WriteLockedMsixFile> FileLock;

        // Declares that we created the file at Path and must remove it if nothing takes it.
        void MarkTemporary() { m_isTemporary = true; }

        // Declares that a store has taken responsibility for the file.
        void ReleaseTemporary() { m_isTemporary = false; }

        // Whether the file at Path is ours to consume. When it is not, the location named by the
        // source is being used in place and a store must copy rather than move it.
        bool IsTemporary() const { return m_isTemporary; }

    private:
        bool m_isTemporary = false;
    };

    // Holds a source's packages locally, so that they can be found again in a later process.
    // A store knows nothing about how the index is composed.
    struct IPackageStore
    {
        virtual ~IPackageStore() = default;

        // Fetches a package to a temporary location and validates it.
        // Nothing when the operation was cancelled.
        virtual std::optional<AcquiredPackage> Acquire(const PackageKey& package, const std::string& location, IProgressCallback& progress) = 0;

        // Stores an acquired package so that it can be found again.
        virtual void Persist(AcquiredPackage&& package, IProgressCallback& progress) = 0;

        // The version currently held for a package, or nothing when it is not held.
        virtual std::optional<Msix::PackageVersion> GetVersion(const PackageKey& package) const = 0;

        // Makes the index inside a held package readable. Nothing when it is not held.
        virtual std::optional<ExtractedIndex> GetIndex(const PackageKey& package, IProgressCallback& progress) = 0;

        // Removes everything held for the given packages.
        virtual void Remove(const std::vector<PackageKey>& packages, IProgressCallback& progress) = 0;

        // Takes the cross process lock guarding this source's local copy.
        virtual Synchronization::CrossProcessLock Lock(IProgressCallback& progress, bool isBackground = false) = 0;

        // Whether a read may be attempted before taking the lock.
        virtual bool AllowsUnlockedRead() const = 0;
    };

    // The store independent half of a store: the fields every store needs, acquisition, and the
    // lock, all of which are properties of the source rather than of the mechanism.
    struct PackageStoreBase : public IPackageStore
    {
        PackageStoreBase(const SourceDetails& details);

        std::optional<AcquiredPackage> Acquire(const PackageKey& package, const std::string& location, IProgressCallback& progress) override;

        Synchronization::CrossProcessLock Lock(IProgressCallback& progress, bool isBackground = false) override;

    protected:
        // Whether the trust validation of packages should require a Microsoft origin.
        bool RequireStoreOrigin() const;

        // Whether a package may be deployed without being explicitly trusted.
        bool IsTrusted() const;

        // Whether a source package is trusted enough to be used.
        //
        // Every store validates the same way, and this is the single point at which it happens, so
        // that no mechanism can diverge on what it accepts.
        bool ValidateTrust(const Msix::WriteLockedMsixFile& package) const;

        std::string m_sourceName;

        // The identity of the source itself, which every store derives its storage location from.
        std::string m_sourceIdentity;

        SourceTrustLevel m_trustLevel = SourceTrustLevel::None;
    };

    // Creates the store that this process should maintain a source in.
    std::unique_ptr<IPackageStore> CreateStore(const SourceDetails& details);

    // Creates a store over every store that this process can reach a source's packages through.
    // Reading answers from whichever of them holds the most recently published copy, and removing
    // clears all of them.
    std::unique_ptr<IPackageStore> CreateCompositeStore(const SourceDetails& details);

    // The individual mechanisms.
    std::unique_ptr<IPackageStore> CreateDeployedPackageStore(const SourceDetails& details);
    std::unique_ptr<IPackageStore> CreateLocalFilePackageStore(const SourceDetails& details);

    // Whether deployment is available to this process at all.
    bool CanUseDeployedPackage();
}
