// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "AppInstallerHash.h"

namespace AppInstaller::Utility {

    // Binds the generic hash implementation to the SHA256 algorithm.
    // See AlgorithmHash for usage; create one and Add data to it if the data
    // is not all available, or simply call ComputeHash if the data is all in memory.
    class SHA256 final : public AlgorithmHash<HashAlgorithm::Sha256>
    {
    };
}
