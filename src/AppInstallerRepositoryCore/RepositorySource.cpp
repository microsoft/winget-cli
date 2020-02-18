// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerRepositorySource.h"


namespace AppInstaller::Repository
{
    std::unique_ptr<ISource> AddSource(const std::string& name, const std::string& type, const std::string& arg)
    {
        UNREFERENCED_PARAMETER(name);
        UNREFERENCED_PARAMETER(type);
        UNREFERENCED_PARAMETER(arg);
        return {};
    }

    std::unique_ptr<ISource> OpenSource(const std::string& name)
    {
        UNREFERENCED_PARAMETER(name);
        return {};

    }

    std::vector<SourceDetails> GetSources()
    {
        return {};

    }

    void RemoveSource(const std::string& name)
    {
        UNREFERENCED_PARAMETER(name);

    }
}
