// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "TestOverride.h"

using namespace SFS;
using SFS::test::ScopedTestOverride;
using SFS::test::TestOverride;

bool test::AreTestOverridesAllowed()
{
#ifdef SFS_ENABLE_TEST_OVERRIDES
    return true;
#else
    return false;
#endif
}

std::string test::GetEnvVarNameFromOverride(TestOverride override)
{
    switch (override)
    {
    case TestOverride::BaseRetryDelayMs:
        return "SFS_TEST_BASE_RETRY_DELAY_MS";
    case TestOverride::BaseUrl:
        return "SFS_TEST_OVERRIDE_BASE_URL";
    }
    return "";
}

std::optional<std::string> test::GetTestOverride(TestOverride override)
{
    if (!AreTestOverridesAllowed())
    {
        return std::nullopt;
    }

    return details::env::GetEnv(GetEnvVarNameFromOverride(override));
}

std::optional<int> test::GetTestOverrideAsInt(TestOverride override)
{
    auto str = GetTestOverride(override);
    if (str)
    {
        return std::stoi(*str);
    }
    return std::nullopt;
}

ScopedTestOverride::ScopedTestOverride(TestOverride override, const std::string& value)
    : m_scopedEnv(GetEnvVarNameFromOverride(override), value)
{
}

ScopedTestOverride::ScopedTestOverride(TestOverride override, int value)
    : m_scopedEnv(GetEnvVarNameFromOverride(override), std::to_string(value))
{
}
