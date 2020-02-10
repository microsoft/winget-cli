// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/result.h>

#include <filesystem>
#include <string>

#define SQLITE_MEMORY_DB_CONNECTION_TARGET ":memory:"

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
        operator const std::string () const { return _filepath.u8string(); }

        static void SetDestructorBehavior(TempFileDestructionBehavior behavior);

        static void SetTestFailed(bool failed);

    private:
        std::filesystem::path _filepath;
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
            result << "has HR == 0x" << std::hex << std::setfill('0') << std::setw(8) << m_expectedHR;
            return result.str();
        }

    private:
        HRESULT m_expectedHR = S_OK;
    };
}
