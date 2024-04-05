// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Microsoft/Schema/ISQLiteIndex.h"

#include "Microsoft/Schema/1_0/Interface.h"
#include "Microsoft/Schema/1_1/Interface.h"
#include "Microsoft/Schema/1_2/Interface.h"
#include "Microsoft/Schema/1_3/Interface.h"
#include "Microsoft/Schema/1_4/Interface.h"
#include "Microsoft/Schema/1_5/Interface.h"
#include "Microsoft/Schema/1_6/Interface.h"
#include "Microsoft/Schema/1_7/Interface.h"
#include "Microsoft/Schema/2_0/Interface.h"

namespace AppInstaller::Repository::Microsoft::Schema
{
    std::unique_ptr<Schema::ISQLiteIndex> CreateISQLiteIndex(const SQLite::Version& version)
    {
        using namespace Schema;

        if (version.MajorVersion == 1 ||
            version.IsLatest())
        {
            constexpr std::array<std::unique_ptr<Schema::ISQLiteIndex>(*)(), 8> versionCreatorMap =
            {
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V1_0::Interface>()); },
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V1_1::Interface>()); },
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V1_2::Interface>()); },
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V1_3::Interface>()); },
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V1_4::Interface>()); },
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V1_5::Interface>()); },
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V1_6::Interface>()); },
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V1_7::Interface>()); },
            };

            return versionCreatorMap[std::min(static_cast<size_t>(version.MinorVersion), versionCreatorMap.size() - 1)]();
        }

        // Version 2.0 is designed solely for minimizing the size of the index for transport.
        // Unless it is prepared for packaging, it will be identical to a 1.N index.
        if (version.MajorVersion == 2)
        {
            constexpr std::array<std::unique_ptr<Schema::ISQLiteIndex>(*)(), 1> versionCreatorMap =
            {
                []() { return std::unique_ptr<Schema::ISQLiteIndex>(std::make_unique<V2_0::Interface>()); },
            };

            return versionCreatorMap[std::min(static_cast<size_t>(version.MinorVersion), versionCreatorMap.size() - 1)]();
        }

        // We do not have the capacity to operate on this schema version
        THROW_HR(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    }
}
