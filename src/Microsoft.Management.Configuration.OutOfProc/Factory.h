// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <hstring.h>
#include <inspectable.h>
#include <winrt/Windows.Foundation.h>
#include <atomic>

namespace Microsoft::Management::Configuration::OutOfProc
{
    struct Factory : winrt::implements<Factory, winrt::Windows::Foundation::IActivationFactory, IClassFactory>
    {
        Factory();
        ~Factory();

        // Returns true if the reference count is not 0; false if it is.
        static bool HasReferences();

        // Forcibly destroys any static objects.
        static void Terminate();

        // Determines if the given CLSID is the CLSID for the factory.
        static bool IsCLSID(const GUID& clsid);

        // Determines if the given CLSID is the CLSID for the factory.
        static bool IsCLSID(HSTRING clsid);

        // IActivationFactory
        winrt::Windows::Foundation::IInspectable ActivateInstance();

        // IClassFactory
        HRESULT STDMETHODCALLTYPE CreateInstance(::IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
        HRESULT STDMETHODCALLTYPE LockServer(BOOL fLock);

    private:
        static void IncrementRefCount();
        static void DecrementRefCount();

        static std::atomic<int32_t> s_referenceCount;
    };
}
