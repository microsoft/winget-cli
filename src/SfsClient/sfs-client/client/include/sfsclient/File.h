// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Result.h"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace SFS
{
enum class HashType
{
    Sha1,
    Sha256
};

class File
{
  public:
    [[nodiscard]] static Result Make(std::string fileId,
                                     std::string url,
                                     uint64_t sizeInBytes,
                                     std::unordered_map<HashType, std::string> hashes,
                                     std::unique_ptr<File>& out) noexcept;

    File(File&&) noexcept;

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    /**
     * @return Unique file identifier within a content version
     */
    const std::string& GetFileId() const noexcept;

    /**
     * @return Download URL
     */
    const std::string& GetUrl() const noexcept;

    /**
     * @return File size in number of bytes
     */
    uint64_t GetSizeInBytes() const noexcept;

    /**
     * @return Dictionary of algorithm type to base64 encoded file hash string
     */
    const std::unordered_map<HashType, std::string>& GetHashes() const noexcept;

  protected:
    File(std::string&& fileId,
         std::string&& url,
         uint64_t sizeInBytes,
         std::unordered_map<HashType, std::string>&& hashes);

    [[nodiscard]] Result Clone(std::unique_ptr<File>& out) const noexcept;

    friend class Content;

    std::string m_fileId;
    std::string m_url;
    uint64_t m_sizeInBytes;
    std::unordered_map<HashType, std::string> m_hashes;
};
} // namespace SFS
