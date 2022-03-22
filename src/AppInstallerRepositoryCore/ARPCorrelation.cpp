// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "winget/ARPCorrelation.h"

namespace AppInstaller::Repository::Correlation
{
    double NoMatching::GetMatchingScore(
        const Manifest::Manifest& manifest,
        std::shared_ptr<IPackage> arpEntry) const
    {
        return 0;
    }

    double NoMatch::GetMatchingThreshold() const
    {
        return 1;
    }
}