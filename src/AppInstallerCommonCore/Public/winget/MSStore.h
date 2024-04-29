// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Manifest.h"
#include <AppInstallerProgress.h>

#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.ApplicationModel.Store.Preview.InstallControl.h>

#include <string>

namespace AppInstaller::MSStore
{
    using namespace std::string_view_literals;
    static constexpr std::wstring_view s_AppInstallerProductId = L"9NBLGGH4NNS1"sv;

    enum class MSStoreOperationType
    {
        Install,
        Update,
        Repair,
    };

    struct MSStoreOperation
    {
        MSStoreOperation(MSStoreOperationType type, const std::wstring& productId, Manifest::ScopeEnum scope, bool isSilentMode, bool force) :
            m_type(type), m_productId(productId), m_scope(scope), m_isSilentMode(isSilentMode), m_force(force)
        {
        }

        MSStoreOperation(const MSStoreOperation&) = delete;
        MSStoreOperation& operator=(const MSStoreOperation&) = delete;

        MSStoreOperation(MSStoreOperation&&) = delete;
        MSStoreOperation& operator=(MSStoreOperation&&) = delete;

        HRESULT StartAndWaitForOperation(IProgressCallback& progress);

    private:
        HRESULT InstallPackage(IProgressCallback& progress);
        HRESULT UpdatePackage(IProgressCallback& progress);
        HRESULT WaitForOperation(winrt::Windows::Foundation::Collections::IVectorView<winrt::Windows::ApplicationModel::Store::Preview::InstallControl::AppInstallItem>& installItems, IProgressCallback& progress);

        MSStoreOperationType m_type;
        std::wstring m_productId;
        Manifest::ScopeEnum m_scope;
        bool m_isSilentMode;
        bool m_force;
    };
}
