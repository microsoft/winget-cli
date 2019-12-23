// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/1_0/Interface.h"


namespace AppInstaller::Repository::Microsoft::Schema::V1_0
{
    Schema::Version Interface::GetVersion() const
    {
        return { 1, 0 };
    }
}
