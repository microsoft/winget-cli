// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/PreIndexed/IndexForm.h"
#include "Microsoft/PreIndexed/RemotePackage.h"

#include <AppInstallerDateTime.h>
#include <AppInstallerDownloader.h>

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    namespace anon
    {
        // A source composed of a single package holding the complete index.
        struct FullIndexForm : public IIndexForm
        {
            FullIndexForm(const SourceDetails& details) : m_details(details)
            {
                m_identity = details.Data;
            }

            std::optional<std::string> DiscoverIdentities(IProgressCallback&) override
            {
                if (!m_identity.empty())
                {
                    return m_identity;
                }

                PreIndexedPackageInfo packageInfo(GetFullIndexPackageLocations(m_details), [](const std::string& packageLocation)
                    {
                        THROW_HR_IF(APPINSTALLER_CLI_ERROR_SOURCE_NOT_SECURE, Utility::IsUrlRemote(packageLocation) && !Utility::IsUrlSecure(packageLocation));
                    });

                AICLI_LOG(Repo, Info, << "Initializing source from: " << m_details.Name << " => " << packageInfo.PackageLocation());

                THROW_HR_IF(APPINSTALLER_CLI_ERROR_PACKAGE_IS_BUNDLE, packageInfo.MsixInfo().GetIsBundle());

                auto fullName = packageInfo.MsixInfo().GetPackageFullName();
                AICLI_LOG(Repo, Info, << "Found package full name: " << m_details.Name << " => " << fullName);

                m_identity = Msix::GetPackageFamilyNameFromFullName(fullName);

                return m_identity;
            }

            std::vector<PackageKey> GetPackages() const override
            {
                return { GetKey() };
            }

            bool HasIdentities() const override
            {
                return !m_identity.empty();
            }

            bool IsHeld(const IPackageStore& store) const override
            {
                return GetHeldVersion(store).has_value();
            }

            std::optional<Msix::PackageVersion> GetHeldVersion(const IPackageStore& store) const override
            {
                return store.GetVersion(GetKey());
            }

            UpdateResult Update(IPackageStore& store, bool isBackground, IProgressCallback& progress, UpdateReport& report) override
            {
                std::optional<Msix::PackageVersion> currentVersion = GetHeldVersion(store);
                PreIndexedPackageUpdateCheck updateCheck(GetFullIndexPackageLocations(m_details));

                if (currentVersion)
                {
                    report.PreviousIndexPublishedAt = Utility::GetTimePointFromVersion(currentVersion.value());
                }

                report.NewIndexPublishedAt = Utility::GetTimePointFromVersion(updateCheck.AvailableVersion());

                if (currentVersion)
                {
                    if (currentVersion.value() >= updateCheck.AvailableVersion())
                    {
                        AICLI_LOG(Repo, Verbose, << "Remote source data (" << updateCheck.AvailableVersion().ToString() <<
                            ") was not newer than existing (" << currentVersion.value().ToString() << "), no update needed");
                        return UpdateResult::Success;
                    }
                    else
                    {
                        AICLI_LOG(Repo, Verbose, << "Remote source data (" << updateCheck.AvailableVersion().ToString() <<
                            ") was newer than existing (" << currentVersion.value().ToString() << "), updating");
                    }
                }

                if (progress.IsCancelledBy(CancelReason::Any))
                {
                    AICLI_LOG(Repo, Info, << "Cancelling update upon request");
                    return UpdateResult::Aborted;
                }

                auto lock = store.Lock(progress, isBackground);
                if (!lock)
                {
                    // The lock says nothing about whether this form would have worked.
                    return UpdateResult::Aborted;
                }

                report.Reportable = true;

                return Acquire(store, updateCheck.PackageLocation(), progress, report);
            }

            // Acquires the index from a location that the caller has already determined, under a
            // lock that the caller already holds.
            UpdateResult Acquire(IPackageStore& store, const std::string& location, IProgressCallback& progress, UpdateReport& report)
            {
                auto acquired = store.Acquire(GetKey(), location, progress);
                if (!acquired)
                {
                    // Acquisition reports nothing only when it was cancelled.
                    return UpdateResult::Aborted;
                }

                report.DownloadedBytes = acquired->DownloadedBytes;

                store.Persist(std::move(acquired.value()), progress);

                return UpdateResult::Success;
            }

            SQLiteIndex Open(IPackageStore& store, IProgressCallback& progress) override
            {
                auto extracted = store.GetIndex(GetKey(), progress);

                if (!extracted)
                {
                    THROW_HR(APPINSTALLER_CLI_ERROR_SOURCE_DATA_MISSING);
                }

                return SQLiteIndex::Open(extracted->Path.u8string(), SQLiteIndex::OpenDisposition::Immutable, std::move(extracted->TemporaryFile));
            }

        private:
            PackageKey GetKey() const
            {
                THROW_HR_IF(E_NOT_VALID_STATE, m_identity.empty());
                return PackageKey{ PackageSlot::FullIndex, m_identity };
            }

            SourceDetails m_details;
            std::string m_identity;
        };
    }

    std::unique_ptr<IIndexForm> CreateFullIndexForm(const SourceDetails& details)
    {
        return std::make_unique<anon::FullIndexForm>(details);
    }
}
