// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "SFSExceptionMatcher.h"

#include <catch2/matchers/catch_matchers_string.hpp>

using namespace SFS;
using namespace SFS::details;
using namespace SFS::test;

SFSExceptionMatcher::SFSExceptionMatcher(const Result::Code& code) : m_code(code)
{
}

SFSExceptionMatcher::SFSExceptionMatcher(const Result::Code& code, std::string message)
    : m_code(code)
    , m_message(std::move(message))
{
}

bool SFSExceptionMatcher::match(const SFSException& other) const
{
    if (m_message)
    {
        return other.GetResult() == m_code && other.what() == m_message;
    }
    return other.GetResult() == m_code;
}

std::string SFSExceptionMatcher::describe() const
{
    return "Equals: " + std::string(ToString(m_code)) + (m_message ? " with message: " + *m_message : "");
}

template <typename Matcher>
SFSExceptionGenericMatcher<Matcher>::SFSExceptionGenericMatcher(const Result::Code& code, Matcher messageMatcher)
    : m_code(code)
    , m_messageMatcher(std::move(messageMatcher))
{
}

template <typename Matcher>
bool SFSExceptionGenericMatcher<Matcher>::match(const SFSException& other) const
{
    return other.GetResult() == m_code && m_messageMatcher.match(other.what());
}

template <typename Matcher>
std::string SFSExceptionGenericMatcher<Matcher>::describe() const
{
    return "Equals: " + std::string(ToString(m_code)) + m_messageMatcher.describe();
}

template struct SFS::test::SFSExceptionGenericMatcher<Catch::Matchers::StringEqualsMatcher>;
template struct SFS::test::SFSExceptionGenericMatcher<Catch::Matchers::StringContainsMatcher>;
template struct SFS::test::SFSExceptionGenericMatcher<Catch::Matchers::EndsWithMatcher>;
template struct SFS::test::SFSExceptionGenericMatcher<Catch::Matchers::StartsWithMatcher>;
template struct SFS::test::SFSExceptionGenericMatcher<Catch::Matchers::RegexMatcher>;
