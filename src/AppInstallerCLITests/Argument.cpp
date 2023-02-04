// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <Argument.h>

using namespace AppInstaller::CLI;

TEST_CASE("EnsureAllArgumentsDefined", "[argument]")
{
    using Arg_t = std::underlying_type_t<Execution::Args::Type>;
    for (Arg_t i = static_cast<Arg_t>(0); i < static_cast<Arg_t>(Execution::Args::Type::Max); ++i)
    {
        REQUIRE_NOTHROW(ArgumentCommon::ForType(static_cast<Execution::Args::Type>(i)));
    }
}