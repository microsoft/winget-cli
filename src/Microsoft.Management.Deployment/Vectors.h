#pragma once
#include "Vectors.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    struct Vectors : VectorsT<Vectors>
    {
        Vectors() = default;

        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::CatalogPackage> GetCatalogPackageVector();
        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::CatalogPackage> GetCatalogPackageVectorView();
        Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageVersionInfo> GetPackageVersionInfoVector();
        Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionInfo> GetPackageVersionInfoVectorView();
    };
}
namespace winrt::Microsoft::Management::Deployment::factory_implementation
{
    struct Vectors : VectorsT<Vectors, implementation::Vectors>
    {
    };
}