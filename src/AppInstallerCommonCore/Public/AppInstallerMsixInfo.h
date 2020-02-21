// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once

namespace AppInstaller::Msix
{
    // Function to create an AppxBundle package reader given the input file name.
    // Returns true if success, false if the input stream is of wrong type.
    bool GetBundleReader(
        IStream* inputStream,
        IAppxBundleReader** reader);

    // Function to create an Appx package reader given the input file name.
    // Returns true if success, false if the input stream is of wrong type.
    bool GetPackageReader(
        IStream* inputStream,
        IAppxPackageReader** reader);

    // MsixInfo class handles all appx/msix related query.
    struct MsixInfo
    {
        MsixInfo(const std::string& uriStr);

        MsixInfo(const MsixInfo&) = default;
        MsixInfo& operator=(const MsixInfo&) = default;

        MsixInfo(MsixInfo&&) = default;
        MsixInfo& operator=(MsixInfo&&) = default;

        inline bool GetIsBundle()
        {
            return m_isBundle;
        }

        // Full content of AppxSignature.p7x
        std::vector<byte> GetSignature();

    private:
        bool m_isBundle;
        Microsoft::WRL::ComPtr<IStream> m_stream;
        Microsoft::WRL::ComPtr<IAppxBundleReader> m_bundleReader;
        Microsoft::WRL::ComPtr<IAppxPackageReader> m_packageReader;
    };
}