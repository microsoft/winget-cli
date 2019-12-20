// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"

namespace TestCommon
{
    namespace
    {
        struct initRand
        {
            initRand() { srand(static_cast<unsigned int>(time(NULL))); }
        };

        inline int getRand()
        {
            static initRand srandHolder;
            return rand();
        }

        inline std::string GetTempFilePath(const std::string& baseName, const std::string& baseExt)
        {
            char tempPath[MAX_PATH]{};
            REQUIRE(GetTempPathA(MAX_PATH, tempPath) != 0);

            srand(static_cast<unsigned int>(time(NULL)));
            std::stringstream tempFileName;
            tempFileName << tempPath << '\\' << baseName << getRand() << baseExt;

            return tempFileName.str();
        }

        static bool s_TempFileDestructorKeepsFile{};
    }

    TempFile::TempFile(const std::string& baseName, const std::string& baseExt, bool deleteFileOnConstruction)
    {
        _filepath = GetTempFilePath(baseName, baseExt);
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
}
