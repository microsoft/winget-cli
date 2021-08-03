// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/AppInstallerRepositorySource.h"
#include "SourceFactory.h"

#include <string_view>

namespace AppInstaller::Repository::Microsoft
{
    using namespace std::string_view_literals;
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

    private:
        static std::shared_ptr<ISource> g_sharedSource;
        static std::once_flag g_InstallingSourceOnceFlag;
    };

    // A source of installing packages on the local system.
    // Arg  ::  A value indicating the type of writeable source
    // Data ::  Not used.
    struct PredefinedWriteableSourceFactory
    {
        // Get the type string for this source.
        static constexpr std::string_view Type()
        {
            return "Microsoft.Predefined.Writeable"sv;
        }

        // The type for the source.
        enum class WriteableType
        {
            Installing
        };

        // Converts a type to its string.
        static std::string_view TypeToString(WriteableType type);

        // Creates a source factory for this type.
        static std::unique_ptr<ISourceFactory> Create();
    };
}
