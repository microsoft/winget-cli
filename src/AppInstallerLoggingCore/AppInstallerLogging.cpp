// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerLogging.h"

namespace AppInstaller::Logging
{
    DiagnosticLogger& DiagnosticLogger::GetInstance()
    {
        static DiagnosticLogger instance;
        return instance;
    }
}