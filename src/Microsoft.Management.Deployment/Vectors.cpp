#include "pch.h"
#include "Microsoft/PredefinedInstalledSourceFactory.h"
#include "Vectors.h"
#include "Vectors.g.cpp"
#include "CatalogPackage.h"

namespace winrt::Microsoft::Management::Deployment::implementation
{
    Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::CatalogPackage> Vectors::GetCatalogPackageVector()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::CatalogPackage> Vectors::GetCatalogPackageVectorView()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVector<Microsoft::Management::Deployment::PackageVersionInfo> Vectors::GetPackageVersionInfoVector()
    {
        throw hresult_not_implemented();
    }
    Windows::Foundation::Collections::IVectorView<Microsoft::Management::Deployment::PackageVersionInfo> Vectors::GetPackageVersionInfoVectorView()
    {
        throw hresult_not_implemented();
    }
}
