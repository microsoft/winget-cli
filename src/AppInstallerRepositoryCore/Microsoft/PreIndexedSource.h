// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/AppInstallerRepositorySource.h"
#include "SourceFactory.h"

#include <string_view>

namespace AppInstaller::Repository::Microsoft
{
    // A source where the index is precomputed and stored on a server within an optional MSIX package.
    // In addition, the manifest files are also individually available on the server.
    struct PreIndexedSource : public ISource
    {
        PreIndexedSource(const SourceDetails& details);

        PreIndexedSource(const PreIndexedSource&) = delete;
        PreIndexedSource& operator=(const PreIndexedSource&) = delete;

        PreIndexedSource(PreIndexedSource&&) = default;
        PreIndexedSource& operator=(PreIndexedSource&&) = default;

        // Get the type string for this source.
        static constexpr std::string_view Type()
        {
            using namespace std::string_view_literals;
            return "Microsoft.PreIndexed"sv;
        }

        // Creates a source factory for this type.
        static std::unique_ptr<ISourceFactory> CreateFactory();

        // ISource

        // Get the source's details.
        const SourceDetails& GetDetails() const override;

        // Execute a search on the source.
        SearchResult Search(const SearchRequest& request) const override;

    private:
        SourceDetails m_details;
    };
}
