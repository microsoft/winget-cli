// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "SFSException.h"

using namespace SFS::details;

SFSException::SFSException(SFS::Result result) : m_result(std::move(result))
{
}

SFSException::SFSException(SFS::Result::Code code) : m_result(Result(code))
{
}

SFSException::SFSException(SFS::Result::Code code, std::string message) : m_result(Result(code, std::move(message)))
{
}

const SFS::Result& SFSException::GetResult() const noexcept
{
    return m_result;
}

const char* SFSException::what() const noexcept
{
    return m_result.GetMsg().c_str();
}
