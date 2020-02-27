// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerMsixInfo.h"
#include "HttpStream/HttpRandomAccessStream.h"
#include "Public/AppInstallerStrings.h"


using namespace winrt::Windows::Storage::Streams;
using namespace Microsoft::WRL;
using namespace AppInstaller::Utility::HttpStream;

namespace AppInstaller::Msix
{
    namespace
    {
        // Gets the version from the manifest reader.
        UINT64 GetVersionFromManifestReader(IAppxManifestReader* reader)
        {
            ComPtr<IAppxManifestPackageId> packageId;
            THROW_IF_FAILED(reader->GetPackageId(&packageId));

            UINT64 result = 0;
            THROW_IF_FAILED(packageId->GetVersion(&result));

            return result;
        }
    }

    bool GetBundleReader(
        IStream* inputStream,
        IAppxBundleReader** reader)
    {
        ComPtr<IAppxBundleFactory> bundleFactory;

        // Create a new Appxbundle factory
        THROW_IF_FAILED(CoCreateInstance(
            __uuidof(AppxBundleFactory),
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IAppxBundleFactory),
            (LPVOID*)(&bundleFactory)));

        HRESULT hr = bundleFactory->CreateBundleReader(inputStream, reader);

        if (SUCCEEDED(hr))
        {
            return true;
        }
        else if (hr == APPX_E_MISSING_REQUIRED_FILE)
        {
            // APPX_E_MISSING_REQUIRED_FILE returned when trying to open
            // an *.msix as an *.msixbundle or vice-versa.
            return false;
        }
        else
        {
            THROW_HR(hr);
        }
    }

    bool GetPackageReader(
        IStream* inputStream,
        IAppxPackageReader** reader)
    {
        ComPtr<IAppxFactory> appxFactory;

        // Create a new Appx factory
        THROW_IF_FAILED(CoCreateInstance(
            __uuidof(AppxFactory),
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IAppxFactory),
            (LPVOID*)(&appxFactory)));

        // Create a new package reader using the factory.
        HRESULT hr = appxFactory->CreatePackageReader(inputStream, reader);

        if (SUCCEEDED(hr))
        {
            return true;
        }
        else if (hr == APPX_E_MISSING_REQUIRED_FILE)
        {
            // APPX_E_MISSING_REQUIRED_FILE returned when trying to open
            // an *.msix as an *.msixbundle or vice-versa.
            return false;
        }
        else
        {
            THROW_HR(hr);
        }
    }

    void GetManifestReader(
        IStream* inputStream,
        IAppxManifestReader** reader)
    {
        ComPtr<IAppxFactory> appxFactory;

        THROW_IF_FAILED(CoCreateInstance(
            __uuidof(AppxFactory),
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IAppxFactory),
            (LPVOID*)(&appxFactory)));

        THROW_IF_FAILED(appxFactory->CreateManifestReader(inputStream, reader));
    }

    MsixInfo::MsixInfo(const std::string& uriStr)
    {
        // Get an IStream from the input uri and try to create package or bundler reader.
        winrt::Windows::Foundation::Uri uri(Utility::ConvertToUTF16(uriStr));
        IRandomAccessStream randomAccessStream = HttpRandomAccessStream::CreateAsync(uri).get();

        ::IUnknown* rasAsIUnknown = (::IUnknown*)winrt::get_abi(randomAccessStream);
        THROW_IF_FAILED(CreateStreamOverRandomAccessStream(
            rasAsIUnknown,
            IID_PPV_ARGS(m_stream.ReleaseAndGetAddressOf())));

        if (GetBundleReader(m_stream.Get(), &m_bundleReader))
        {
            m_isBundle = true;
        }
        else if (GetPackageReader(m_stream.Get(), &m_packageReader))
        {
            m_isBundle = false;
        }
        else
        {
            THROW_HR_MSG(HRESULT_FROM_WIN32(ERROR_INSTALL_OPEN_PACKAGE_FAILED),
                "Failed to open uri as msix package or bundle. Uri: %s", uriStr.c_str());
        }
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
        THROW_HR_IF(E_UNEXPECTED, stat.cbSize.HighPart != 0); // Signature size should be small
        signatureSize = stat.cbSize.LowPart;

        signatureContent.resize(signatureSize);

        DWORD signatureRead;
        THROW_IF_FAILED(signatureStream->Read(signatureContent.data(), signatureSize, &signatureRead));
        THROW_HR_IF_MSG(E_UNEXPECTED, signatureRead != signatureSize, "Failed to read the whole signature stream");

        return signatureContent;
    }

    std::string MsixInfo::GetPackageFamilyName()
    {
        ComPtr<IAppxManifestPackageId> packageId;
        if (m_isBundle)
        {
            ComPtr<IAppxBundleManifestReader> manifestReader;
            THROW_IF_FAILED(m_bundleReader->GetManifest(&manifestReader));
            THROW_IF_FAILED(manifestReader->GetPackageId(&packageId));
        }
        else
        {
            ComPtr<IAppxManifestReader> manifestReader;
            THROW_IF_FAILED(m_packageReader->GetManifest(&manifestReader));
            THROW_IF_FAILED(manifestReader->GetPackageId(&packageId));
        }

        wil::unique_cotaskmem_string familyName;
        THROW_IF_FAILED(packageId->GetPackageFamilyName(&familyName));

        return Utility::ConvertToUTF8(familyName.get());
    }

    bool MsixInfo::IsNewerThan(const std::filesystem::path& otherManifest)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, m_isBundle);

        ComPtr<IStream> otherStream;
        THROW_IF_FAILED(SHCreateStreamOnFileEx(otherManifest.c_str(), 
            STGM_READ | STGM_SHARE_DENY_WRITE | STGM_FAILIFTHERE, 0, FALSE, nullptr, &otherStream));

        ComPtr<IAppxManifestReader> otherReader;
        GetManifestReader(otherStream.Get(), &otherReader);

        ComPtr<IAppxManifestReader> manifestReader;
        THROW_IF_FAILED(m_packageReader->GetManifest(&manifestReader));

        return (GetVersionFromManifestReader(manifestReader.Get()) > GetVersionFromManifestReader(otherReader.Get()));
    }

    void MsixInfo::WriteToFile(std::string_view packageFile, const std::filesystem::path& target, IProgressCallback& progress)
    {

    }

    void MsixInfo::WriteManifestToFile(const std::filesystem::path& target, IProgressCallback& progress)
    {

    }
}
