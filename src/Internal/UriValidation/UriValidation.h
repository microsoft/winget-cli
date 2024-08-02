// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::UriValidation
{
    enum class UriValidationResult
    {
        Allow,
        Block,
    };

    UriValidationResult UriValidation(const std::string& uri);
}
