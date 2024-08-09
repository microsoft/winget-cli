// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Documentation.g.cpp"
#include "Documentation.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    void Documentation::Initialize(::AppInstaller::Manifest::Documentation documentation)
    {
        m_documentation = std::move(documentation);
    }
    hstring Documentation::DocumentLabel()
    {
        return winrt::to_hstring(m_documentation.DocumentLabel);
    }
    hstring Documentation::DocumentUrl()
    {
        return winrt::to_hstring(m_documentation.DocumentUrl);
    }
}
