// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include <AppInstallerRuntime.h>
#include "ResumeFlow.h"
#include "winget/Reboot.h"

namespace AppInstaller::CLI::Workflow
{
    void Checkpoint::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Resume))
        {
            return;
        }

        context.Checkpoint(m_checkpointName, m_contextData);
    }

    void RegisterStartupAfterReboot::operator()(Execution::Context& context) const
    {
        if (!Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Resume) || 
            !Settings::ExperimentalFeature::IsEnabled(Settings::ExperimentalFeature::Feature::Reboot))
        {
            return;
        }

        if (WI_IsFlagSet(context.GetFlags(), Execution::ContextFlag::RegisterResume))
        {
            int argc = 0;
            wil::unique_hlocal_ptr<LPWSTR> argv{ CommandLineToArgvW(GetCommandLineW(), &argc) };
            THROW_LAST_ERROR_IF_NULL(argv);

            // RunOnce registry value must start with the full path of the executable.
            std::string commandLine = Utility::ConvertToUTF8(argv.get()[0]) + " resume -g " + context.GetResumeId();

            bool isAdmin = AppInstaller::Runtime::IsRunningAsAdmin();
            Reboot::WriteToRunOnceRegistry(commandLine, isAdmin);
        }
    }
}
