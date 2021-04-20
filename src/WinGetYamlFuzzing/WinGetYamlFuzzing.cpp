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
        AppInstaller::Manifest::Manifest manifest = AppInstaller::Manifest::YamlParser::Create(input, false);
    }
    catch (...) {}

    return 0;
}

#ifndef WINGET_DISABLE_FOR_FUZZING

#include <filesystem>
#include <AppInstallerStrings.h>

// Emulate libFuzzer main by just sending all files in the corpus (last arg) to the fuzzer.
int main(int argc, char** argv)
{
    if (argc <= 1)
    {
        return 1;
    }

    std::filesystem::path corpus = argv[argc - 1];

    if (std::filesystem::is_directory(corpus))
    {
        for (auto& file : std::filesystem::directory_iterator{ corpus })
        {
            if (!file.is_directory())
            {
                std::ifstream stream{ file.path(), std::ios_base::in | std::ios_base::binary };
                std::string contents = AppInstaller::Utility::ReadEntireStream(stream);

                LLVMFuzzerTestOneInput(reinterpret_cast<const uint8_t*>(contents.data()), contents.size());
            }
        }
    }
    else
    {
        std::ifstream stream{ corpus, std::ios_base::in | std::ios_base::binary };
        std::string contents = AppInstaller::Utility::ReadEntireStream(stream);

        LLVMFuzzerTestOneInput(reinterpret_cast<const uint8_t*>(contents.data()), contents.size());
    }

    return 0;
}

#endif
