// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ActivePackageView.g.h"

namespace winrt::WinGetUWPCaller::implementation
{
    struct ActivePackageView : ActivePackageViewT<ActivePackageView>
    {
        using AsyncOperation_t = Windows::Foundation::IAsyncOperationWithProgress<Microsoft::Management::Deployment::InstallResult, Microsoft::Management::Deployment::InstallProgress>;

        ActivePackageView() = default;

        Microsoft::Management::Deployment::CatalogPackage Package();
        void Package(Microsoft::Management::Deployment::CatalogPackage const& value);
        AsyncOperation_t AsyncOperation();
        void AsyncOperation(AsyncOperation_t const& value);
        double Progress();
        void Progress(double value);
        hstring StatusText();
        void StatusText(hstring const& value);
        Windows::UI::Core::CoreDispatcher Dispatcher();
        void Dispatcher(Windows::UI::Core::CoreDispatcher const& value);
        event_token PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& value);
        void PropertyChanged(event_token const& token);

    private:
        Microsoft::Management::Deployment::CatalogPackage m_package{ nullptr };
        AsyncOperation_t m_asyncOperation{ nullptr };
        double m_progress = 0;
        hstring m_text;
        Windows::UI::Core::CoreDispatcher m_dispatcher{ nullptr };
        event<Windows::UI::Xaml::Data::PropertyChangedEventHandler> m_propertyChanged;
    };
}

namespace winrt::WinGetUWPCaller::factory_implementation
{
    struct ActivePackageView : ActivePackageViewT<ActivePackageView, implementation::ActivePackageView>
    {
    };
}
