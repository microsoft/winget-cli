// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "UriValidation.h"

namespace AppInstaller::UriValidation
{
    namespace
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        bool EndsWith(const std::string& value, const std::string& ending)
        {
            if (ending.size() > value.size())
            {
                return false;
            }

            return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
        }
#endif
    }

    UriValidationResult ValidateUri(const std::string& uri)
    {
#ifndef AICLI_DISABLE_TEST_HOOKS
        // For testing purposes, block all URIs that end with "/block"
        if (EndsWith(uri, "/block"))
        {
            return UriValidationResult(UriValidationDecision::Block, std::string());
        }
#endif

        // In Dev mode, allow all URIs
        return UriValidationResult(UriValidationDecision::Allow, std::string());
    }
}
