// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "DscCommandBase.h"

namespace AppInstaller::CLI
{
    // A test resource implementing file content configuration.
    struct DscTestFileResource : public DscCommandBase
    {
        DscTestFileResource(std::string_view parent);

        Resource::LocString ShortDescription() const override;
        Resource::LocString LongDescription() const override;

    protected:
        // TODO: Resource function calls
    };
}
