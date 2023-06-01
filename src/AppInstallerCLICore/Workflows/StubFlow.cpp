// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "StubFlow.h"
#include "winget/SelfManagement.h"

namespace AppInstaller::CLI::Workflow
{
    using namespace AppInstaller::SelfManagement;
    using namespace AppInstaller::Utility::literals;

    void VerifyStubSupport(Execution::Context& context)
    {
        if (!IsStubPreferenceSupported())
        {
            // TODO
            context.Reporter.Warn() << "Not supported"_liv << std::endl;
            AICLI_TERMINATE_CONTEXT(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
        }
    }

    void AppInstallerStubPreferred(Execution::Context& context)
    {
        SetStubPreferred(true);

        // TODO: check if file doesn't exists or if there's a way to know it is a stub.
        context.Reporter.Info() << "Set to stub package"_liv << std::endl;
    }

    void AppInstallerFullPreferred(Execution::Context& context)
    {
        SetStubPreferred(false);

        // TODO: check if file exists or if there's a way to know it is a stub.
        context.Reporter.Info() << "Set to full package"_liv << std::endl;
    }
}
