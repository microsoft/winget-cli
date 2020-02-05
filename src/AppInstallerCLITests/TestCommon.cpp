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

            srand(static_cast<unsigned int>(time(NULL)));
            tempFilePath /= baseName + std::to_string(getRand()) + baseExt;

            return tempFilePath;
        }

        static bool s_TempFileDestructorKeepsFile{};

        static std::filesystem::path s_TestDataFileBasePath{};
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
        _filepath = filePath;
        if (deleteFileOnConstruction)
        {
            std::filesystem::remove(_filepath);
        }
    }

    TempFile::~TempFile()
    {
        if (!s_TempFileDestructorKeepsFile)
        {
            std::filesystem::remove(_filepath);
        }
    }

    void TempFile::SetDestructorBehavior(bool keepFilesOnDestruction)
    {
        s_TempFileDestructorKeepsFile = keepFilesOnDestruction;
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
}
