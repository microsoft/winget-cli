// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winget/AdminSettings.h>
#include <winget/Manifest.h>
#include <memory>
#include <optional>
#include <string_view>

namespace AppInstaller::CLI::Execution
{
    struct Context;
}

namespace AppInstaller::CLI::Workflow
{
    struct IManifestProvider
    {
        virtual ~IManifestProvider() = default;
        virtual Settings::BoolAdminSetting RequiredAdminSetting() const = 0;
        // Returns no manifest when loading terminates the context.
        virtual std::optional<Manifest::Manifest> Load(Execution::Context& context) const = 0;
    };

    std::unique_ptr<IManifestProvider> CreateManifestProvider(std::string_view argument);
}
