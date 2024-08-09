// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "UriValidation.h"

namespace AppInstaller::UriValidation
{
    UriValidationResult UriValidation(const std::string&)
    {
        // In Dev mode, allow all URIs
        return UriValidationResult(UriValidationDecision::Allow, std::string());
    }
}
