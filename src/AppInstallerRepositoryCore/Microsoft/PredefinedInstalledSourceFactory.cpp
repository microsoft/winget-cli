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
            Manifest::Manifest manifest;
            manifest.Id = "test";
            manifest.Name = "Test!";
            manifest.Version = "1.2.3";

            index.AddManifest(manifest, "nope");
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
            return Filter::ARP_User;
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
