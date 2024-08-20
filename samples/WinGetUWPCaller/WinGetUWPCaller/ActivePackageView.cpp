// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "ActivePackageView.h"
#include "ActivePackageView.g.cpp"
#include <winrt/Windows.UI.Core.h>

namespace winrt::WinGetUWPCaller::implementation
{
    Microsoft::Management::Deployment::CatalogPackage ActivePackageView::Package()
    {
        return m_package;
    }

    void ActivePackageView::Package(Microsoft::Management::Deployment::CatalogPackage const& value)
    {
        m_package = value;
    }

    ActivePackageView::AsyncOperation_t ActivePackageView::AsyncOperation()
    {
        return m_asyncOperation;
    }

    Windows::Foundation::IAsyncAction UpdateUIProgress(
        Microsoft::Management::Deployment::InstallProgress progress,
        WinGetUWPCaller::ActivePackageView view)
    {
        co_await resume_foreground(view.Dispatcher());
        view.Progress(progress.DownloadProgress * 100);
    }

    void ActivePackageView::AsyncOperation(ActivePackageView::AsyncOperation_t const& value)
    {
        m_asyncOperation = value;
        m_asyncOperation.Progress([=](
            ActivePackageView::AsyncOperation_t const& /* sender */,
            Microsoft::Management::Deployment::InstallProgress const& progress)
            {
                UpdateUIProgress(progress, *this);
            });
    }

    double ActivePackageView::Progress()
    {
        return m_progress;
    }

    void ActivePackageView::Progress(double value)
    {
        if (m_progress != value)
        {
            m_progress = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"Progress" });
        }
    }

    hstring ActivePackageView::StatusText()
    {
        return m_text;
    }

    void ActivePackageView::StatusText(hstring const& value)
    {
        m_text = value;
    }

    Windows::UI::Core::CoreDispatcher ActivePackageView::Dispatcher()
    {
        return m_dispatcher;
    }

    void ActivePackageView::Dispatcher(Windows::UI::Core::CoreDispatcher const& value)
    {
        m_dispatcher = value;
    }

    event_token ActivePackageView::PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void ActivePackageView::PropertyChanged(event_token const& token)
    {
        m_propertyChanged.remove(token);
    }
}
