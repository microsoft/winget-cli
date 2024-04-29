// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

namespace SFS::details
{
class ReportingHandler;

enum class ContentType
{
    Generic,
    App,
};

void ValidateContentType(ContentType currentType, ContentType expectedType, const ReportingHandler& handler);
} // namespace SFS::details
