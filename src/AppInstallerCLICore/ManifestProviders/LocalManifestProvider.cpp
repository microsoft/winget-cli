// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "LocalManifestProvider.h"
#include "ManifestArchive.h"
#include "ExecutionContext.h"
#include "Workflows/WorkflowBase.h"
#include <winget/ManifestYamlParser.h>

namespace AppInstaller::CLI::Workflow
{
    LocalManifestProvider::LocalManifestProvider(std::string_view argument) : m_argument(argument) {}

    Settings::BoolAdminSetting LocalManifestProvider::RequiredAdminSetting() const
    {
        return Settings::BoolAdminSetting::LocalManifestFiles;
    }

    std::optional<Manifest::Manifest> LocalManifestProvider::Load(Execution::Context& context) const
    {
        Logging::Telemetry().LogIsManifestLocal(true);
        context << VerifyPath(Execution::Args::Type::Manifest);
        if (context.IsTerminated())
        {
            return std::nullopt;
        }
        std::filesystem::path manifestPath{ Utility::ConvertToUTF16(m_argument) };
        if (std::filesystem::is_regular_file(manifestPath) &&
            Utility::ToLower(manifestPath.extension().u8string()) == ".zip")
        {
            return LoadManifestArchive(std::filesystem::absolute(manifestPath));
        }
        return Manifest::YamlParser::CreateFromPath(manifestPath);
    }
}
