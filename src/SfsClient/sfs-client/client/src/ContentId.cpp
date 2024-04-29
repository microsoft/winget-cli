// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentId.h"

#include "details/ErrorHandling.h"

using namespace SFS;

Result ContentId::Make(std::string nameSpace,
                       std::string name,
                       std::string version,
                       std::unique_ptr<ContentId>& out) noexcept
try
{
    out.reset();

    std::unique_ptr<ContentId> tmp(new ContentId());
    tmp->m_nameSpace = std::move(nameSpace);
    tmp->m_name = std::move(name);
    tmp->m_version = std::move(version);

    out = std::move(tmp);

    return Result::Success;
}
SFS_CATCH_RETURN()

ContentId::ContentId(ContentId&& other) noexcept
{
    m_nameSpace = std::move(other.m_nameSpace);
    m_name = std::move(other.m_name);
    m_version = std::move(other.m_version);
}

const std::string& ContentId::GetNameSpace() const noexcept
{
    return m_nameSpace;
}

const std::string& ContentId::GetName() const noexcept
{
    return m_name;
}

const std::string& ContentId::GetVersion() const noexcept
{
    return m_version;
}
