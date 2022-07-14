// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wincrypt.h>


namespace AppInstaller::Certificates
{
    // Holds the details about how a certificate chain is to be validated (aka "pinned").
    struct PinningConfiguration
    {
        PinningConfiguration() = default;

        // Validates the given certificate against the configuration.
        // Returns true to indicate that the certificate meets the pinning configuration criteria.
        // Returns false to indicate the it does not.
        bool Validate(PCCERT_CONTEXT certContext) const;

    private:

    };
}
