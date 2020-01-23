// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

#include <iostream>
#include <fstream>

int main(int argc, const char** argv)
{
    std::ofstream file("TestExeInstalled.txt", std::ofstream::out);

    for (int i = 1; i < argc; i++)
    {
        file << argv[i] << ' ';
    }

    file.close();
}
