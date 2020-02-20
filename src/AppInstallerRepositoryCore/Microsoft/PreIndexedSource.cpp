// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include "Microsoft/PreIndexedSource.h"

namespace AppInstaller::Repository::Microsoft
{
    namespace
    {
        struct PreIndexedSourceFactory : public ISourceFactory
        {
            std::unique_ptr<ISource> Create(const SourceDetails& details) override
            {
                UNREFERENCED_PARAMETER(details);
                THROW_HR(E_NOTIMPL);
            }

            void Update(SourceDetails& details) override
            {
                UNREFERENCED_PARAMETER(details);
                THROW_HR(E_NOTIMPL);
            }

            void Remove(const SourceDetails& details) override
            {
                UNREFERENCED_PARAMETER(details);
                THROW_HR(E_NOTIMPL);
            }
        };
    }

    std::unique_ptr<ISourceFactory> PreIndexedSource::CreateFactory()
    {
        return std::make_unique<PreIndexedSourceFactory>();
    }

    const SourceDetails& PreIndexedSource::GetDetails() const
    {
        THROW_HR(E_NOTIMPL);
    }

    SearchResult PreIndexedSource::Search(const SearchRequest& request) const
    {
        UNREFERENCED_PARAMETER(request);
        THROW_HR(E_NOTIMPL);
    }
}
