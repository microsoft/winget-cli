#pragma once
#include "FindPackagesResult.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct FindPackagesResult : FindPackagesResultT<FindPackagesResult>
    {
        FindPackagesResult() = default;

        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::ResultMatch> Matches();
        bool IsTruncated();
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct FindPackagesResult : FindPackagesResultT<FindPackagesResult, implementation::FindPackagesResult>
    {
    };
}
