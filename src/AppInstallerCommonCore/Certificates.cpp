// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/Certificates.h"
#include "Public/AppInstallerLogging.h"
#include "Public/AppInstallerStrings.h"

namespace AppInstaller::Certificates
{
    bool PinningConfiguration::Validate(PCCERT_CONTEXT certContext) const
    {
        //  the root cert if fully pinned but leaf & intermediary are only SN+Issuer checks
        THROW_HR(E_NOTIMPL);
    }
}
