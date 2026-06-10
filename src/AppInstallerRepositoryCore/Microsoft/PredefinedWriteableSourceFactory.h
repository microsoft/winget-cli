// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include "ISource.h"
#include "SourceFactory.h"

#include <string_view>

namespace AppInstaller::Repository::Microsoft
{
    using namespace std::string_view_literals;
    
    // A source of installing packages on the local system.
    // Arg  ::  A value indicating the type of writeable source
    // Data ::  Not used.
    struct PredefinedWriteableSourceFactory
    {
        // Get the type string for this source.
        static constexpr std::string_view Type()
        {
            return "Microsoft.Predefined.Writeable"sv;
        }

        // The type for the source.
        enum class WriteableType
        {
            Installing
        };

        // Converts a type to its string.
        static std::string_view TypeToString(WriteableType type);

        // Creates a source factory for this type.
        static std::unique_ptr<ISourceFactory> Create();
    };
}
