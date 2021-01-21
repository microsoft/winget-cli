// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "TestCommon.h"
#include <winget/NameNormalization.h>

using namespace std::string_view_literals;
using namespace AppInstaller::Utility;


TEST_CASE("NormTest", "[name_norm]")
{
    std::ifstream names(R"(C:\Temp\NormTest\ArpNames.txt)");
    std::ifstream pubs(R"(C:\Temp\NormTest\ArpPubs.txt)");
    char name[1024];
    char pub[1024];

    NameNormalizer normer(NormalizationVersion::Initial);

    std::ofstream namesOut(R"(C:\Temp\NormTest\StdNames.txt)", std::ofstream::trunc | std::ofstream::binary);
    std::ofstream archOut(R"(C:\Temp\NormTest\StdArchs.txt)", std::ofstream::trunc | std::ofstream::binary);
    std::ofstream localeOut(R"(C:\Temp\NormTest\StdLocs.txt)", std::ofstream::trunc | std::ofstream::binary);
    std::ofstream pubsOut(R"(C:\Temp\NormTest\StdPubs.txt)", std::ofstream::trunc | std::ofstream::binary);

    while (names.getline(name, 1024) && pubs.getline(pub, 1024))
    {
        auto normed = normer.Normalize(name, pub);
        namesOut << normed.Name() << std::endl;
        archOut << ToString(normed.Architecture()) << std::endl;
        localeOut << normed.Locale() << std::endl;
        pubsOut << normed.Publisher() << std::endl;
    }
}
