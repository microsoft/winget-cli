// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ContentType.h"

#include "../ErrorHandling.h"
#include "../ReportingHandler.h"
#include "Result.h"

using namespace SFS;
using namespace SFS::details;

namespace
{
std::string ToString(ContentType type)
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
} // namespace

void SFS::details::ValidateContentType(ContentType currentType,
                                       ContentType expectedType,
                                       const ReportingHandler& handler)
{
    THROW_CODE_IF_LOG(Result::ServiceUnexpectedContentType,
                      currentType != expectedType,
                      handler,
                      "Unexpected content type [" + ::ToString(currentType) +
                          "] returned by the service does not match the expected [" + ::ToString(expectedType) + "]");
}
