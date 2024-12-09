// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentType.h"

using namespace SFS::details;

std::string SFS::details::ToString(ContentType type)
{
    switch (type)
    {
    case ContentType::Generic:
        return "Generic";
    case ContentType::App:
        return "App";
    default:
        return "Unknown";
    }
}
