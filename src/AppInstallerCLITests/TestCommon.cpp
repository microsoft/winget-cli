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

        inline std::string GetTempFilePath(const std::string& baseName, const std::string& baseExt, bool useRand, bool useCurrentDir)
        {
            std::stringstream tempFileName;

            if (useCurrentDir)
            {
                tempFileName << std::filesystem::current_path().string();
            }
            else
            {
                char tempPath[MAX_PATH]{};
                REQUIRE(GetTempPathA(MAX_PATH, tempPath) != 0);
                tempFileName << tempPath;
            }

            tempFileName << '\\' << baseName;

            if (useRand)
            {
                srand(static_cast<unsigned int>(time(NULL)));
                tempFileName << getRand();
            }

            tempFileName << baseExt;

            return tempFileName.str();
        }

        static bool s_TempFileDestructorKeepsFile{};

        static std::filesystem::path s_TestDataFileBasePath{};
    }

    TempFile::TempFile(const std::string& baseName, const std::string& baseExt, bool deleteFileOnConstruction, bool useRand, bool useCurrentDir)
    {
        _filepath = GetTempFilePath(baseName, baseExt, useRand, useCurrentDir);
        if (deleteFileOnConstruction)
        {
            DeleteFileA(_filepath.c_str());
        }
    }

    TempFile::~TempFile()
    {
        if (!s_TempFileDestructorKeepsFile)
        {
            DeleteFileA(_filepath.c_str());
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
