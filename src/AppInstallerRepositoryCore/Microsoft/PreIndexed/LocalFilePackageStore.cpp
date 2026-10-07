// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/PreIndexed/PackageStore.h"
#include "Microsoft/PreIndexedPackageSourceFactory.h"

#include <AppInstallerMsixInfo.h>
#include <AppInstallerRuntime.h>
#include <winget/Filesystem.h>

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    namespace anon
    {
        static constexpr std::string_view s_LocalFullIndexFileName = "source.msix"sv;
        static constexpr std::string_view s_LocalDeltaFileName = "delta.msix"sv;
        static constexpr std::string_view s_LocalBaselineFileName = "baseline.msix"sv;

        // The file name that a slot's package is stored under.
        //
        // We choose these names ourselves, so this mechanism needs no package identity at all.
        // Note that the full index is stored as source.msix regardless of which of the two remote
        // names it came from.
        std::string_view GetLocalFileNameForSlot(PackageSlot slot)
        {
            switch (slot)
            {
            case PackageSlot::FullIndex: return s_LocalFullIndexFileName;
            case PackageSlot::Delta: return s_LocalDeltaFileName;
            case PackageSlot::Baseline: return s_LocalBaselineFileName;
            }

            THROW_HR(E_UNEXPECTED);
        }


        // Holds a source's packages as files in our own local state.
        struct LocalFilePackageStore : public PackageStoreBase
        {
            using PackageStoreBase::PackageStoreBase;

            void Persist(AcquiredPackage&& package, IProgressCallback&) override
            {
                std::filesystem::path packageState = GetStatePath();
                std::filesystem::create_directories(packageState);

                std::filesystem::path packagePath = packageState / GetLocalFileNameForSlot(package.Key.Slot);
                std::filesystem::path stagedPath = packagePath.u8string() + ".dnld.msix";

                auto removeStagedFileOnExit = wil::scope_exit([&]()
                    {
                        try
                        {
                            std::filesystem::remove(stagedPath);
                        }
                        catch (...)
                        {
                            AICLI_LOG(Repo, Info, << "Failed to remove temp index file at: " << stagedPath);
                        }
                    });

                // The final move has to be a rename within the state directory, because that is
                // what makes replacing the held package atomic. So the validated file is staged
                // beside its destination first, which may cross a volume boundary, and only then
                // renamed into place.
                //
                // The lock is released before the file is moved; everything it was guarding has
                // already been checked, and a locked file cannot be moved or removed.
                package.FileLock.reset();

                if (package.IsTemporary())
                {
                    Filesystem::RenameFile(package.Path, stagedPath);
                    package.ReleaseTemporary();
                }
                else
                {
                    // The file is the location that the source names, not ours to consume.
                    std::filesystem::copy_file(package.Path, stagedPath, std::filesystem::copy_options::overwrite_existing);
                }

                std::filesystem::rename(stagedPath, packagePath);
                AICLI_LOG(Repo, Info, << "Source update success.");

                removeStagedFileOnExit.release();
            }

            std::optional<Msix::PackageVersion> GetVersion(const PackageKey& package) const override
            {
                std::filesystem::path packagePath = GetPathForSlot(package.Slot);

                if (std::filesystem::exists(packagePath))
                {
                    // If we already have a trusted index package, use it to determine if we need to update or not.
                    Msix::WriteLockedMsixFile indexPackage{ packagePath };
                    if (ValidateTrust(indexPackage))
                    {
                        Msix::MsixInfo msixInfo{ packagePath };
                        auto manifest = msixInfo.GetAppPackageManifests();

                        if (manifest.size() == 1)
                        {
                            return manifest[0].GetIdentity().GetVersion();
                        }
                    }
                }

                return std::nullopt;
            }

            std::optional<ExtractedIndex> GetIndex(const PackageKey& package, IProgressCallback& progress) override
            {
                std::filesystem::path packagePath = GetPathForSlot(package.Slot);

                if (!std::filesystem::exists(packagePath))
                {
                    AICLI_LOG(Repo, Info, << "Data not found at " << packagePath);
                    return std::nullopt;
                }

                // Put a write exclusive lock on the index package.
                Msix::WriteLockedMsixFile indexPackage{ packagePath };

                // Validate index package trust info.
                THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_DATA_INTEGRITY_FAILURE, !ValidateTrust(indexPackage));

                // Create a temp lock exclusive index file.
                auto tempIndexFilePath = Runtime::GetNewTempFilePath();
                auto tempIndexFile = Utility::ManagedFile::CreateWriteLockedFile(tempIndexFilePath, GENERIC_WRITE, true);

                // Populate temp index file.
                Msix::MsixInfo packageInfo{ packagePath };
                packageInfo.WriteToFileHandle(s_IndexFilePath, tempIndexFile.GetFileHandle(), progress);

                ExtractedIndex result;
                result.Path = tempIndexFile.GetFilePath();
                result.TemporaryFile = std::move(tempIndexFile);

                return std::optional<ExtractedIndex>{ std::move(result) };
            }

            // The state directory holds this source's packages and nothing else, so removing it
            // removes every slot. Nothing that a caller could name is left behind.
            void Remove(const std::vector<PackageKey>&, IProgressCallback&) override
            {
                std::filesystem::path packageState = GetStatePath();

                if (!std::filesystem::exists(packageState))
                {
                    AICLI_LOG(Repo, Info, << "No state found for source: " << packageState.u8string());
                }
                else
                {
                    AICLI_LOG(Repo, Info, << "Removing state found for source: " << packageState.u8string());
                    std::filesystem::remove_all(packageState);
                }
            }

            // Reading extracts from the held file, so the lock must be taken first.
            bool AllowsUnlockedRead() const override { return false; }

        private:
            std::filesystem::path GetStatePath() const
            {
                std::filesystem::path result = Runtime::GetPathTo(Runtime::PathName::LocalState);
                result /= PreIndexedPackageSourceFactory::Type();
                result /= m_sourceIdentity;
                return result;
            }

            std::filesystem::path GetPathForSlot(PackageSlot slot) const
            {
                return GetStatePath() / GetLocalFileNameForSlot(slot);
            }
        };
    }

    std::unique_ptr<IPackageStore> CreateLocalFilePackageStore(const SourceDetails& details)
    {
        return std::make_unique<anon::LocalFilePackageStore>(details);
    }
}
