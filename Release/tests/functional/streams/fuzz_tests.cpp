/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Fuzzing tests for streams read operations that involve parsing of data.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

using namespace concurrency::streams;

namespace tests
{
namespace functional
{
namespace streams
{
using namespace utility;
using namespace ::pplx;

SUITE(streams_fuzz_tests)
{
    std::string get_fuzzed_file_path(std::string requires_str)
    {
        std::string ipfile;

        if (UnitTest::GlobalSettings::Has(requires_str))
        {
            ipfile = UnitTest::GlobalSettings::Get(requires_str);
        }

        return ipfile;
    }

    concurrency::streams::basic_istream<char> get_input_stream(std::string requires_str)
    {
        utility::string_t ipfile = utility::conversions::to_string_t(get_fuzzed_file_path(requires_str));
        concurrency::streams::basic_istream<char> ifs;

        if (true == ipfile.empty())
        {
            VERIFY_IS_TRUE(false, "Input file is empty");
            return ifs;
        }

        ifs = concurrency::streams::file_stream<char>::open_istream(ipfile, std::ios::in).get();

        // Look for UTF-8 BOM
        if (ifs.read().get() != 0xEF || ifs.read().get() != 0xBB || ifs.read().get() != 0xBF)
        {
            VERIFY_IS_TRUE(false, "Input file encoding is not UTF-8. Test will not parse the file.");
            ifs.close().get();
        }
        return ifs;
    }

    TEST(fuzz_read_line, "Requires", "fuzz_read_line_ipfile", "Timeout", "600000")
    {
        auto ifs = get_input_stream("fuzz_read_line_ipfile");
        if (!ifs.is_valid() || !ifs.is_open()) return;

        size_t num_lines = 0;
        while (false == ifs.is_eof())
        {
            container_buffer<std::vector<uint8_t>> buf;
            ifs.read_line(buf).get();
            num_lines++;
        }
        ifs.close().get();
        std::wcout << U("Number of lines read:") << num_lines;
    }

    template<class T>
    void extract(const basic_istream<char>& ifs)
    {
        try
        {
            ifs.extract<T>().get();
        }
        catch (std::exception)
        {
        }
        return;
    }

    TEST(fuzz_extract, "Requires", "fuzz_extract_ipfile", "Timeout", "600000")
    {
        auto ifs = get_input_stream("fuzz_extract_ipfile");
        if (!ifs.is_valid() || !ifs.is_open()) return;

        int num_lines = 0;
        while (false == ifs.is_eof())
        {
            extract<std::string>(ifs);
            extract<std::string>(ifs);
            extract<unsigned int>(ifs);
            extract<uint64_t>(ifs);
            extract<bool>(ifs);
            extract<std::string>(ifs);
            extract<int>(ifs);
            container_buffer<std::vector<uint8_t>> buf;
            ifs.read_line(buf).get();
            num_lines++;
        }
        ifs.close().get();
        std::wcout << L"Number of lines read:" << num_lines << std::endl;
    }

} // SUITE(streams_fuzz_tests)

} // namespace streams
} // namespace functional
} // namespace tests
