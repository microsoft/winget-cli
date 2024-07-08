// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Result.h"

#include <memory>
#include <string>

namespace SFS
{
class ContentId
{
  public:
    [[nodiscard]] static Result Make(std::string nameSpace,
                                     std::string name,
                                     std::string version,
                                     std::unique_ptr<ContentId>& out) noexcept;

    ContentId(ContentId&&) noexcept;

    ContentId(const ContentId&) = delete;
    ContentId& operator=(const ContentId&) = delete;

    /**
     * @return Content namespace
     */
    const std::string& GetNameSpace() const noexcept;

    /**
     * @return Content name
     */
    const std::string& GetName() const noexcept;

    /**
     * @return 4-part integer version. Each part can range from 0-65535
     */
    const std::string& GetVersion() const noexcept;

  private:
    ContentId() = default;

    std::string m_nameSpace;
    std::string m_name;
    std::string m_version;
};
} // namespace SFS
