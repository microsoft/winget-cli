// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include <cstdint>
#include <string>
#include <winget/ManifestYamlParser.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t * data, size_t size)
{
    std::string input{ reinterpret_cast<const char*>(data), size };

    try
    {
        AppInstaller::Manifest::Manifest manifest = AppInstaller::Manifest::YamlParser::Create(input, true);
    }
    catch (...) {}

    return 0;
}
