// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/ARPHelper.h"
#include "Microsoft/PredefinedWriteableSourceFactory.h"
#include "Microsoft/SQLiteIndex.h"
#include "Microsoft/SQLiteIndexSource.h"
#include <winget/ManifestInstaller.h>

#include <winget/Registry.h>
#include <AppInstallerArchitecture.h>

using namespace std::string_literals;
using namespace std::string_view_literals;

namespace AppInstaller::Repository::Microsoft
{
    // The factory for the predefined installing source.
    struct PredefinedWriteableSourceFactoryImpl : public ISourceFactory
    {
        std::shared_ptr<ISource> Create(const SourceDetails& details, IProgressCallback& progress) override final;

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

    std::shared_ptr<ISource> PredefinedWriteableSourceFactoryImpl::Create(const SourceDetails& details, IProgressCallback&)
    {
        THROW_HR_IF(E_INVALIDARG, details.Type != PredefinedWriteableSourceFactory::Type());

        std::string lockName = "WriteableSource_";

        // Create an in memory index
        SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, Schema::Version::Latest());

        std::shared_ptr<ISource> sharedSource = std::make_shared<SQLiteIndexWriteableSource>(details, "*PredefinedWriteableSource", std::move(index), Synchronization::CrossProcessReaderWriteLock{}, true);
        return sharedSource;
    }

    std::string_view PredefinedWriteableSourceFactory::TypeToString(WriteableType type)
    {
        switch (type)
        {
        case AppInstaller::Repository::Microsoft::PredefinedWriteableSourceFactory::WriteableType::Installing:
            return "Installing"sv;
        default:
            return "Unknown"sv;
        }
    }

    std::unique_ptr<ISourceFactory> PredefinedWriteableSourceFactory::Create()
    {
        return std::make_unique<PredefinedWriteableSourceFactoryImpl>();
    }
}
