// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "AppFile.h"

#include "details/ErrorHandling.h"

using namespace SFS;

AppFile::AppFile(std::string&& fileId,
                 std::string&& url,
                 uint64_t sizeInBytes,
                 std::unordered_map<HashType, std::string>&& hashes,
                 std::string&& fileMoniker)
    : File(std::move(fileId), std::move(url), sizeInBytes, std::move(hashes))
    , m_fileMoniker(std::move(fileMoniker))
{
}

Result AppFile::Make(std::string fileId,
                     std::string url,
                     uint64_t sizeInBytes,
                     std::unordered_map<HashType, std::string> hashes,
                     std::vector<Architecture> architectures,
                     std::vector<std::string> platformApplicabilityForPackage,
                     std::string fileMoniker,
                     std::unique_ptr<AppFile>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<ApplicabilityDetails> details;
    RETURN_IF_FAILED(
        ApplicabilityDetails::Make(std::move(architectures), std::move(platformApplicabilityForPackage), details));

    std::unique_ptr<AppFile> tmp(
        new AppFile(std::move(fileId), std::move(url), sizeInBytes, std::move(hashes), std::move(fileMoniker)));
    tmp->m_applicabilityDetails = std::move(details);

    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

AppFile::AppFile(AppFile&& other) noexcept : File(std::move(other))
{
    m_applicabilityDetails = std::move(other.m_applicabilityDetails);
    m_fileMoniker = std::move(other.m_fileMoniker);
}

const ApplicabilityDetails& AppFile::GetApplicabilityDetails() const noexcept
{
    return *m_applicabilityDetails;
}

const std::string& AppFile::GetFileMoniker() const noexcept
{
    return m_fileMoniker;
}
