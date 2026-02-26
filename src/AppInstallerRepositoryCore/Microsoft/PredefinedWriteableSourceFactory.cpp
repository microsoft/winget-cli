// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
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
    namespace
    {
        // The factory for the predefined installing source.
        struct PredefinedWriteableSourceFactoryImpl : public ISourceFactory
        {
            std::string_view TypeName() const override final
            {
                return PredefinedWriteableSourceFactory::Type();
            }

            std::shared_ptr<ISourceReference> Create(const SourceDetails& details) override final;

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

        struct PredefinedWriteableSourceReference : public ISourceReference
        {
            PredefinedWriteableSourceReference(const SourceDetails& details) : m_details(details)
            {
                m_details.Identifier = "*PredefinedWriteableSource";
            }

            std::string GetIdentifier() override { return m_details.Identifier; }

            SourceDetails& GetDetails() override { return m_details; };

            std::shared_ptr<ISource> Open(IProgressCallback&) override
            {
                // Installing is the only type right now so just return the Installing source to all callers.
                // Since the source is writeable, it must be shared by all callers that try to open it
                // since queries on one instance would not see what was written on another instance.
                std::call_once(g_InstallingSourceOnceFlag,
                    [&]()
                    {
                        // Create an in memory index without paths or dependencies
                        SQLiteIndex index = SQLiteIndex::CreateNew(SQLITE_MEMORY_DB_CONNECTION_TARGET, SQLite::Version::Latest(), SQLiteIndex::CreateOptions::SupportPathless | SQLiteIndex::CreateOptions::DisableDependenciesSupport);

                        g_sharedSource = std::make_shared<SQLiteIndexWriteableSource>(m_details, std::move(index), true);
                    });

                return g_sharedSource;
            }

        private:
            SourceDetails m_details;
            static std::shared_ptr<ISource> g_sharedSource;
            static std::once_flag g_InstallingSourceOnceFlag;
        };

        std::shared_ptr<ISource> PredefinedWriteableSourceReference::g_sharedSource = nullptr;
        std::once_flag PredefinedWriteableSourceReference::g_InstallingSourceOnceFlag;

        std::shared_ptr<ISourceReference> PredefinedWriteableSourceFactoryImpl::Create(const SourceDetails& details)
        {
            THROW_HR_IF(E_INVALIDARG, details.Type != PredefinedWriteableSourceFactory::Type());

            return std::make_shared<PredefinedWriteableSourceReference>(details);
        }
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
