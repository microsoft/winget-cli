// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

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

        inline std::filesystem::path GetTempFilePath(const std::string& baseName, const std::string& baseExt)
        {
            std::filesystem::path tempFilePath = std::filesystem::temp_directory_path();

            tempFilePath /= baseName + std::to_string(getRand()) + baseExt;

            return tempFilePath;
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
}
