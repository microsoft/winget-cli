#include "pch.h"
#include "PackageVersionId.h"
#include "PackageVersionId.g.cpp"





namespace winrt::Microsoft::Management::Deployment::implementation
{
    hstring PackageVersionId::AppCatalogId()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionId::Version()
    {
        throw hresult_not_implemented();
    }
    hstring PackageVersionId::Channel()
    {
        throw hresult_not_implemented();
    }
}
