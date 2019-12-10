// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

#include <windows.h>
#include "TraceLogging.h"

struct TraceLoggingRegistration
{
    TraceLoggingRegistration()
    {
        RegisterTraceLogging();
    }

    ~TraceLoggingRegistration()
    {
        UnRegisterTraceLogging();
    }
};

