// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestMsixPackage.h"

#include <AppInstallerMsixInfo.h>

#pragma comment(lib, "urlmon.lib")

using namespace Microsoft::WRL;
using namespace AppInstaller;

namespace TestCommon
{
    namespace
    {
        // A 1x1 PNG. The manifest schema requires a logo and the packaging API requires every file
        // the manifest names to be present, so the smallest real image is the cheapest way to
        // satisfy both.
        constexpr uint8_t s_MinimalPng[] = {
            0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D, 0x49, 0x48, 0x44, 0x52,
            0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0x15, 0xC4,
            0x89, 0x00, 0x00, 0x00, 0x01, 0x73, 0x52, 0x47, 0x42, 0x00, 0xAE, 0xCE, 0x1C, 0xE9, 0x00, 0x00,
            0x00, 0x04, 0x67, 0x41, 0x4D, 0x41, 0x00, 0x00, 0xB1, 0x8F, 0x0B, 0xFC, 0x61, 0x05, 0x00, 0x00,
            0x00, 0x09, 0x70, 0x48, 0x59, 0x73, 0x00, 0x00, 0x0E, 0xC3, 0x00, 0x00, 0x0E, 0xC3, 0x01, 0xC7,
            0x6F, 0xA8, 0x64, 0x00, 0x00, 0x00, 0x0B, 0x49, 0x44, 0x41, 0x54, 0x18, 0x57, 0x63, 0x60, 0x00,
            0x02, 0x00, 0x00, 0x05, 0x00, 0x01, 0xAA, 0xD5, 0xC8, 0x51, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45,
            0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82,
        };

        constexpr std::wstring_view s_LogoPath = L"Assets\\AppPackageStoreLogo.png";
        constexpr std::wstring_view s_IndexPath = L"Public\\index.db";

        // The hash method the block map is built with. The packaging API will not choose one on
        // our behalf, so it has to be named explicitly.
        constexpr std::wstring_view s_Sha256Uri = L"http://www.w3.org/2001/04/xmlenc#sha256";

        ComPtr<IStream> CreateMemoryStream(const void* data, size_t size)
        {
            ComPtr<IStream> result;

            THROW_IF_FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &result));

            if (size)
            {
                ULONG written = 0;
                THROW_IF_FAILED(result->Write(data, static_cast<ULONG>(size), &written));
                THROW_HR_IF(E_UNEXPECTED, written != size);
            }

            LARGE_INTEGER start{};
            THROW_IF_FAILED(result->Seek(start, STREAM_SEEK_SET, nullptr));

