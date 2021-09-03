#pragma once
#include "InstallingPackageView.g.h"

namespace winrt::AppInstallerCaller::implementation
{
    struct InstallingPackageView : InstallingPackageViewT<InstallingPackageView>
    {
        InstallingPackageView() = default;

        winrt::Microsoft::Management::Deployment::CatalogPackage Package();
        void Package(winrt::Microsoft::Management::Deployment::CatalogPackage const& value);
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> AsyncOperation();
        void AsyncOperation(winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> const& value);
        double Progress();
        void Progress(double value);
        hstring StatusText();
        void StatusText(hstring const& value);
        winrt::Windows::UI::Core::CoreDispatcher Dispatcher();
        void Dispatcher(winrt::Windows::UI::Core::CoreDispatcher const& value);
        winrt::event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& value);
        void PropertyChanged(winrt::event_token const& token);
    private:
        winrt::Microsoft::Management::Deployment::CatalogPackage m_package{ nullptr };
        winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress> m_asyncOperation{ nullptr };
        hstring m_text;
        double m_progress = 0;
        winrt::Windows::UI::Core::CoreDispatcher m_dispatcher{ nullptr };
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}
namespace winrt::AppInstallerCaller::factory_implementation
{
    struct InstallingPackageView : InstallingPackageViewT<InstallingPackageView, implementation::InstallingPackageView>
    {
    };
}
