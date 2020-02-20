// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <SourceFactory.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>

#ifdef AICLI_DISABLE_TEST_HOOKS
static_assert(false, "Test hooks have been disabled");
#endif

namespace AppInstaller
{
    namespace Runtime
    {
        void TestHook_ForceContainerPrepend(const std::filesystem::path& prepend);
    }

    namespace Repository
    {
        void TestHook_SetSourceFactoryOverride(const std::string& type, std::function<std::unique_ptr<ISourceFactory>()>&& factory);
        void TestHook_ClearSourceFactoryOverrides();
    }
}
