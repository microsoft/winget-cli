#include "pch.h"
#include "FindPackagesResult.h"
#include "FindPackagesResult.g.cpp"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::ResultMatch> FindPackagesResult::Matches()
    {
        throw hresult_not_implemented();
    }
    bool FindPackagesResult::IsTruncated()
    {
        throw hresult_not_implemented();
    }
}
