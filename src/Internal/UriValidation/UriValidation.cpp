// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "UriValidation.h"

namespace AppInstaller::UriValidation
{
    UriValidationResult UriValidation(const std::string&)
    {
        return UriValidationResult::Allow;
    }
}
