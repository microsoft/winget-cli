// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Microsoft/SQLiteIndex.h"
#include "Microsoft/SQLiteIndexSource.h"

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        // Populates the index with the ARP entries from the given root.
        void PopulateIndexFromARP(SQLiteIndex& index, HKEY rootKey)
        {
            UNREFERENCED_PARAMETER(index);
            UNREFERENCED_PARAMETER(rootKey);
        }

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
            manifest.Tags = { "msix" };

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
                manifest.Name = Utility::ConvertToUTF8(package.DisplayName());

                if (manifest.Name.empty())
                {
                    manifest.Name = Utility::ConvertToUTF8(packageId.Name());
                }

                std::ostringstream strstr;
                auto packageVersion = packageId.Version();
                strstr << packageVersion.Major << '.' << packageVersion.Minor << '.' << packageVersion.Build << '.' << packageVersion.Revision;

                manifest.Version = strstr.str();
                
                manifest.Installers[0].PackageFamilyName = familyName;

                // Use the family name as a unique key for the path
                index.AddManifest(manifest, std::filesystem::path{ packageId.FamilyName().c_str() });
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
                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::ARP_System)
                {
                    PopulateIndexFromARP(index, HKEY_LOCAL_MACHINE);
                }

                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::ARP_User)
                {
                    PopulateIndexFromARP(index, HKEY_CURRENT_USER);
                }

                if (filter == PredefinedInstalledSourceFactory::Filter::None || filter == PredefinedInstalledSourceFactory::Filter::MSIX)
                {
                    PopulateIndexFromMSIX(index);
                }

                return std::make_shared<SQLiteIndexSource>(details, "*PredefinedInstalledSource", std::move(index), Synchronization::CrossProcessReaderWriteLock{}, true);
            }

            void Add(SourceDetails&, IProgressCallback&) override final
            {
                // Add should never be needed, as this is predefined.
                THROW_HR(E_NOTIMPL);
            }

            void Update(const SourceDetails&, IProgressCallback&) override final
            {
                // Update could be used later, but not for now.
                THROW_HR(E_NOTIMPL);
            }

            void Remove(const SourceDetails&, IProgressCallback&) override final
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
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::ARP_System:
            return "ARP_System"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::ARP_User:
            return "ARP_User"sv;
        case AppInstaller::Repository::Microsoft::PredefinedInstalledSourceFactory::Filter::MSIX:
            return "MSIX"sv;
        default:
            return "Unknown"sv;
        }
    }

    PredefinedInstalledSourceFactory::Filter PredefinedInstalledSourceFactory::StringToFilter(std::string_view filter)
    {
        if (filter == FilterToString(Filter::ARP_System))
        {
            return Filter::ARP_System;
        }
        else if (filter == FilterToString(Filter::ARP_User))
        {
            return Filter::ARP_User;
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
