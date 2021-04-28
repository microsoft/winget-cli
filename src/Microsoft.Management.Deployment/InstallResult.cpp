#include "pch.h"
#include "InstallResult.h"
#include "InstallResult.g.cpp"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring InstallResult::CorrelationId()
    {
        throw hresult_not_implemented();
    }
    bool InstallResult::RebootRequired()
    {
        throw hresult_not_implemented();
    }
}
