// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ActivePackageView.g.h"

namespace winrt::WinGetUWPCaller::implementation
{
    struct ActivePackageView : ActivePackageViewT<ActivePackageView>
    {
        using AsyncOperation_t = winrt::Windows::Foundation::IAsyncOperationWithProgress<winrt::Microsoft::Management::Deployment::InstallResult, winrt::Microsoft::Management::Deployment::InstallProgress>;

        ActivePackageView() = default;

        winrt::Microsoft::Management::Deployment::CatalogPackage Package();
        void Package(winrt::Microsoft::Management::Deployment::CatalogPackage const& value);
        AsyncOperation_t AsyncOperation();
        void AsyncOperation(AsyncOperation_t const& value);
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
        AsyncOperation_t m_asyncOperation{ nullptr };
        double m_progress = 0;
        hstring m_text;
        winrt::Windows::UI::Core::CoreDispatcher m_dispatcher{ nullptr };
        winrt::event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}

namespace winrt::WinGetUWPCaller::factory_implementation
{
    struct ActivePackageView : ActivePackageViewT<ActivePackageView, implementation::ActivePackageView>
    {
    };
}
