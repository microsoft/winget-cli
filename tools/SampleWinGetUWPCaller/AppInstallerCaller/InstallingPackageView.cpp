// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "InstallingPackageView.h"
#include "InstallingPackageView.g.cpp"
#include <winrt/Windows.UI.Core.h>

namespace winrt::AppInstallerCaller::implementation
{
    winrt::Microsoft::Management::Deployment::CatalogPackage InstallingPackageView::Package()
    {
        return m_package;
    }

    void InstallingPackageView::Package(winrt::Microsoft::Management::Deployment::CatalogPackage const& value)
    {
        m_package = value;;
    }

    InstallingPackageView::AsyncOperation_t InstallingPackageView::AsyncOperation()
    {
        return m_asyncOperation;
    }

    winrt::Windows::Foundation::IAsyncAction UpdateUIProgress(
        winrt::Microsoft::Management::Deployment::InstallProgress progress,
        winrt::AppInstallerCaller::InstallingPackageView view)
    {
        co_await winrt::resume_foreground(view.Dispatcher());
        view.Progress(progress.DownloadProgress * 100);
    }

    void InstallingPackageView::AsyncOperation(InstallingPackageView::AsyncOperation_t const& value)
    {
        m_asyncOperation = value;
        m_asyncOperation.Progress([=](
            InstallingPackageView::AsyncOperation_t const& /* sender */,
            winrt::Microsoft::Management::Deployment::InstallProgress const& progress)
            {
                UpdateUIProgress(progress, *this);
            });
    }

    double InstallingPackageView::Progress()
    {
        return m_progress;
    }

    void InstallingPackageView::Progress(double value)
    {
        if (m_progress != value)
        {
            m_progress = value;
            m_propertyChanged(*this, Windows::UI::Xaml::Data::PropertyChangedEventArgs{ L"Progress" });
        }
    }

    hstring InstallingPackageView::StatusText()
    {
        return m_text;
    }

    void InstallingPackageView::StatusText(hstring const& value)
    {
        m_text = value;
    }

    winrt::Windows::UI::Core::CoreDispatcher InstallingPackageView::Dispatcher()
    {
        return m_dispatcher;
    }

    void InstallingPackageView::Dispatcher(winrt::Windows::UI::Core::CoreDispatcher const& value)
    {
        m_dispatcher = value;
    }

    winrt::event_token InstallingPackageView::PropertyChanged(Windows::UI::Xaml::Data::PropertyChangedEventHandler const& handler)
    {
        return m_propertyChanged.add(handler);
    }

    void InstallingPackageView::PropertyChanged(winrt::event_token const& token)
    {
        m_propertyChanged.remove(token);
    }
}
