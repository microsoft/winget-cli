// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "File.h"

#include "details/ErrorHandling.h"

using namespace SFS;

File::File(std::string&& fileId,
           std::string&& url,
           uint64_t sizeInBytes,
           std::unordered_map<HashType, std::string>&& hashes)
    : m_fileId(std::move(fileId))
    , m_url(std::move(url))
    , m_sizeInBytes(sizeInBytes)
    , m_hashes(std::move(hashes))
{
}

Result File::Make(std::string fileId,
                  std::string url,
                  uint64_t sizeInBytes,
                  std::unordered_map<HashType, std::string> hashes,
                  std::unique_ptr<File>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<File> tmp(new File(std::move(fileId), std::move(url), sizeInBytes, std::move(hashes)));
    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

Result File::Clone(std::unique_ptr<File>& out) const noexcept
{
    return Make(m_fileId, m_url, m_sizeInBytes, m_hashes, out);
}

File::File(File&& other) noexcept
{
    m_fileId = std::move(other.m_fileId);
    m_url = std::move(other.m_url);
    m_sizeInBytes = other.m_sizeInBytes;
    m_hashes = std::move(other.m_hashes);
}

const std::string& File::GetFileId() const noexcept
{
    return m_fileId;
}

const std::string& File::GetUrl() const noexcept
{
    return m_url;
}

uint64_t File::GetSizeInBytes() const noexcept
{
    return m_sizeInBytes;
}

const std::unordered_map<HashType, std::string>& File::GetHashes() const noexcept
{
    return m_hashes;
}