            return result;
        }

        std::vector<uint8_t> ReadFileBytes(const std::filesystem::path& path)
        {
            std::ifstream stream{ path, std::ios::in | std::ios::binary };
            THROW_HR_IF(HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND), !stream);
            return std::vector<uint8_t>{ std::istreambuf_iterator<char>{ stream }, std::istreambuf_iterator<char>{} };
        }

        void WriteStreamToFile(IStream* stream, const std::filesystem::path& path)
        {
            LARGE_INTEGER start{};
            THROW_IF_FAILED(stream->Seek(start, STREAM_SEEK_SET, nullptr));

            STATSTG stat{};
            THROW_IF_FAILED(stream->Stat(&stat, STATFLAG_NONAME));

            std::vector<uint8_t> buffer(static_cast<size_t>(stat.cbSize.QuadPart));

            if (!buffer.empty())
            {
                ULONG read = 0;
                THROW_IF_FAILED(stream->Read(buffer.data(), static_cast<ULONG>(buffer.size()), &read));
                THROW_HR_IF(E_UNEXPECTED, read != buffer.size());
            }

            std::filesystem::create_directories(path.parent_path());

            std::ofstream file{ path, std::ios::out | std::ios::binary | std::ios::trunc };
            THROW_HR_IF(E_INVALIDARG, !file);

            if (!buffer.empty())
            {
                file.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
            }

            file.close();
        }

        std::string CreateManifest(const TestIndexPackageDefinition& definition)
        {
            // Modelled on the checked in test package's manifest, so that a built package and the
            // existing test data differ only in the values a test chooses.
            std::ostringstream manifest;
            manifest <<
                R"(<?xml version="1.0" encoding="utf-8"?>)" "\n"
                R"(<Package xmlns="http://schemas.microsoft.com/appx/manifest/foundation/windows10">)" "\n"
                R"(  <Identity Name=")" << definition.Name << R"(")" "\n"
                R"(            ProcessorArchitecture="neutral")" "\n"
                R"(            Publisher=")" << definition.Publisher << R"(")" "\n"
                R"(            Version=")" << definition.Version << R"(" />)" "\n"
                R"(  <Properties>)" "\n"
                R"(    <DisplayName>Fake index for tests</DisplayName>)" "\n"
                R"(    <PublisherDisplayName>Microsoft Corporation</PublisherDisplayName>)" "\n"
                R"(    <Logo>Assets\AppPackageStoreLogo.png</Logo>)" "\n"
                R"(  </Properties>)" "\n"
                R"(  <Dependencies>)" "\n"
                R"(    <TargetDeviceFamily Name="Windows.Universal" MinVersion="10.0.16299.0" MaxVersionTested="10.0.18287.0" />)" "\n"
                R"(  </Dependencies>)" "\n"
                R"(  <Resources>)" "\n"
                R"(    <Resource Language="en-US" />)" "\n"
                R"(  </Resources>)" "\n"
                R"(</Package>)" "\n";

            return manifest.str();
        }
    }

    std::string CreateTestIndexPackage(
        const std::filesystem::path& outputPath,
        const std::filesystem::path& indexFile,
        const TestIndexPackageDefinition& definition)
    {
        ComPtr<IAppxFactory> factory;
        THROW_IF_FAILED(CoCreateInstance(
            __uuidof(AppxFactory),
            nullptr,
            CLSCTX_INPROC_SERVER,
            __uuidof(IAppxFactory),
            reinterpret_cast<void**>(factory.GetAddressOf())));

        ComPtr<IUri> hashMethod;
        THROW_IF_FAILED(CreateUri(s_Sha256Uri.data(), Uri_CREATE_CANONICALIZE, 0, &hashMethod));

        APPX_PACKAGE_SETTINGS settings{};
        settings.forceZip32 = TRUE;
        settings.hashMethod = hashMethod.Get();

        // The package is assembled in memory and written out only once it is complete, so a failure
        // partway through leaves nothing at the output path for a later test to pick up.
        ComPtr<IStream> packageStream = CreateMemoryStream(nullptr, 0);

        ComPtr<IAppxPackageWriter> writer;
        THROW_IF_FAILED(factory->CreatePackageWriter(packageStream.Get(), &settings, &writer));

        std::vector<uint8_t> indexBytes = ReadFileBytes(indexFile);
        ComPtr<IStream> indexStream = CreateMemoryStream(indexBytes.data(), indexBytes.size());

        THROW_IF_FAILED(writer->AddPayloadFile(
            s_IndexPath.data(),
            L"application/octet-stream",
            APPX_COMPRESSION_OPTION_NORMAL,
            indexStream.Get()));

        ComPtr<IStream> logoStream = CreateMemoryStream(s_MinimalPng, sizeof(s_MinimalPng));

        THROW_IF_FAILED(writer->AddPayloadFile(
            s_LogoPath.data(),
            L"image/png",
            APPX_COMPRESSION_OPTION_NONE,
            logoStream.Get()));

        std::string manifest = CreateManifest(definition);
        ComPtr<IStream> manifestStream = CreateMemoryStream(manifest.data(), manifest.size());

        // Close validates the manifest and writes the block map, so anything malformed is reported
        // here rather than when the package is later read.
        THROW_IF_FAILED(writer->Close(manifestStream.Get()));

        WriteStreamToFile(packageStream.Get(), outputPath);

        Msix::MsixInfo info{ outputPath };
        return Msix::GetPackageFamilyNameFromFullName(info.GetPackageFullName());
    }
}
