// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include "pch.h"
#include "HttpStream/HttpRandomAccessStream.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerMsixInfo.h"


using namespace winrt::Windows::Storage::Streams;
using namespace Microsoft::WRL;
using namespace AppInstaller::Utility::HttpStream;

namespace AppInstaller::Utility::Msix
{
    HRESULT GetBundleReader(
        _In_ IStream* inputStream,
        _Outptr_ IAppxBundleReader** reader)
    {
        ComPtr<IAppxBundleFactory> bundleFactory;

        // Create a new Appxbundle factory
        RETURN_IF_FAILED(CoCreateInstance(
            __uuidof(AppxBundleFactory),
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IAppxBundleFactory),
            (LPVOID*)(&bundleFactory)));

        RETURN_IF_FAILED(bundleFactory->CreateBundleReader(inputStream, reader));

        return S_OK;
    }

    HRESULT GetPackageReader(
        _In_ IStream* inputStream,
        _Outptr_ IAppxPackageReader** reader)
    {

        ComPtr<IAppxFactory> appxFactory;

        // Create a new Appx factory
        RETURN_IF_FAILED(CoCreateInstance(
            __uuidof(AppxFactory),
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IAppxFactory),
            (LPVOID*)(&appxFactory)));

        // Create a new package reader using the factory.
        RETURN_IF_FAILED(appxFactory->CreatePackageReader(inputStream, reader));

        return S_OK;
    }

    std::unique_ptr<MsixInfo> MsixInfo::CreateMsixInfo(const std::string& uriStr)
    {
        auto msixInfo = std::unique_ptr<MsixInfo>(new MsixInfo());
        msixInfo->PopulateMsixInfo(uriStr);
        return msixInfo;
    }

    void MsixInfo::PopulateMsixInfo(const std::string& uriStr)
    {
        winrt::Windows::Foundation::Uri uri(Utility::ConvertToUTF16(uriStr));
        IRandomAccessStream randomAccessStream = HttpRandomAccessStream::CreateAsync(uri).get();

        ::IUnknown* rasAsIUnknown = (::IUnknown*)winrt::get_abi(randomAccessStream);
        THROW_IF_FAILED(CreateStreamOverRandomAccessStream(
            rasAsIUnknown,
            IID_PPV_ARGS(m_stream.ReleaseAndGetAddressOf())));

        HRESULT hr = GetBundleReader(m_stream.Get(), &m_bundleReader);
        if (SUCCEEDED(hr))
        {
            m_isBundle = true;
        }
        else if (hr == APPX_E_MISSING_REQUIRED_FILE) // returned when trying to open an *.msix as an *.msixbundle or vice-versa
        {
            hr = GetPackageReader(m_stream.Get(), &m_packageReader);
            if (SUCCEEDED(hr))
            {
                m_isBundle = false;
            }
        }

        THROW_IF_FAILED(hr);
    }

    std::vector<byte> MsixInfo::GetSignature()
    {
        ComPtr<IAppxFile> signatureFile;
        if (m_isBundle)
        {
            THROW_IF_FAILED(m_bundleReader->GetFootprintFile(APPX_BUNDLE_FOOTPRINT_FILE_TYPE_SIGNATURE, &signatureFile));
        }
        else
        {
            THROW_IF_FAILED(m_packageReader->GetFootprintFile(APPX_FOOTPRINT_FILE_TYPE_SIGNATURE, &signatureFile));
        }

        std::vector<byte> signatureContent;
        DWORD signatureSize;

        ComPtr<IStream> signatureStream;
        THROW_IF_FAILED(signatureFile->GetStream(&signatureStream));

        STATSTG stat = { 0 };
        THROW_IF_FAILED(signatureStream->Stat(&stat, STATFLAG_NONAME));
        signatureSize = stat.cbSize.LowPart;

        signatureContent.resize(signatureSize);

        DWORD signatureRead;
        THROW_IF_FAILED(signatureStream->Read(signatureContent.data(), signatureSize, &signatureRead));
        THROW_HR_IF_MSG(E_UNEXPECTED, signatureRead != signatureSize, "Failed to read the whole signature stream");

        return signatureContent;
    }
}