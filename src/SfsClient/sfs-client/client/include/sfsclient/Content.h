// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "ContentId.h"
#include "File.h"
#include "Result.h"

#include <memory>
#include <string>
#include <vector>

namespace SFS
{
class Content
{
  public:
    /**
     * @brief This Make() method should be used when the caller wants the @param files to be cloned
     */
    [[nodiscard]] static Result Make(std::string contentNameSpace,
                                     std::string contentName,
                                     std::string contentVersion,
                                     const std::vector<File>& files,
                                     std::unique_ptr<Content>& out) noexcept;

    /**
     * @brief This Make() method should be used when the caller wants the @param files to be moved
     */
    [[nodiscard]] static Result Make(std::string contentNameSpace,
                                     std::string contentName,
                                     std::string contentVersion,
                                     std::vector<File>&& files,
                                     std::unique_ptr<Content>& out) noexcept;

    /**
     * @brief This Make() method should be used when the caller wants the @param contentId and @param files to be moved
     */
    [[nodiscard]] static Result Make(std::unique_ptr<ContentId>&& contentId,
                                     std::vector<File>&& files,
                                     std::unique_ptr<Content>& out) noexcept;

    Content(Content&&) noexcept;

    Content(const Content&) = delete;
    Content& operator=(const Content&) = delete;

    /**
     * @return Unique content identifier
     */
    const ContentId& GetContentId() const noexcept;

    const std::vector<File>& GetFiles() const noexcept;

  private:
    Content() = default;

    std::unique_ptr<ContentId> m_contentId;
    std::vector<File> m_files;
};
} // namespace SFS
