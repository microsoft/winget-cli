// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ManifestProvider.h"
#include <string>

namespace AppInstaller::CLI::Workflow
{
    struct LocalManifestProvider : IManifestProvider
    {
        explicit LocalManifestProvider(std::string_view argument);
        Settings::BoolAdminSetting RequiredAdminSetting() const override;
        std::optional<Manifest::Manifest> Load(Execution::Context& context) const override;

    private:
        std::string m_argument;
    };
}
