// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <iostream>
#include <fstream>
#include <filesystem>
#include <sstream>

// The installer simply prints all args to an output file
int main(int argc, const char** argv)
{
    std::filesystem::path outFilePath = std::filesystem::temp_directory_path();
    std::stringstream outContent;

    for (int i = 1; i < argc; i++)
    {
        outContent << argv[i] << ' ';

        // Supports custom install path.
        if (_stricmp(argv[i], "/InstallDir") == 0 && ++i < argc)
        {
            outFilePath = argv[i];
            outContent << argv[i] << ' ';
        }
    }

    outFilePath /= "TestExeInstalled.txt";
    std::ofstream file(outFilePath, std::ofstream::out);

    file << outContent.str();

    file.close();
}
