// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::Reboot
{
    bool InitiateReboot();

    bool RegisterApplicationForReboot(const std::string& commandLineArgs);
}