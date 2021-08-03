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
    std::shared_ptr<ISource> PredefinedWriteableSourceFactoryImpl::g_sharedSource = nullptr;
    std::once_flag PredefinedWriteableSourceFactoryImpl::g_InstallingSourceOnceFlag;

    std::shared_ptr<ISource> PredefinedWriteableSourceFactoryImpl::Create(const SourceDetails& details, IProgressCallback& progress)
    {
        UNREFERENCED_PARAMETER(progress);

        THROW_HR_IF(E_INVALIDARG, details.Type != PredefinedWriteableSourceFactory::Type());

        std::string lockName = "WriteableSource_";

        // Installing is the only type right now so just return the same source to all callers.
        std::call_once(g_InstallingSourceOnceFlag,
            [&]()
            {
                // Create an in memory index
                SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, Schema::Version::Latest());

                g_sharedSource = std::make_shared<SQLiteIndexWriteableSource>(details, "*PredefinedWriteableSource", std::move(index), Synchronization::CrossProcessReaderWriteLock{}, true);
                return g_sharedSource;
            });
        return g_sharedSource;
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
