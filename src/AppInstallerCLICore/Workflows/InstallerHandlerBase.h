// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include <string>
#include "Manifest/Manifest.h"
#include "ExecutionContext.h"

namespace AppInstaller::Workflow
{
    using namespace std::string_view_literals;

    // Token specified in installer args will be replaced by proper value.
    static constexpr std::string_view ARG_TOKEN_LOGPATH = "<LOGPATH>"sv;
    static constexpr std::string_view ARG_TOKEN_INSTALLPATH = "<INSTALLPATH>"sv;

    // This is the base class for installer handlers. Individual installer handler should override
    // member methods to do appropriate work on different installers.
    class InstallerHandlerBase
    {
    public:

        // The Download method downloads installer to local temp folder.
        // The downloaded installer does not have any extension appended.
        // SHA256 of the downloaded installer is verified during download.
        virtual void Download();

        virtual void Install() { THROW_HR(E_NOTIMPL); }
        virtual void Cancel() { THROW_HR(E_NOTIMPL); }

    protected:
        InstallerHandlerBase(
            const Manifest::ManifestInstaller& manifestInstaller,
            AppInstaller::CLI::Execution::Context& context) :
            m_manifestInstallerRef(manifestInstaller), m_reporterRef(context.Reporter), m_argsRef(context.Args) {};

        const Manifest::ManifestInstaller& m_manifestInstallerRef;
        const AppInstaller::CLI::Execution::Args& m_argsRef;
        AppInstaller::CLI::Execution::Reporter& m_reporterRef;
        std::filesystem::path m_downloadedInstaller;
    };
}

