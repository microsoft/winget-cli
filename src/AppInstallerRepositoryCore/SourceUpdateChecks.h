// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Public/winget/RepositorySource.h"

namespace AppInstaller::Repository
{
    // Determines if the current time is before a previously stored "do note update before" time.
    bool IsBeforeDoNotUpdateBeforeTime(const SourceDetails& details);

    // Determines if the given details and desired update interval indicate an update check should occur.
    bool IsAfterUpdateCheckTime(const SourceDetails& details, std::optional<TimeSpan> requestedUpdateInterval);
}
