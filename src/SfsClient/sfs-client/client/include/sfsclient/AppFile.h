// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ApplicabilityDetails.h"
#include "File.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace SFS
{
class AppFile : private File
{
  public:
    [[nodiscard]] static Result Make(std::string fileId,
                                     std::string url,
                                     uint64_t sizeInBytes,
                                     std::unordered_map<HashType, std::string> hashes,
                                     std::vector<Architecture> architectures,
                                     std::vector<std::string> platformApplicabilityForPackage,
                                     std::string fileMoniker,
                                     std::unique_ptr<AppFile>& out) noexcept;

    AppFile(AppFile&&) noexcept;

    AppFile(const AppFile&) = delete;
    AppFile& operator=(const AppFile&) = delete;

    /// Getter methods from File class
    using File::GetFileId;
    using File::GetHashes;
    using File::GetSizeInBytes;
    using File::GetUrl;

    /**
     * @return Set of details related to applicability of the file
     */
    const ApplicabilityDetails& GetApplicabilityDetails() const noexcept;

    /**
     * @return Package Moniker of the file
     */
    const std::string& GetFileMoniker() const noexcept;

  private:
    AppFile(std::string&& fileId,
            std::string&& url,
            uint64_t sizeInBytes,
            std::unordered_map<HashType, std::string>&& hashes,
            std::string&& fileMoniker);

    std::unique_ptr<ApplicabilityDetails> m_applicabilityDetails;
    std::string m_fileMoniker;
};
} // namespace SFS
