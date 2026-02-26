// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "AppFile.h"
#include "Content.h"

#include <memory>
#include <string>
#include <vector>

namespace SFS
{
class AppPrerequisiteContent
{
  public:
    [[nodiscard]] static Result Make(std::unique_ptr<ContentId>&& contentId,
                                     std::vector<AppFile>&& files,
                                     std::unique_ptr<AppPrerequisiteContent>& out) noexcept;

    AppPrerequisiteContent(AppPrerequisiteContent&&) noexcept;

    AppPrerequisiteContent(const AppPrerequisiteContent&) = delete;
    AppPrerequisiteContent& operator=(const AppPrerequisiteContent&) = delete;

    /**
     * @return Unique content identifier
     */
    const ContentId& GetContentId() const noexcept;

    /**
     * @return Files belonging to this Prequisite
     */
    const std::vector<AppFile>& GetFiles() const noexcept;

  private:
    AppPrerequisiteContent() = default;

    std::unique_ptr<ContentId> m_contentId;
    std::vector<AppFile> m_files;
};

class AppContent
{
  public:
    [[nodiscard]] static Result Make(std::unique_ptr<ContentId>&& contentId,
                                     std::string updateId,
                                     std::vector<AppPrerequisiteContent>&& prerequisites,
                                     std::vector<AppFile>&& files,
                                     std::unique_ptr<AppContent>& out) noexcept;

    AppContent(AppContent&&) noexcept;

    AppContent(const AppContent&) = delete;
    AppContent& operator=(const AppContent&) = delete;

    /**
     * @return Unique content identifier
     */
    const ContentId& GetContentId() const noexcept;

    /**
     * @return Unique Update Id
     */
    const std::string& GetUpdateId() const noexcept;

    /**
     * @return Files belonging to this App
     */
    const std::vector<AppFile>& GetFiles() const noexcept;

    /**
     * @return List of Prerequisite content needed for this App. Prerequisites don't have further dependencies.
     */
    const std::vector<AppPrerequisiteContent>& GetPrerequisites() const noexcept;

  private:
    AppContent() = default;

    std::unique_ptr<ContentId> m_contentId;
    std::vector<AppFile> m_files;

    std::string m_updateId;
    std::vector<AppPrerequisiteContent> m_prerequisites;
};
} // namespace SFS
