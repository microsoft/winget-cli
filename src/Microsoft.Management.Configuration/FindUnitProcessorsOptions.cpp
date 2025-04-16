// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "FindUnitProcessorsOptions.h"
#include "FindUnitProcessorsOptions.g.cpp"

namespace winrt::Microsoft::Management::Configuration::implementation
{
    hstring FindUnitProcessorsOptions::SearchPaths() const
    {
        return m_searchPaths;
    }

    void FindUnitProcessorsOptions::SearchPaths(hstring const& value)
    {
        m_searchPaths = value;
    }

    bool FindUnitProcessorsOptions::SearchPathsExclusive() const
    {
        return m_searchPathsExclusive;
    }

    void FindUnitProcessorsOptions::SearchPathsExclusive(bool value)
    {
        m_searchPathsExclusive = value;
    }

    Microsoft::Management::Configuration::ConfigurationUnitDetailFlags FindUnitProcessorsOptions::UnitDetailFlags() const
    {
        return m_detailFlags;
    }

    void FindUnitProcessorsOptions::UnitDetailFlags(Microsoft::Management::Configuration::ConfigurationUnitDetailFlags value)
    {
        m_detailFlags = value;
    }

    HRESULT STDMETHODCALLTYPE FindUnitProcessorsOptions::SetLifetimeWatcher(IUnknown* watcher)
    {
        return AppInstaller::WinRT::LifetimeWatcherBase::SetLifetimeWatcher(watcher);
    }
}
