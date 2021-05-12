#pragma once
#include "ResultMatch.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("ca471e3f-e3ae-4658-8f7e-682f4bcaa97e")]
    struct ResultMatch : ResultMatchT<ResultMatch>
    {
        ResultMatch() = default;
        ResultMatch(Microsoft::Management::Deployment::CatalogPackage package, Microsoft::Management::Deployment::PackageMatchFilter matchCriteria);
        void Initialize(Microsoft::Management::Deployment::CatalogPackage package, Microsoft::Management::Deployment::PackageMatchFilter matchCriteria);

        Microsoft::Management::Deployment::CatalogPackage CatalogPackage();
        Microsoft::Management::Deployment::PackageMatchFilter MatchCriteria();
    private:
        Microsoft::Management::Deployment::CatalogPackage m_catalogPackage{ nullptr };
        Microsoft::Management::Deployment::PackageMatchFilter m_matchCriteria;
    };
}
