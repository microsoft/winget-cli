// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::Reboot
{
    bool HasRebootPrivilege();

    bool InitiateReboot();
}