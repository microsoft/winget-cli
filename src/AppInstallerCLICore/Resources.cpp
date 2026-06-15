// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Resources.h"

using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI::Resource
{
    Utility::LocIndView GetFixedString(FixedString fs)
    {
        switch (fs)
        {
        case FixedString::ProductName: return "Windows Package Manager"_liv;
        }

        THROW_HR(E_UNEXPECTED);
    }
}
