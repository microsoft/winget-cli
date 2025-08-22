// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/COMStaticStorage.h"

namespace AppInstaller::WinRT
{
    COMStaticStorageStatics& COMStaticStorageStatics::Instance()
    {
        static COMStaticStorageStatics s_instance;
        return s_instance;
    }

    void COMStaticStorageStatics::AddStaticStorageItem(const winrt::hstring& name, const winrt::Windows::Foundation::IInspectable& item)
    {
        COMStaticStorageStatics& instance = Instance();
        const winrt::slim_lock_guard lock{ instance.m_lock };
        winrt::Windows::ApplicationModel::Core::CoreApplication::Properties().Insert(name, item);
        instance.m_items.emplace(std::wstring{ name });
    }

    void COMStaticStorageStatics::ResetAll() try
    {
        COMStaticStorageStatics& instance = Instance();
        std::set<std::wstring> localItems;

        {
            const winrt::slim_lock_guard lock{ instance.m_lock };
            instance.m_items.swap(localItems);
        }

        for (const auto& item : localItems)
        {
            winrt::Windows::ApplicationModel::Core::CoreApplication::Properties().TryRemove(item);
        }
    }
    CATCH_LOG();
}
