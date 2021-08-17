// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "TestSource.h"
#include <AppInstallerRepositorySource.h>

using namespace TestCommon;
using namespace AppInstaller::Repository;

// Test rest source.
struct TestRestSource : public TestSource
{
    TestRestSource() = default;
    TestRestSource(const SourceDetails& details)
    {
        Details = details;
    }

    static std::shared_ptr<ISource> Create(const SourceDetails& details)
    {
        return std::shared_ptr<ISource>(new TestRestSource(details));
    }
};
