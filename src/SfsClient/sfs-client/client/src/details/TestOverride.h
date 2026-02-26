// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

#include "Env.h"

#include <optional>
#include <string>

namespace SFS::test
{
/**
 * @brief Check if test overrides are allowed and logs if so.
 * @details Test overrides are allowed if the SFS_ENABLE_TEST_OVERRIDES macro is defined.
 */
bool AreTestOverridesAllowed();

enum class TestOverride
{
    BaseRetryDelayMs, // Integer. Allows one to override the base retry delay.
    BaseUrl,          // String. Allows one to override the base URL used for all requests
};

/**
 * @brief Get the environment variable name for a given test override
 */
std::string GetEnvVarNameFromOverride(TestOverride override);

/**
 * @brief Get the value of a test override
 * @details std::nullopt is returned if the environment variable is not set or in case of failure.
 * The returned string may be different in Win32 due to the encoding of the environment variables.
 */
std::optional<std::string> GetTestOverride(TestOverride override);

/**
 * @brief Get the value of a test override as int
 * @details std::nullopt is returned if the environment variable is not set or in case of failure.
 */
std::optional<int> GetTestOverrideAsInt(TestOverride override);

class ScopedTestOverride
{
  public:
    ScopedTestOverride(TestOverride override, const std::string& value);
    ScopedTestOverride(TestOverride override, int value);

  private:
    SFS::details::env::ScopedEnv m_scopedEnv;
};
} // namespace SFS::test
