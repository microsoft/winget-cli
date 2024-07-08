// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Env.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[EnvTests] " __VA_ARGS__)

using namespace SFS::details::env;

TEST("Testing GetEnv()")
{
    SECTION("Testing GetEnv() on an existing environment variable")
    {
        // Get the value of an existing environment variable per platform
#ifdef _WIN32
        const std::string varName = "COMPUTERNAME";
#else
        const std::string varName = "LANG";
#endif
        auto env = GetEnv(varName);
        REQUIRE(env.has_value());
        REQUIRE(env.value().size() > 0);
    }

    SECTION("Testing GetEnv() on a non-existing variable")
    {
        auto env = GetEnv("DUMMYVARIABLESHOULDNOTEXIST");
        REQUIRE(!env.has_value());
    }
}

TEST("Testing SetEnv()")
{
    SECTION("Testing SetEnv() on a non-existing variable")
    {
        auto env = GetEnv("DUMMYVARIABLE");
        REQUIRE(!env.has_value());

        REQUIRE(SetEnv("DUMMYVARIABLE", "dummyValue"));
        env = GetEnv("DUMMYVARIABLE");
        REQUIRE(env.has_value());
        REQUIRE(*env == "dummyValue");

        SECTION("Testing SetEnv() on an existing variable overwrites it")
        {
            REQUIRE(SetEnv("DUMMYVARIABLE", "dummyValue2"));

            env = GetEnv("DUMMYVARIABLE");
            REQUIRE(env.has_value());
            REQUIRE(*env == "dummyValue2");
        }

        INFO("Unsetting the environment variable");
        REQUIRE(UnsetEnv("DUMMYVARIABLE"));
        env = GetEnv("DUMMYVARIABLE");
        REQUIRE(!env.has_value());
    }

    SECTION("Testing SetEnv() with empty strings fails")
    {
        REQUIRE_FALSE(SetEnv("DUMMYVARIABLE", ""));
        REQUIRE_FALSE(SetEnv("DUMMYVARIABLE", std::string()));
        REQUIRE_FALSE(SetEnv("", "dummy"));
        REQUIRE_FALSE(SetEnv(std::string(), "dummy"));
        REQUIRE_FALSE(SetEnv(std::string(), std::string()));
    }
}

TEST("Testing UnsetEnv()")
{
    SECTION("Testing UnsetEnv() on a non-existing variable still succeeds")
    {
        auto env = GetEnv("DUMMYVARIABLE");
        REQUIRE(!env.has_value());

        REQUIRE(UnsetEnv("DUMMYVARIABLE"));
    }
}

TEST("Testing ScopedEnv")
{
    SECTION("Testing ScopedEnv on a non-existing variable")
    {
        auto env = GetEnv("DUMMYVARIABLE");
        REQUIRE(!env.has_value());

        {
            ScopedEnv scopedEnv("DUMMYVARIABLE", "dummyValue");

            INFO("Variable exists within scope");
            env = GetEnv("DUMMYVARIABLE");
            REQUIRE(env.has_value());
            REQUIRE(*env == "dummyValue");

            SECTION("Testing ScopedEnv on an existing variable overwrites it")
            {
                {
                    ScopedEnv scopedEnv2("DUMMYVARIABLE", "dummyValue2");

                    INFO("Variable has value overwritten within scope");
                    env = GetEnv("DUMMYVARIABLE");
                    REQUIRE(env.has_value());
                    REQUIRE(*env == "dummyValue2");
                }

                INFO("Variable goes back to previous value");
                env = GetEnv("DUMMYVARIABLE");
                REQUIRE(env.has_value());
                REQUIRE(*env == "dummyValue");
            }
        }

        INFO("Variable should be unset after the scope ends");
        env = GetEnv("DUMMYVARIABLE");
        REQUIRE(!env.has_value());
    }
}
