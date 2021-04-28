// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
#include <AppInstallerProgress.h>
#include <winget/UserSettings.h>
#include <wil/result.h>

#include <filesystem>
#include <functional>
#include <string>

#define REQUIRE_THROWS_HR(_expr_, _hr_)     REQUIRE_THROWS_MATCHES(_expr_, wil::ResultException, ::TestCommon::ResultExceptionHRMatcher(_hr_))

namespace TestCommon
{
    enum class TempFileDestructionBehavior
    {
        Delete,
        Keep,
        ShellExecuteOnFailure,
    };

    // Use this to create a temporary file for testing.
    struct TempFile
    {
        TempFile(const std::string& baseName, const std::string& baseExt, bool deleteFileOnConstruction = true);
        TempFile(const std::filesystem::path& filePath, bool deleteFileOnConstruction = true);

        TempFile(const TempFile&) = delete;
        TempFile& operator=(const TempFile&) = delete;

        TempFile(TempFile&&) = delete;
        TempFile& operator=(TempFile&&) = delete;

        ~TempFile();

        const std::filesystem::path& GetPath() const { return _filepath; }
        operator const std::filesystem::path& () const { return _filepath; }
        operator const std::string() const { return _filepath.u8string(); }

        static void SetDestructorBehavior(TempFileDestructionBehavior behavior);

        static void SetTestFailed(bool failed);

    protected:
        TempFile() = default;
        std::filesystem::path _filepath;
    };

    // Use to create a temporary directory for testing.
    struct TempDirectory : public TempFile
    {
        TempDirectory(const std::string& baseName, bool create = true);
    };

    // Use this to find a test data file when testing.
    struct TestDataFile
    {
        TestDataFile(const std::filesystem::path& path) : m_path(path) {}

        std::filesystem::path GetPath() const;
        operator std::filesystem::path () const { return GetPath(); }

        static void SetTestDataBasePath(const std::filesystem::path& path);

    private:
        std::filesystem::path m_path;
    };

    // Matcher that lets us verify wil::ResultExceptions have a specific HR.
    struct ResultExceptionHRMatcher : public Catch::MatcherBase<wil::ResultException>
    {
        ResultExceptionHRMatcher(HRESULT hr) : m_expectedHR(hr) {}

        bool match(const wil::ResultException& re) const override
        {
            return re.GetErrorCode() == m_expectedHR;
        }

        std::string describe() const override
        {
            std::ostringstream result;
            result << "has HR == 0x" << AppInstaller::Logging::SetHRFormat << m_expectedHR;
            return result.str();
        }

    private:
        HRESULT m_expectedHR = S_OK;
    };

    // An IProgressCallback that is easily hooked.
    struct TestProgress : public AppInstaller::IProgressCallback
    {
        // Inherited via IProgressCallback
        void BeginProgress() override;
        
        void OnProgress(uint64_t current, uint64_t maximum, AppInstaller::ProgressType type) override;
        
        void EndProgress(bool) override;

        bool IsCancelled() override;
        CancelFunctionRemoval SetCancellationFunction(std::function<void()>&& f) override;

        std::function<void(uint64_t, uint64_t, AppInstaller::ProgressType)> m_OnProgress;
    };

    // Creates a volatile key for testing.
    wil::unique_hkey RegCreateVolatileTestRoot();

    // Creates a volatile subkey for testing.
    wil::unique_hkey RegCreateVolatileSubKey(HKEY parent, const std::wstring& name);

    // Set registry values.
    void SetRegistryValue(HKEY key, const std::wstring& name, const std::wstring& value, DWORD type = REG_SZ);
    void SetRegistryValue(HKEY key, const std::wstring& name, const std::vector<BYTE>& value, DWORD type = REG_BINARY);
    void SetRegistryValue(HKEY key, const std::wstring& name, DWORD value);

    // Override UserSettings using this class.
    // Automatically overrides the user settings for the lifetime of this object.
    // DOES NOT SUPPORT NESTED USE
    struct TestUserSettings : public AppInstaller::Settings::UserSettings
    {
        TestUserSettings(bool keepFileSettings = false);
        ~TestUserSettings();

        template <AppInstaller::Settings::Setting S>
        void Set(typename AppInstaller::Settings::details::SettingMapping<S>::value_t&& value)
        {
            m_settings[S].emplace<AppInstaller::Settings::details::SettingIndex(S)>(std::move(value));
        }
    };
}
