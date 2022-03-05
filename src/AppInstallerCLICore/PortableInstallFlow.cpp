// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "PortableInstallFlow.h"

namespace AppInstaller::CLI::Workflow
{
    //bool WriteToAppPathsRegistrySubkey()
    //{

    //}

    void PortableInstallImpl(Execution::Context& context)
    {
        context.Reporter.Info() << Resource::String::InstallFlowStartingPackageInstall << std::endl;

        const std::filesystem::path& installerPath = context.Get<Execution::Data::InstallerPath>();

        // copy the filepath to the specified location
        //std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing);

        // write to the app path registry subkey
        // 

    }
}
