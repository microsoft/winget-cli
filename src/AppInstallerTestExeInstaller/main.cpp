// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <iostream>
#include <fstream>
#include <filesystem>

int main(int argc, const char** argv)
{
    std::filesystem::path temp = std::filesystem::temp_directory_path();
    temp /= "TestExeInstalled.txt";
    std::ofstream file(temp, std::ofstream::out);

    for (int i = 1; i < argc; i++)
    {
        file << argv[i] << ' ';
    }

    file.close();
}
