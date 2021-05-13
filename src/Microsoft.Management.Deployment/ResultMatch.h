#pragma once
#include "ResultMatch.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct ResultMatch : ResultMatchT<ResultMatch>
    {
        ResultMatch() = default;
        void Initialize(Microsoft::Management::Deployment::CatalogPackage package, Microsoft::Management::Deployment::PackageMatchFilter matchCriteria);

        Microsoft::Management::Deployment::CatalogPackage CatalogPackage();
        Microsoft::Management::Deployment::PackageMatchFilter MatchCriteria();
    private:
        Microsoft::Management::Deployment::CatalogPackage m_catalogPackage{ nullptr };
        Microsoft::Management::Deployment::PackageMatchFilter m_matchCriteria;
    };
}
