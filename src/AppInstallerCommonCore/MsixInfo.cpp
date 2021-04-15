// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/AppInstallerMsixInfo.h"
#include "HttpStream/HttpRandomAccessStream.h"
#include "Public/AppInstallerDownloader.h"
#include "Public/AppInstallerLogging.h"
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

        // Gets the UINT64 version from the version struct.
        UINT64 GetVersionFromVersion(const winrt::Windows::ApplicationModel::PackageVersion& version)
        {
            UINT64 result = version.Major;
            result = (result << 16) | version.Minor;
            result = (result << 16) | version.Build;
            result = (result << 16) | version.Revision;

            return result;
        }

        // Writes the stream (from current location) to the given file.
        void WriteStreamToFile(IStream* stream, UINT64 expectedSize, const std::filesystem::path& target, IProgressCallback& progress)
        {
            std::filesystem::path tempFile = target;
            tempFile += ".dnld";

            {
                std::ofstream file(tempFile, std::ios_base::binary | std::ios_base::out | std::ios_base::trunc);

                constexpr ULONG bufferSize = 1 << 20;
                std::unique_ptr<char[]> buffer = std::make_unique<char[]>(bufferSize);

                UINT64 totalBytesRead = 0;

                while (!progress.IsCancelled())
                {
                    ULONG bytesRead = 0;
                    HRESULT hr = stream->Read(buffer.get(), bufferSize, &bytesRead);

                    if (bytesRead)
                    {
                        // If we got bytes, just accept them and keep going.
                        LOG_IF_FAILED(hr);

                        file.write(buffer.get(), bytesRead);
                        totalBytesRead += bytesRead;
                        progress.OnProgress(totalBytesRead, expectedSize, ProgressType::Bytes);
                    }
                    else
                    {
                        // If given a size, and we have read it all, quit
                        if (expectedSize && totalBytesRead == expectedSize)
                        {
                            break;
                        }

                        // If the stream returned an error, throw it
                        THROW_IF_FAILED(hr);

                        // If we were given a size and didn't reach it, throw our own error;
                        // otherwise assume that this is just normal EOF.
                        if (expectedSize)
                        {
                            THROW_WIN32(ERROR_HANDLE_EOF);
                        }
                        else
                        {
                            break;
                        }
                    }
                }
            }

            std::filesystem::path backupFile = target;
            backupFile += ".bkup";
            if (std::filesystem::exists(target))
            {
                if (std::filesystem::exists(backupFile))
                {
                    std::filesystem::remove(backupFile);
                }
                std::filesystem::rename(target, backupFile);
            }

            std::filesystem::rename(tempFile, target);
        }

        // Writes the appx file to the given file.
        void WriteAppxFileToFile(IAppxFile* appxFile, const std::filesystem::path& target, IProgressCallback& progress)
        {
            UINT64 size = 0;
            THROW_IF_FAILED(appxFile->GetSize(&size));

            ComPtr<IStream> stream;
            THROW_IF_FAILED(appxFile->GetStream(&stream));

            WriteStreamToFile(stream.Get(), size, target, progress);
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

    std::optional<std::string> GetPackageFullNameFromFamilyName(std::string_view familyName)
    {
        std::wstring pfn = Utility::ConvertToUTF16(familyName);
        UINT32 fullNameCount = 0;
        UINT32 bufferLength = 0;
        UINT32 properties = 0;

        LONG findResult = FindPackagesByPackageFamily(pfn.c_str(), PACKAGE_FILTER_HEAD, &fullNameCount, nullptr, &bufferLength, nullptr, &properties);
        if (findResult == ERROR_SUCCESS || fullNameCount == 0)
        {
            // No package found
            return {};
        }
        else if (findResult != ERROR_INSUFFICIENT_BUFFER)
        {
            THROW_WIN32(findResult);
        }
        else if (fullNameCount != 1)
        {
            // Don't directly error, let caller deal with it
            AICLI_LOG(Core, Error, << "Multiple packages found for family name: " << fullNameCount);
            return {};
        }

        // fullNameCount == 1 at this point
        PWSTR fullNamePtr;
        std::wstring buffer(static_cast<size_t>(bufferLength) + 1, L'\0');

        THROW_IF_WIN32_ERROR(FindPackagesByPackageFamily(pfn.c_str(), PACKAGE_FILTER_HEAD, &fullNameCount, &fullNamePtr, &bufferLength, &buffer[0], &properties));
        if (fullNameCount != 1 || bufferLength == 0)
        {
            // Something changed in between, abandon
            AICLI_LOG(Core, Error, << "Packages found for family name: " << fullNameCount);
            return {};
        }

        buffer.resize(bufferLength - 1);
        return Utility::ConvertToUTF8(buffer);
    }

    std::string GetPackageFamilyNameFromFullName(std::string_view fullName)
    {
        std::wstring result;
        result.resize(PACKAGE_FAMILY_NAME_MAX_LENGTH + 1);
        UINT32 size = static_cast<UINT32>(result.size());
        THROW_IF_WIN32_ERROR(PackageFamilyNameFromFullName(Utility::ConvertToUTF16(fullName).c_str(), &size, &result[0]));
        result.resize(size - 1);
        return Utility::ConvertToUTF8(result);
    }

    std::optional<std::filesystem::path> GetPackageLocationFromFullName(std::string_view fullName)
    {
        std::wstring fn = Utility::ConvertToUTF16(fullName);

        UINT32 length = 0;
        LONG returnVal = GetStagedPackagePathByFullName(fn.c_str(), &length, nullptr);
        if (returnVal != ERROR_INSUFFICIENT_BUFFER)
        {
            LOG_WIN32(returnVal);
            return {};
        }

        THROW_HR_IF(E_UNEXPECTED, length == 0);

        std::wstring result;
        result.resize(length);

        returnVal = GetStagedPackagePathByFullName(fn.c_str(), &length, &result[0]);
        if (returnVal != ERROR_SUCCESS)
        {
            LOG_WIN32(returnVal);
            return {};
        }

        result.resize(length - 1);
        return { result };
    }

    MsixInfo::MsixInfo(std::string_view uriStr)
    {
        if (Utility::IsUrlRemote(uriStr))
        {
            // Get an IStream from the input uri and try to create package or bundler reader.
            winrt::Windows::Foundation::Uri uri(Utility::ConvertToUTF16(uriStr));
            IRandomAccessStream randomAccessStream = HttpRandomAccessStream::CreateAsync(uri).get();

            ::IUnknown* rasAsIUnknown = (::IUnknown*)winrt::get_abi(randomAccessStream);
            THROW_IF_FAILED(CreateStreamOverRandomAccessStream(
                rasAsIUnknown,
                IID_PPV_ARGS(m_stream.ReleaseAndGetAddressOf())));
        }
        else
        {
            std::filesystem::path path(Utility::ConvertToUTF16(uriStr));
            THROW_IF_FAILED(SHCreateStreamOnFileEx(path.c_str(),
                STGM_READ | STGM_SHARE_DENY_WRITE | STGM_FAILIFTHERE, 0, FALSE, nullptr, &m_stream));
        }

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
                "Failed to open uri as msix package or bundle. Uri: %s", uriStr.data());
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

    std::string MsixInfo::GetPackageFullName()
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

        wil::unique_cotaskmem_string fullName;
        THROW_IF_FAILED(packageId->GetPackageFullName(&fullName));

        return Utility::ConvertToUTF8(fullName.get());
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

    bool MsixInfo::IsNewerThan(const winrt::Windows::ApplicationModel::PackageVersion& otherVersion)
    {
        THROW_HR_IF(E_NOT_VALID_STATE, m_isBundle);

        ComPtr<IAppxManifestReader> manifestReader;
        THROW_IF_FAILED(m_packageReader->GetManifest(&manifestReader));

        return (GetVersionFromManifestReader(manifestReader.Get()) > GetVersionFromVersion(otherVersion));
    }

    void MsixInfo::WriteToFile(std::string_view packageFile, const std::filesystem::path& target, IProgressCallback& progress)
    {
        std::wstring fileUTF16 = Utility::ConvertToUTF16(packageFile);

        ComPtr<IAppxFile> appxFile;
        if (m_isBundle)
        {
            THROW_IF_FAILED(m_bundleReader->GetPayloadPackage(fileUTF16.c_str(), &appxFile));
        }
        else
        {
            THROW_IF_FAILED(m_packageReader->GetPayloadFile(fileUTF16.c_str(), &appxFile));
        }

        WriteAppxFileToFile(appxFile.Get(), target, progress);
    }

    void MsixInfo::WriteManifestToFile(const std::filesystem::path& target, IProgressCallback& progress)
    {
        ComPtr<IAppxFile> appxFile;
        if (m_isBundle)
        {
            THROW_IF_FAILED(m_bundleReader->GetFootprintFile(APPX_BUNDLE_FOOTPRINT_FILE_TYPE_MANIFEST, &appxFile));
        }
        else
        {
            THROW_IF_FAILED(m_packageReader->GetFootprintFile(APPX_FOOTPRINT_FILE_TYPE_MANIFEST, &appxFile));
        }

        WriteAppxFileToFile(appxFile.Get(), target, progress);
    }
}
