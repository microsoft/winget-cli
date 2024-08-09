// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Content.h"

#include "details/ErrorHandling.h"

using namespace SFS;

Result Content::Make(std::string contentNameSpace,
                     std::string contentName,
                     std::string contentVersion,
                     const std::vector<File>& files,
                     std::unique_ptr<Content>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<Content> tmp(new Content());
    RETURN_IF_FAILED(ContentId::Make(std::move(contentNameSpace),
                                     std::move(contentName),
                                     std::move(contentVersion),
                                     tmp->m_contentId));

    for (const auto& file : files)
    {
        std::unique_ptr<File> clone;
        RETURN_IF_FAILED(file.Clone(clone));
        tmp->m_files.push_back(std::move(*clone));
    }

    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

Result Content::Make(std::string contentNameSpace,
                     std::string contentName,
                     std::string contentVersion,
                     std::vector<File>&& files,
                     std::unique_ptr<Content>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<Content> tmp(new Content());
    RETURN_IF_FAILED(ContentId::Make(std::move(contentNameSpace),
                                     std::move(contentName),
                                     std::move(contentVersion),
                                     tmp->m_contentId));
    tmp->m_files = std::move(files);

    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

Result Content::Make(std::unique_ptr<ContentId>&& contentId,
                     std::vector<File>&& files,
                     std::unique_ptr<Content>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<Content> tmp(new Content());
    tmp->m_contentId = std::move(contentId);
    tmp->m_files = std::move(files);

    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

Content::Content(Content&& other) noexcept
{
    m_contentId = std::move(other.m_contentId);
    m_files = std::move(other.m_files);
}

const ContentId& Content::GetContentId() const noexcept
{
    return *m_contentId;
}

const std::vector<File>& Content::GetFiles() const noexcept
{
    return m_files;
}
