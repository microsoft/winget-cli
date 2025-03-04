// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "DscTestFileResource.h"

using namespace AppInstaller::Utility::literals;

namespace AppInstaller::CLI
{
    DscTestFileResource::DscTestFileResource(std::string_view parent) :
        DscCommandBase(parent, "test-file", DscResourceKind::Resource,
            DscFunctions::Get | DscFunctions::Set | DscFunctions::WhatIf | DscFunctions::Test | DscFunctions::Export | DscFunctions::Schema,
            DscFunctionModifiers::ImplementsPretest | DscFunctionModifiers::HandlesExist)
    {
    }

    Resource::LocString DscTestFileResource::ShortDescription() const
    {
        return "[TEST] File content resource"_lis;
    }

    Resource::LocString DscTestFileResource::LongDescription() const
    {
        return "This resource is only available for tests. It provides file content configuration."_lis;
    }
}
