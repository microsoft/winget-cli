// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ApplicabilityDetails.h"

#include "details/ErrorHandling.h"

using namespace SFS;

Result ApplicabilityDetails::Make(std::vector<Architecture> architectures,
                                  std::vector<std::string> platformApplicabilityForPackage,
                                  std::unique_ptr<ApplicabilityDetails>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<ApplicabilityDetails> tmp(new ApplicabilityDetails());
    tmp->m_architectures = std::move(architectures);
    tmp->m_platformApplicabilityForPackage = std::move(platformApplicabilityForPackage);

    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

const std::vector<Architecture>& ApplicabilityDetails::GetArchitectures() const noexcept
{
    return m_architectures;
}

const std::vector<std::string>& ApplicabilityDetails::GetPlatformApplicabilityForPackage() const noexcept
{
    return m_platformApplicabilityForPackage;
}
