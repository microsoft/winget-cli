// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#pragma once
#include "pch.h"

namespace AppInstaller::Utility::Msix
{
    // Function to create an AppxBundle package reader given the input file name.
    HRESULT GetBundleReader(
        IStream* inputStream,
        IAppxBundleReader** reader);

    // Function to create an Appx package reader given the input file name.
    HRESULT GetPackageReader(
        IStream* inputStream,
        IAppxPackageReader** reader);

    class MsixInfo
    {
    public:
        static std::unique_ptr<MsixInfo> CreateMsixInfo(const std::string& uriStr);

        inline bool GetIsBundle()
        {
            return m_isBundle;
        }

        std::vector<byte> GetSignature();

    private:
        MsixInfo() {};

        void PopulateMsixInfo(const std::string& uriStr);

        bool m_isBundle;
        Microsoft::WRL::ComPtr<IStream> m_stream;
        Microsoft::WRL::ComPtr<IAppxBundleReader> m_bundleReader;
        Microsoft::WRL::ComPtr<IAppxPackageReader> m_packageReader;
    };
}