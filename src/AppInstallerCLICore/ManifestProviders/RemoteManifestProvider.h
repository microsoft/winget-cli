// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ManifestProvider.h"
#include <filesystem>
#include <string>

namespace AppInstaller::CLI::Workflow
{
    struct RemoteManifestProvider : IManifestProvider
    {
        explicit RemoteManifestProvider(std::string_view argument);
        Settings::BoolAdminSetting RequiredAdminSetting() const override;
        std::optional<Manifest::Manifest> Load(Execution::Context& context) const override;

    private:
        std::filesystem::path Download(Execution::Context& context, std::filesystem::path& temporaryDirectory) const;
        std::string m_argument;
    };
}
