#pragma once
#include "InstallResult.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct InstallResult : InstallResultT<InstallResult>
    {
        InstallResult() = default;

        hstring CorrelationId();
        bool RebootRequired();
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct InstallResult : InstallResultT<InstallResult, implementation::InstallResult>
    {
    };
}
