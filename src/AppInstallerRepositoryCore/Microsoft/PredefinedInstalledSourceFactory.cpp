// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
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
        // Populates the index with the entries from MSIX.
        void PopulateIndexFromMSIX(SQLiteIndex& index)
        {
            using namespace winrt::Windows::ApplicationModel;
            using namespace winrt::Windows::Management::Deployment;

            // TODO: Consider if Optional packages should also be enumerated
            PackageManager packageManager;
            auto packages = packageManager.FindPackagesForUserWithPackageTypes({}, PackageTypes::Main);

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

                bool isPackageNameSet = false;
                // Attempt to get the DisplayName. Since this will retrieve the localized value, it has a chance to fail.
                // Rather than completely skip this package in that case, we will simply fall back to using the package name below.
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
                    AICLI_LOG(Repo, Info, << "winrt::hresult_error[0x" << Logging::SetHRFormat << hre.code() << ": " <<
                        Utility::ConvertToUTF8(hre.message()) << "] exception thrown when getting DisplayName for " << familyName);
                }
                catch (...)
                {
                    AICLI_LOG(Repo, Info, << "Unknown exception thrown when getting DisplayName for " << familyName);
                }

                if (!isPackageNameSet)
                {
                    manifest.DefaultLocalization.Add<Manifest::Localization::PackageName>(Utility::ConvertToUTF8(packageId.Name()));
                }

                std::ostringstream strstr;
                auto packageVersion = packageId.Version();
                strstr << packageVersion.Major << '.' << packageVersion.Minor << '.' << packageVersion.Build << '.' << packageVersion.Revision;

                manifest.Version = strstr.str();
                
                manifest.Installers[0].PackageFamilyName = familyName;

                // Use the family name as a unique key for the path
                auto manifestId = index.AddManifest(manifest, std::filesystem::path{ packageId.FamilyName().c_str() });

                index.SetMetadataByManifestId(manifestId, PackageVersionMetadata::InstalledType, 
                    Manifest::InstallerTypeToString(Manifest::InstallerTypeEnum::Msix));
            }
        }

        // The factory for the predefined installed source.
        struct Factory : public ISourceFactory
        {
            std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback& progress) override final
            {
                // TODO: Maybe we do need to use it?
                UNREFERENCED_PARAMETER(progress);

                THROW_HR_IF(E_INVALIDARG, details.Type != PredefinedInstalledSourceFactory::Type());

                // Determine the filter
                PredefinedInstalledSourceFactory::Filter filter = PredefinedInstalledSourceFactory::StringToFilter(details.Arg);
                AICLI_LOG(Repo, Info, << "Creating PredefinedInstalledSource with filter [" << PredefinedInstalledSourceFactory::FilterToString(filter) << ']');

                // Create an in memory index
                SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, Schema::Version::Latest());

                // Put installed packages into the index
                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::ARP)
                {
                    ARPHelper arpHelper;
                    arpHelper.PopulateIndexFromARP(index, Manifest::ScopeEnum::Machine);
                    arpHelper.PopulateIndexFromARP(index, Manifest::ScopeEnum::User);
                }

                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::MSIX)
                {
                    PopulateIndexFromMSIX(index);
                }

                return std::make_shared<SQLiteIndexSource>(details, "*PredefinedInstalledSource", std::move(index), Synchronization::CrossProcessReaderWriteLock{}, true);
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
