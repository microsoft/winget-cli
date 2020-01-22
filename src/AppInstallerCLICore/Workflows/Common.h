// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once

namespace AppInstaller::Workflow
{
    class WorkflowException : public wil::ResultException
    {
    public:
        WorkflowException(HRESULT hr) : wil::ResultException(hr) {}
    };
}