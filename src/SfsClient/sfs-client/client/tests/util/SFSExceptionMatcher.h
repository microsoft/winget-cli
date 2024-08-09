// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Result.h"
#include "SFSException.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_templated.hpp>

#include <optional>
#include <string>

#define REQUIRE_THROWS_CODE(call, code)                                                                                \
    REQUIRE_THROWS_MATCHES(call, SFS::details::SFSException, SFS::test::SFSExceptionMatcher(SFS::Result::code))
#define REQUIRE_THROWS_CODE_MSG(call, code, message)                                                                   \
    REQUIRE_THROWS_MATCHES(call, SFS::details::SFSException, SFS::test::SFSExceptionMatcher(SFS::Result::code, message))
#define REQUIRE_THROWS_CODE_MSG_MATCHES(call, code, matcher)                                                           \
    REQUIRE_THROWS_MATCHES(call,                                                                                       \
                           SFS::details::SFSException,                                                                 \
                           SFS::test::SFSExceptionGenericMatcher(SFS::Result::code, matcher))

namespace SFS::test
{
struct SFSExceptionMatcher : Catch::Matchers::MatcherGenericBase
{
    SFSExceptionMatcher(const Result::Code& code);
    SFSExceptionMatcher(const Result::Code& code, std::string message);

    bool match(const SFS::details::SFSException& other) const;
    std::string describe() const override;

  private:
    Result::Code m_code{Result::Success};
    std::optional<std::string> m_message;
};

template <typename Matcher>
struct SFSExceptionGenericMatcher : Catch::Matchers::MatcherGenericBase
{
    SFSExceptionGenericMatcher(const Result::Code& code, Matcher messageMatcher);

    bool match(const SFS::details::SFSException& other) const;
    std::string describe() const override;

  private:
    Result::Code m_code{Result::Success};
    Matcher m_messageMatcher;
};
} // namespace SFS::test
