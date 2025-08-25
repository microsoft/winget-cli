// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.ApplicationModel.Core.h>
#include <memory>
#include <shared_mutex>
#include <set>
#include <string>
#include <string_view>

namespace AppInstaller::WinRT
{
    // Contains registration for static storage so that they can be cleared.
    struct COMStaticStorageStatics
    {
        // Adds a static storage key to the set of known items.
        static void AddStaticStorageItem(const winrt::hstring& name, const winrt::Windows::Foundation::IInspectable& item);

        // Removes all known static storage items.
        static void ResetAll();

    private:
        COMStaticStorageStatics() = default;

        static COMStaticStorageStatics& Instance();

        winrt::slim_mutex m_lock;
        std::set<std::wstring> m_items;
    };

    // https://devblogs.microsoft.com/oldnewthing/20210215-00/?p=104865
    // Base class for an object that needs to live in the COM static store.
    // An object needs to use this if it has:
    //  - static lifetime
    //  - references to externally implemented COM objects
    //
    // Additionally, it should *not* contain references to WRL counted objects implemented by this module.
    // If it does, it will prevent the module from being unloaded until COM is uninitialized, which is often never.
    template <typename DataType>
    struct COMStaticStorageBase
    {
    private:
        struct DataHolder : public winrt::implements<DataHolder, winrt::Windows::Foundation::IInspectable>
        {
            std::shared_ptr<DataType> m_shared{ std::make_shared<DataType>() };
        };

        std::weak_ptr<DataType> m_weak;
        winrt::slim_mutex m_lock;
        winrt::hstring m_name;

    public:
        COMStaticStorageBase(std::wstring_view name) : m_name(name) {}

        std::shared_ptr<DataType> Get()
        {
            {
                const std::shared_lock lock{ m_lock };
                if (auto cached = m_weak.lock())
                {
                    return cached;
                }
            }

            auto value = winrt::make_self<DataHolder>();

            const winrt::slim_lock_guard lock{ m_lock };
            if (auto cached = m_weak.lock())
            {
                return cached;
            }

            COMStaticStorageStatics::AddStaticStorageItem(m_name, value.as<winrt::Windows::Foundation::IInspectable>());
            m_weak = value->m_shared;
            return value->m_shared;
        }

        void Reset()
        {
            winrt::Windows::ApplicationModel::Core::CoreApplication::Properties().TryRemove(m_name);
        }
    };
}
