// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "ConnectionConfig.h"

#include "RequestParams.h"

using namespace SFS;
using namespace SFS::details;

ConnectionConfig::ConnectionConfig(const SFS::RequestParams& requestParams)
    : maxRetries(requestParams.retryOnError ? c_maxRetries : 0)
    , baseCV(requestParams.baseCV)
{
}
