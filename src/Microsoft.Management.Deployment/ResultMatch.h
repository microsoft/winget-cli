#pragma once
#include "ResultMatch.g.h"

// Note: Remove this static_assert after copying these generated source files to your project.
// This assertion exists to avoid compiling these generated source files directly.


namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct ResultMatch : ResultMatchT<ResultMatch>
    {
        ResultMatch() = default;

        Microsoft::Management::Deployment::CatalogPackage CatalogPackage();
        Microsoft::Management::Deployment::PackageMatchFilter MatchCriteria();
    };
}
