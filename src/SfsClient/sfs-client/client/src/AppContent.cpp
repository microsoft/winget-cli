// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "AppContent.h"

#include "details/ErrorHandling.h"

using namespace SFS;

Result AppPrerequisiteContent::Make(std::unique_ptr<ContentId>&& contentId,
                                    std::vector<AppFile>&& files,
                                    std::unique_ptr<AppPrerequisiteContent>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<AppPrerequisiteContent> tmp(new AppPrerequisiteContent());
    tmp->m_contentId = std::move(contentId);
    tmp->m_files = std::move(files);

    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

AppPrerequisiteContent::AppPrerequisiteContent(AppPrerequisiteContent&& other) noexcept
{
    m_contentId = std::move(other.m_contentId);
    m_files = std::move(other.m_files);
}

const ContentId& AppPrerequisiteContent::GetContentId() const noexcept
{
    return *m_contentId;
}

const std::vector<AppFile>& AppPrerequisiteContent::GetFiles() const noexcept
{
    return m_files;
}

Result AppContent::Make(std::unique_ptr<ContentId>&& contentId,
                        std::string updateId,
                        std::vector<AppPrerequisiteContent>&& prerequisites,
                        std::vector<AppFile>&& files,
                        std::unique_ptr<AppContent>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<AppContent> tmp(new AppContent());
    tmp->m_contentId = std::move(contentId);
    tmp->m_updateId = std::move(updateId);
    tmp->m_prerequisites = std::move(prerequisites);
    tmp->m_files = std::move(files);

    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

AppContent::AppContent(AppContent&& other) noexcept
{
    m_contentId = std::move(other.m_contentId);
    m_updateId = std::move(other.m_updateId);
    m_prerequisites = std::move(other.m_prerequisites);
    m_files = std::move(other.m_files);
}

const ContentId& AppContent::GetContentId() const noexcept
{
    return *m_contentId;
}

const std::string& AppContent::GetUpdateId() const noexcept
{
    return m_updateId;
}

const std::vector<AppFile>& AppContent::GetFiles() const noexcept
{
    return m_files;
}

const std::vector<AppPrerequisiteContent>& AppContent::GetPrerequisites() const noexcept
{
    return m_prerequisites;
}
