// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "pch.h"
#include <string>

#define SQLITE_MEMORY_DB_CONNECTION_TARGET ":memory:"

namespace TestCommon
{
    struct TempFile
    {
        TempFile(const std::string& baseName, const std::string& baseExt, bool deleteFileOnConstruction = true);

        TempFile(const TempFile&) = delete;
        TempFile& operator=(const TempFile&) = delete;

        TempFile(TempFile&&) = delete;
        TempFile& operator=(TempFile&&) = delete;

        ~TempFile();

        const std::string& GetPath() const { return _filepath; }
        operator const std::string& () const { return _filepath; }

        static void SetDestructorBehavior(bool keepFilesOnDestruction);

    private:
        std::string _filepath;
    };
}
