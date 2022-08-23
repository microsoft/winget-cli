// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include "TestHooks.h"
#include <winget/GroupPolicy.h>
#include <winget/UserSettings.h>
#include <AppInstallerMsixInfo.h>
#include <AppInstallerDownloader.h>

namespace TestCommon
{
    namespace
    {
        int initRand()
        {
            srand(static_cast<unsigned int>(time(NULL)));
            return rand();
        };

        inline int getRand()
        {
            static int randStart = initRand();
            return randStart++;
        }

        inline std::filesystem::path GetFilePath(std::filesystem::path path, const std::string& baseName, const std::string& baseExt)
        {
            path /= baseName + std::to_string(getRand()) + baseExt;
            return path;
        }

        inline std::filesystem::path GetTempFilePath(const std::string& baseName, const std::string& baseExt)
        {
            std::filesystem::path tempFilePath = std::filesystem::temp_directory_path();
            return GetFilePath(tempFilePath, baseName, baseExt);
        }

        static TempFileDestructionBehavior s_TempFileDestructorBehavior = TempFileDestructionBehavior::Delete;
        static std::vector<std::filesystem::path> s_TempFilesOnFile;

        static std::filesystem::path s_TestDataFileBasePath{};

        bool CleanVolatileTestRoot(HKEY root)
        {
            THROW_IF_WIN32_ERROR(RegDeleteTreeW(root, nullptr));
            return true;
        }
    }

    TempFile::TempFile(const std::string& baseName, const std::string& baseExt, bool deleteFileOnConstruction)
    {
        _filepath = GetTempFilePath(baseName, baseExt);
        if (deleteFileOnConstruction)
        {
            std::filesystem::remove(_filepath);
        }
    }

    TempFile::TempFile(const std::filesystem::path& parent, const std::string& baseName, const std::string& baseExt, bool deleteFileOnConstruction)
    {
        _filepath = GetFilePath(parent, baseName, baseExt);
        if (deleteFileOnConstruction)
        {
            std::filesystem::remove(_filepath);
        }
    }

    TempFile::TempFile(const std::filesystem::path& filePath, bool deleteFileOnConstruction)
    {
        if (filePath.is_relative())
        {
            _filepath = std::filesystem::temp_directory_path();
            _filepath /= filePath;
        }
        else
        {
            _filepath = filePath;
        }
        if (deleteFileOnConstruction)
        {
            std::filesystem::remove(_filepath);
        }
    }

    TempFile::~TempFile()
    {
        switch (s_TempFileDestructorBehavior)
        {
        case TempFileDestructionBehavior::Delete:
            std::filesystem::remove_all(_filepath);
            break;
        case TempFileDestructionBehavior::Keep:
            break;
        case TempFileDestructionBehavior::ShellExecuteOnFailure:
            s_TempFilesOnFile.emplace_back(std::move(_filepath));
            break;
        }
    }

    void TempFile::Rename(const std::filesystem::path& newFilePath)
    {
        std::filesystem::rename(GetPath(), newFilePath);
        _filepath = newFilePath;
    }

    void TempFile::SetDestructorBehavior(TempFileDestructionBehavior behavior)
    {
        s_TempFileDestructorBehavior = behavior;
    }

    void TempFile::SetTestFailed(bool failed)
    {
        if (failed)
        {
            for (const auto& path : s_TempFilesOnFile)
            {
                SHELLEXECUTEINFOW seinfo{};
                seinfo.cbSize = sizeof(seinfo);
                seinfo.lpVerb = L"open";
                seinfo.lpFile = path.c_str();

                ShellExecuteExW(&seinfo);
            }
        }
        else
        {
            s_TempFilesOnFile.clear();
        }
    }

    TempDirectory::TempDirectory(const std::string& baseName, bool create)
    {
        _filepath = GetTempFilePath(baseName, "");
        if (create)
        {
            if (std::filesystem::exists(_filepath))
            {
                std::filesystem::remove_all(_filepath);
            }
            std::filesystem::create_directories(_filepath);
        }
    }

    std::filesystem::path TestDataFile::GetPath() const
    {
        std::filesystem::path result = s_TestDataFileBasePath;
        result /= m_path;
        return result;
    }

    void TestDataFile::SetTestDataBasePath(const std::filesystem::path& path)
    {
        s_TestDataFileBasePath = path;
    }

    void TestProgress::OnProgress(uint64_t current, uint64_t maximum, AppInstaller::ProgressType type)
    {
        if (m_OnProgress)
        {
            m_OnProgress(current, maximum, type);
        }
    }

    void TestProgress::BeginProgress()
    {
    }

    void TestProgress::EndProgress(bool)
    {
    }

    bool TestProgress::IsCancelled()
    {
        return false;
    }

    AppInstaller::IProgressCallback::CancelFunctionRemoval TestProgress::SetCancellationFunction(std::function<void()>&&)
    {
        return {};
    }

