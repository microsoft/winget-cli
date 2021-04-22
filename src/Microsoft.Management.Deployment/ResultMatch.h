#pragma once
#include "ResultMatch.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct ResultMatch : ResultMatchT<ResultMatch>
    {
        ResultMatch() = default;

        Microsoft::Management::Deployment::Package Package();
        Microsoft::Management::Deployment::PackageMatchFilter MatchCriteria();
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct ResultMatch : ResultMatchT<ResultMatch, implementation::ResultMatch>
    {
    };
}
