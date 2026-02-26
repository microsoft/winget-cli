// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "FindUnitProcessorsOptions.g.h"
#include <winget/ILifetimeWatcher.h>
#include <winrt/Windows.Foundation.h>

namespace winrt::Microsoft::Management::Configuration::implementation
{
    struct FindUnitProcessorsOptions : FindUnitProcessorsOptionsT<FindUnitProcessorsOptions, winrt::cloaked<AppInstaller::WinRT::ILifetimeWatcher>>, AppInstaller::WinRT::LifetimeWatcherBase
    {
        FindUnitProcessorsOptions() = default;

        hstring SearchPaths() const;
        void SearchPaths(hstring const& value);

        bool SearchPathsExclusive() const;
        void SearchPathsExclusive(bool value);

        Microsoft::Management::Configuration::ConfigurationUnitDetailFlags UnitDetailFlags() const;
        void UnitDetailFlags(Microsoft::Management::Configuration::ConfigurationUnitDetailFlags value);

        HRESULT STDMETHODCALLTYPE SetLifetimeWatcher(IUnknown* watcher);

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
    private:
        hstring m_searchPaths;
        bool m_searchPathsExclusive = false;
        Microsoft::Management::Configuration::ConfigurationUnitDetailFlags m_detailFlags = Microsoft::Management::Configuration::ConfigurationUnitDetailFlags::None;
#endif
    };
}

#if !defined(INCLUDE_ONLY_INTERFACE_METHODS)
namespace winrt::Microsoft::Management::Configuration::factory_implementation
{
    struct FindUnitProcessorsOptions : FindUnitProcessorsOptionsT<FindUnitProcessorsOptions, implementation::FindUnitProcessorsOptions>
    {
    };
}
#endif