    wil::unique_hkey RegCreateVolatileTestRoot()
    {
        // First create/open the real test root
        wil::unique_hkey root;
        THROW_IF_WIN32_ERROR(RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\WinGet\\TestRoot", 0, nullptr, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, &root, nullptr));

        static bool s_ignored = CleanVolatileTestRoot(root.get());

        // Create a random name
        GUID name{};
        (void)CoCreateGuid(&name);

        wchar_t nameBuffer[256];
        (void)StringFromGUID2(name, nameBuffer, ARRAYSIZE(nameBuffer));

        return RegCreateVolatileSubKey(root.get(), nameBuffer);
    }

    wil::unique_hkey RegCreateVolatileSubKey(HKEY parent, const std::wstring& name)
    {
        wil::unique_hkey result;
        THROW_IF_WIN32_ERROR(RegCreateKeyExW(parent, name.c_str(), 0, nullptr, REG_OPTION_VOLATILE, KEY_ALL_ACCESS, nullptr, &result, nullptr));
        return result;
    }

    void SetRegistryValue(HKEY key, const std::wstring& name, const std::wstring& value, DWORD type)
    {
        THROW_IF_WIN32_ERROR(RegSetValueExW(key, name.c_str(), 0, type, reinterpret_cast<const BYTE*>(value.c_str()), static_cast<DWORD>(sizeof(wchar_t) * (value.size() + 1))));
    }

    void SetRegistryValue(HKEY key, const std::wstring& name, const std::vector<BYTE>& value, DWORD type)
    {
        THROW_IF_WIN32_ERROR(RegSetValueExW(key, name.c_str(), 0, type, reinterpret_cast<const BYTE*>(value.data()), static_cast<DWORD>(value.size())));
    }

    void SetRegistryValue(HKEY key, const std::wstring& name, DWORD value)
    {
        THROW_IF_WIN32_ERROR(RegSetValueExW(key, name.c_str(), 0, REG_DWORD, reinterpret_cast<const BYTE*>(&value), sizeof(DWORD)));
    }

    void EnableDevMode(bool enable)
    {
        wil::unique_hkey result;
        THROW_IF_WIN32_ERROR(RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\AppModelUnlock", 0, KEY_ALL_ACCESS|KEY_WOW64_64KEY, &result));
        SetRegistryValue(result.get(), L"AllowDevelopmentWithoutDevLicense", (enable ? 1 : 0));
    }

    TestUserSettings::TestUserSettings(bool keepFileSettings)
    {
        if (!keepFileSettings)
        {
            m_settings.clear();
        }

        AppInstaller::Settings::SetUserSettingsOverride(this);
    }

    TestUserSettings::~TestUserSettings()
    {
        AppInstaller::Settings::SetUserSettingsOverride(nullptr);
    }

    bool InstallCertFromSignedPackage(const std::filesystem::path& package)
    {
        auto [certContext, certStore] = AppInstaller::Msix::GetCertContextFromMsix(package);

        wil::unique_hcertstore trustedPeopleStore;
        trustedPeopleStore.reset(CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            L"TrustedPeople"));
        THROW_LAST_ERROR_IF(!trustedPeopleStore.get());

        wil::unique_cert_context existingCert;
        existingCert.reset(CertFindCertificateInStore(
            trustedPeopleStore.get(),
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            0,
            CERT_FIND_EXISTING,
            certContext.get(),
            nullptr));

        // Add if it does not already exist in the store
        if (!existingCert.get())
        {
            THROW_LAST_ERROR_IF(!CertAddCertificateContextToStore(
                trustedPeopleStore.get(),
                certContext.get(),
                CERT_STORE_ADD_NEW,
                nullptr));

            return true;
        }

        return false;
    }

    bool UninstallCertFromSignedPackage(const std::filesystem::path& package)
    {
        auto [certContext, certStore] = AppInstaller::Msix::GetCertContextFromMsix(package);

        wil::unique_hcertstore trustedPeopleStore;
        trustedPeopleStore.reset(CertOpenStore(
            CERT_STORE_PROV_SYSTEM_W,
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            NULL,
            CERT_SYSTEM_STORE_LOCAL_MACHINE,
            L"TrustedPeople"));
        THROW_LAST_ERROR_IF(!trustedPeopleStore.get());

        wil::unique_cert_context existingCert;
        existingCert.reset(CertFindCertificateInStore(
            trustedPeopleStore.get(),
            PKCS_7_ASN_ENCODING | X509_ASN_ENCODING,
            0,
            CERT_FIND_EXISTING,
            certContext.get(),
            nullptr));

        // Remove if it exists in the store
        if (existingCert.get())
        {
            THROW_LAST_ERROR_IF(!CertDeleteCertificateFromStore(existingCert.get()));

            return true;
        }

        return false;
    }

    bool GetMsixPackageManifestReader(std::string_view testFileName, IAppxManifestReader** manifestReader)
    {
        // Locate test file
        TestDataFile testFile(testFileName);
        auto path = testFile.GetPath().u8string();

        // Get the stream for the test file
        auto stream = AppInstaller::Utility::GetReadOnlyStreamFromURI(path);

        // Get manifest from package reader
        Microsoft::WRL::ComPtr<IAppxPackageReader> packageReader;
        return  AppInstaller::Msix::GetPackageReader(stream.Get(), &packageReader)
            && SUCCEEDED(packageReader->GetManifest(manifestReader));
    }
}
