#pragma once
#include "Vectors.g.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    [uuid("b627e5eb-38d0-4879-a94e-06b0dc5aa624")]
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