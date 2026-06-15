// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <Unknwn.h>
#include <winrt/Windows.Foundation.h>

namespace AppInstaller::WinRT
{
    MIDL_INTERFACE("59b5623f-d03e-41f8-b400-89ee04ea02d7")
    ILifetimeWatcher : public IUnknown
    {
    public:
        // Due to the way the winrt types are set up, this watcher will not have AddRef called.
        virtual /* [local] */ HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(
            IUnknown* watcher) = 0;
    };

    // Implements ILifetimeWatcher functionality.
    struct LifetimeWatcherBase
    {
        HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher)
        {
            m_lifetimeWatcher = winrt::Windows::Foundation::IUnknown(watcher, winrt::take_ownership_from_abi);
            return S_OK;
        }

        void PropagateLifetimeWatcher(const winrt::Windows::Foundation::IUnknown& child)
        {
            if (m_lifetimeWatcher && child)
            {
                // Require that any call to this function is for an object that implements watching to prevent
                // accidental assumptions about the child object and lifetime management.
                auto watcher = child.as<ILifetimeWatcher>();

                // Create a copy of the lifetime watcher (to add_ref), then detach and pass it to the child to own.
                watcher->SetLifetimeWatcher(static_cast<IUnknown*>(winrt::detach_abi(winrt::Windows::Foundation::IUnknown{ m_lifetimeWatcher })));
            }
        }

    private:
        winrt::Windows::Foundation::IUnknown m_lifetimeWatcher;
    };
}
