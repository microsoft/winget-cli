// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/PreIndexed/PackageStore.h"

#include <AppInstallerDeployment.h>
#include <AppInstallerMsixInfo.h>

namespace AppInstaller::Repository::Microsoft::PreIndexed
{
    namespace
    {
        // Holds a source's packages by registering them with the platform.
        struct DeployedPackageStore : public PackageStoreBase
        {
            using PackageStoreBase::PackageStoreBase;

            void Persist(AcquiredPackage&& package, IProgressCallback& progress) override
            {
                Deployment::Options options{ IsTrusted() };

                // The file lock is deliberately held across deployment, so that the package the
                // platform reads is the one that was validated.
                winrt::Windows::Foundation::Uri uri = winrt::Windows::Foundation::Uri(package.Path.c_str());
                Deployment::AddPackage(
                    uri,
                    options,
                    progress);

                // Nothing further needs the file; it is removed when the package goes out of scope.
            }

            std::optional<Msix::PackageVersion> GetVersion(const PackageKey& package) const override
            {
                auto extension = GetExtension(package);

                if (!extension)
                {
                    return std::nullopt;
                }

                auto version = extension->GetPackageVersion();
                return Msix::PackageVersion{ version.Major, version.Minor, version.Build, version.Revision };
            }

            std::optional<ExtractedIndex> GetIndex(const PackageKey& package, IProgressCallback& progress) override
            {
                auto extension = GetExtension(package);

                if (!extension)
                {
                    AICLI_LOG(Repo, Info, << "Deployed " << GetSlotName(package.Slot) << " package not found: " << package.Identity);
                    return std::nullopt;
                }

                THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_NEEDS_REMEDIATION), !extension->VerifyContentIntegrity(progress));

                ExtractedIndex result;

                // To work around an issue with accessing the public folder, we are temporarily
                // constructing the location ourself. This was already the case for the local file
                // mechanism, and we can fix both in the future. The only problem with this is that
                // the directory in the extension *must* be Public, rather than one set by the
                // creator.
                result.Path = extension->GetPackagePath();
                result.Path /= s_IndexFilePath;

                return std::optional<ExtractedIndex>{ std::move(result) };
            }

            void Remove(const std::vector<PackageKey>& packages, IProgressCallback& progress) override
            {
                for (const auto& package : packages)
                {
                    auto fullName = Msix::GetPackageFullNameFromFamilyName(package.Identity);

                    if (!fullName)
                    {
                        AICLI_LOG(Repo, Info, << "No full name found for family name: " << package.Identity);
                    }
                    else
                    {
                        AICLI_LOG(Repo, Info, << "Removing package: " << *fullName);
                        Deployment::RemovePackage(*fullName, winrt::Windows::Management::Deployment::RemovalOptions::None, progress);
                    }
                }
            }

            // The deployed index is read in place, so a read can be attempted before taking the
            // lock and retried under it only when that fails.
            bool AllowsUnlockedRead() const override { return true; }

        private:
            std::optional<Deployment::Extension> GetExtension(const PackageKey& package) const
            {
                THROW_HR_IF(E_UNEXPECTED, package.Identity.empty());
                Deployment::ExtensionCatalog catalog(Deployment::SourceExtensionName);
                return catalog.FindByPackageFamilyAndId(package.Identity, Deployment::IndexDBId);
            }
        };
    }

    std::unique_ptr<IPackageStore> CreateDeployedPackageStore(const SourceDetails& details)
    {
        return std::make_unique<DeployedPackageStore>(details);
    }
}
