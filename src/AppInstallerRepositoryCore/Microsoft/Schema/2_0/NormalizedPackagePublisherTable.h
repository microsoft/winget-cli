// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "Microsoft/Schema/2_0/SystemReferenceStringTable.h"


namespace AppInstaller::Repository::Microsoft::Schema::V2_0
{
    namespace details
    {
        using namespace std::string_view_literals;

        struct NormalizedPackagePublisherTableInfo
        {
            inline static constexpr std::string_view TableName() { return "norm_publishers2"sv; }
            inline static constexpr std::string_view ValueName() { return "norm_publisher"sv; }
        };
    }

    using NormalizedPackagePublisherTable = SystemReferenceStringTable<details::NormalizedPackagePublisherTableInfo>;
}
