// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "Env.h"
#include "TestOverride.h"

#include <catch2/catch_test_macros.hpp>

#define TEST(...) TEST_CASE("[TestOverrideTests] " __VA_ARGS__)

using namespace SFS::test;
using namespace SFS::details::env;

TEST("Testing AreTestOverridesAllowed()")
{
    bool areTestOverridesAllowed = AreTestOverridesAllowed();
#ifdef SFS_ENABLE_TEST_OVERRIDES
    REQUIRE(areTestOverridesAllowed);
#else
    REQUIRE_FALSE(areTestOverridesAllowed);
#endif
}

TEST("Testing GetEnvVarNameFromOverride")
{
    REQUIRE(GetEnvVarNameFromOverride(TestOverride::BaseUrl) == "SFS_TEST_OVERRIDE_BASE_URL");
}

TEST("Testing GetTestOverride()")
{
    if (AreTestOverridesAllowed())
    {
        SECTION("Testing GetTestOverride() on a non-existing environment variable")
        {
            auto env = GetTestOverride(TestOverride::BaseUrl);
            REQUIRE(!env.has_value());

            SECTION("Testing GetTestOverride() on an existing environment variable")
            {
                const std::string varName = GetEnvVarNameFromOverride(TestOverride::BaseUrl);
                REQUIRE(SetEnv(varName, "override"));

                env = GetTestOverride(TestOverride::BaseUrl);
                REQUIRE(env.has_value());
                REQUIRE(*env == "override");

                INFO("Unsetting the environment variable");
                REQUIRE(UnsetEnv(varName));
            }
        }
    }
    else
    {
        SECTION("GetTestOverride() returns std::nullopt when test overrides are not allowed")
        {
            auto env = GetTestOverride(TestOverride::BaseUrl);
            REQUIRE(!env.has_value());
        }
    }
}

TEST("Testing ScopedTestOverride")
{
    SECTION("Testing ScopedTestOverride on a non-existing override")
    {
        const std::string varName = GetEnvVarNameFromOverride(TestOverride::BaseUrl);
        auto env = GetTestOverride(TestOverride::BaseUrl);
        REQUIRE(!env.has_value());

        {
            ScopedTestOverride scopedOverride(TestOverride::BaseUrl, "dummyValue");

            INFO("Variable exists within scope");
            env = GetEnv(varName);
            REQUIRE(env.has_value());
            REQUIRE(*env == "dummyValue");

            if (AreTestOverridesAllowed())
            {
                INFO("Checking test override within scope");
                env = GetTestOverride(TestOverride::BaseUrl);
                REQUIRE(env.has_value());
                REQUIRE(*env == "dummyValue");
            }

            SECTION("Testing ScopedEnv on an existing variable overwrites it")
            {
                {
                    ScopedTestOverride scopedOverride2(TestOverride::BaseUrl, "dummyValue2");

                    INFO("Variable has value overwritten within scope");
                    env = GetEnv(varName);
                    REQUIRE(env.has_value());
                    REQUIRE(*env == "dummyValue2");

                    if (AreTestOverridesAllowed())
                    {
                        INFO("Checking test override has been overwritten within scope");
                        env = GetTestOverride(TestOverride::BaseUrl);
                        REQUIRE(env.has_value());
                        REQUIRE(*env == "dummyValue2");
                    }
                }

                INFO("Variable goes back to previous value");
                env = GetEnv(varName);
                REQUIRE(env.has_value());
                REQUIRE(*env == "dummyValue");

                if (AreTestOverridesAllowed())
                {
                    INFO("Checking test override has gone back to previous value");
                    env = GetTestOverride(TestOverride::BaseUrl);
                    REQUIRE(env.has_value());
                    REQUIRE(*env == "dummyValue");
                }
            }
        }

        INFO("Variable should be unset after the scope ends");
        env = GetEnv(varName);
        REQUIRE(!env.has_value());

        if (AreTestOverridesAllowed())
        {
            INFO("Test override should be unset after the scope ends");
            env = GetTestOverride(TestOverride::BaseUrl);
            REQUIRE(!env.has_value());
        }
    }
}
