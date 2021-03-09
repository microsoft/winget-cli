/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests for integration of async streams with std streams.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#include "cpprest/filestream.h"
#include "cpprest/producerconsumerstream.h"
#include "cpprest/rawptrstream.h"

#if (!defined(_WIN32) || !defined(CPPREST_EXCLUDE_WEBSOCKETS)) && !defined(__cplusplus_winrt)
#include <boost/interprocess/streams/bufferstream.hpp>
#endif

#if defined(__cplusplus_winrt)
using namespace Windows::Storage;
#endif

#ifdef _WIN32
#define DEFAULT_PROT (int)std::ios_base::_Openprot
#else
#define DEFAULT_PROT 0
#endif

namespace tests
{
namespace functional
{
namespace streams
{
using namespace ::pplx;
using namespace utility;

utility::string_t get_full_name(const utility::string_t& name);

template<typename CharType>
void extract_test(std::basic_istream<CharType>& stream, std::basic_string<CharType> expected)
{
    std::basic_string<CharType> s;
    stream >> s;
    VERIFY_ARE_EQUAL(s, expected);
}

// Used to prepare data for read tests

void fill_file(const utility::string_t& name, std::string text, size_t repetitions = 1)
{
    std::fstream stream(get_full_name(name), std::ios_base::out | std::ios_base::trunc);

    for (size_t i = 0; i < repetitions; i++)
        stream << text;
}

//
// The following functions will help mask the differences between non-WinRT environments and
// WinRT: on the latter, a file path is typically not used to open files. Rather, a UI element is used
// to get a 'StorageFile' reference and you go from there. However, to test the library properly,
// we need to get a StorageFile reference somehow, and one way to do that is to create all the files
// used in testing in the Documents folder.
//
template<typename _CharType>
pplx::task<Concurrency::streams::streambuf<_CharType>> OPEN_R(const utility::string_t& name)
{
#if !defined(__cplusplus_winrt)
    return Concurrency::streams::file_buffer<_CharType>::open(name, std::ios_base::in);
#else
    auto file =
        pplx::create_task(KnownFolders::DocumentsLibrary->GetFileAsync(ref new Platform::String(name.c_str()))).get();

    return Concurrency::streams::file_buffer<_CharType>::open(file, std::ios_base::in);
#endif
}

SUITE(stdstreambuf_tests)
{
    TEST(sync_on_async_write)
    {
        Concurrency::streams::stringstreambuf strbuf;
        auto ss = strbuf.create_ostream();
        Concurrency::streams::async_ostream<char> bios(ss);

        auto text = "hello!";

        bios.write(text, strlen(text));

        auto buf = ss.streambuf();

        VERIFY_ARE_EQUAL(strbuf.collection(), "hello!");
    }

    TEST(sync_on_async_put)
    {
        Concurrency::streams::stringstreambuf strbuf;
        auto ss = strbuf.create_ostream();
        Concurrency::streams::async_ostream<char> bios(ss);

        bios.put('h').put('e').put('l').put('l').put('o').put('!');

        VERIFY_ARE_EQUAL(strbuf.collection(), "hello!");
    }

    TEST(sync_on_async_insert)
    {
        Concurrency::streams::stringstreambuf strbuf;
        auto ss = strbuf.create_ostream();
        Concurrency::streams::async_ostream<char> bios(ss);

        bios << "hello"
             << ", there, this is " << 4711;

        VERIFY_ARE_EQUAL(strbuf.collection(), "hello, there, this is 4711");
        ss.close().wait();
    }

    TEST(sync_on_async_seekp)
    {
        Concurrency::streams::stringstreambuf strbuf;
        auto ss = strbuf.create_ostream();
        Concurrency::streams::async_ostream<char> bios(ss);

        bios << "hello"
             << ", there, this is " << 4711;

        bios.seekp(10);
        bios << 'X';

        VERIFY_ARE_EQUAL(strbuf.collection(), "hello, theXe, this is 4711");
        ss.close().wait();
    }

    TEST(sync_on_async_getline_1)
    {
        std::string s("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        auto ss = Concurrency::streams::stringstream::open_istream(s);

        Concurrency::streams::async_iostream<char> bios(ss.streambuf());

        char chars[128];
        bios.getline(chars, sizeof(chars));

        VERIFY_ARE_EQUAL(strcmp(chars, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"), 0);
    }

    TEST(sync_on_async_getline_2)
    {
        std::string s("abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ");
        auto ss = Concurrency::streams::stringstream::open_istream(s);

        Concurrency::streams::async_iostream<char> bios(ss.streambuf());

        char chars[128];

        bios.getline(chars, sizeof(chars));

        VERIFY_ARE_EQUAL(strcmp(chars, "abcdefghijklmnopqrstuvwxyz"), 0);

        VERIFY_ARE_EQUAL(bios.get(), 'A');
    }

    TEST(sync_on_async_getline_3)
    {
        std::string s("abcdefghijklmnopqrstuvwxyz|ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        auto ss = Concurrency::streams::stringstream::open_istream(s);

        Concurrency::streams::async_iostream<char> bios(ss.streambuf());

        char chars[128];

        bios.getline(chars, sizeof(chars), '|');

        VERIFY_ARE_EQUAL(strcmp(chars, "abcdefghijklmnopqrstuvwxyz"), 0);

        VERIFY_ARE_EQUAL(bios.get(), 'A');
    }

    TEST(sync_on_async_get_1)
    {
        std::string s("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
        auto ss = Concurrency::streams::stringstream::open_istream(s);

        Concurrency::streams::async_iostream<char> bios(ss.streambuf());

        char chars[128];

        bios.get(chars, sizeof(chars));

        VERIFY_ARE_EQUAL(strcmp(chars, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"), 0);
    }

    TEST(sync_on_async_fget_1)
    {
        utility::string_t fname = U("sync_on_async_fget_1.txt");
        fill_file(fname, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");

        auto ofs = OPEN_R<char>(fname).get();
        Concurrency::streams::async_istream<char> bios(ofs);

        char chars[128];

        bios.get(chars, sizeof(chars));

        VERIFY_ARE_EQUAL(strcmp(chars, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"), 0);
        ofs.close().wait();
    }

    TEST(sync_on_async_get_2)
    {
        std::string s("abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ");
        auto ss = Concurrency::streams::stringstream::open_istream(s);

        Concurrency::streams::async_iostream<char> bios(ss.streambuf());

        char chars[128];

        bios.get(chars, sizeof(chars));

        VERIFY_ARE_EQUAL(strcmp(chars, "abcdefghijklmnopqrstuvwxyz"), 0);

        VERIFY_ARE_EQUAL(bios.get(), '\n');
    }

    TEST(sync_on_async_get_3)
    {
        std::string s("abcdefghijklmnopqrstuvwxyz|ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        auto ss = Concurrency::streams::stringstream::open_istream(s);

        Concurrency::streams::async_iostream<char> bios(ss.streambuf());

        char chars[128];

        bios.get(chars, sizeof(chars), '|');

        VERIFY_ARE_EQUAL(strcmp(chars, "abcdefghijklmnopqrstuvwxyz"), 0);

        VERIFY_ARE_EQUAL(bios.get(), '|');
    }

    TEST(sync_on_async_extract_1)
    {
        auto ss = Concurrency::streams::stringstream::open_istream(std::string("abcdefg 10 1 9.4711"));

        Concurrency::streams::async_iostream<char> bios(ss.streambuf());

        std::string s;
        int i;
        bool b;
        double d;

        bios >> s >> i >> b >> d;

        VERIFY_ARE_EQUAL(s, "abcdefg");
        VERIFY_ARE_EQUAL(i, 10);
        VERIFY_IS_TRUE(b);
        VERIFY_ARE_EQUAL(d, 9.4711);
    }

    TEST(sync_on_async_fextract_1)
    {
        utility::string_t fname = U("sync_on_async_fextract_1.txt");
        fill_file(fname, "abcdefg 10 1 9.4711");

        auto ofs = OPEN_R<char>(fname).get();
        Concurrency::streams::async_istream<char> bios(ofs);

        std::string s;
        int i;
        bool b;
        double d;

        bios >> s >> i >> b >> d;

        VERIFY_ARE_EQUAL(s, "abcdefg");
        VERIFY_ARE_EQUAL(i, 10);
        VERIFY_IS_TRUE(b);
        VERIFY_ARE_EQUAL(d, 9.4711);

        ofs.close().wait();
    }

    TEST(sync_on_async_extract_2)
    {
        std::string s("abcdefg 10 1 9.4711");
        auto is = Concurrency::streams::stringstream::open_istream(s);

        Concurrency::streams::async_istream<char> ss(is.streambuf());
        extract_test<char>(ss, "abcdefg");

        is.close().wait();
    }

    TEST(sync_on_async_prodcons)
    {
        Concurrency::streams::producer_consumer_buffer<uint8_t> pcbuf;

        auto ostream = pcbuf.create_ostream();
        auto istream = pcbuf.create_istream();

        const std::streamsize iterations = 100;

        const std::string the_alphabet("abcdefghijklmnopqrstuvwxyz");

        auto writer = pplx::create_task([ostream, iterations, the_alphabet]() {
            auto os = ostream;
            for (std::streamsize i = 0; i < iterations; i++)
            {
                os.print(the_alphabet).wait();
                os.flush().wait();
            }
            os.close();
        });

        Concurrency::streams::async_istream<char> ss(istream.streambuf());

        char chars[1024];
        std::streamsize count = 0;

        while (!ss.eof())
        {
            memset(chars, 0, sizeof(chars));
            ss.read(chars, sizeof(chars) - 1);
            count += strlen(chars);
        }

        VERIFY_ARE_EQUAL(the_alphabet.size() * iterations, count);

        writer.wait();
    }

    TEST(sync_on_async_tellg)
    {
        Concurrency::streams::producer_consumer_buffer<uint8_t> pcbuf;

        auto ostream = pcbuf.create_ostream();
        auto istream = pcbuf.create_istream();

        const std::streamsize iterations = 100;

        const std::string the_alphabet("abcdefghijklmnopqrstuvwxyz");

        auto writer = pplx::create_task([ostream, iterations, the_alphabet]() {
            auto os = ostream;
            for (std::streamsize i = 0; i < iterations; i++)
            {
                os.print(the_alphabet).wait();
                os.flush().wait();
                VERIFY_ARE_EQUAL((i + 1) * the_alphabet.size(), os.tell());
            }
            os.close();
        });

        Concurrency::streams::async_istream<char> ss(istream.streambuf());

        char chars[1024];
        std::streamsize count = 0;

        while (!ss.eof())
        {
            VERIFY_ARE_EQUAL(count, ss.tellg());
            memset(chars, 0, sizeof(chars));
            ss.read(chars, sizeof(chars) - 1);
            count += strlen(chars);
        }

        VERIFY_ARE_EQUAL(the_alphabet.size() * iterations, count);

        writer.wait();
    }

    TEST(async_on_sync_read_1)
    {
        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << "abcdefghijklmnopqrstuvwxyz";

        for (char c = 'a'; c <= 'z'; c++)
        {
            char ch = (char)astream.read().get();
            VERIFY_ARE_EQUAL(c, ch);
        }

        astream.close().get();
    }

    TEST(async_on_sync_read_2)
    {
        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << "abcdefghijklmnopqrstuvwxyz";

        char buffer[128];
        Concurrency::streams::rawptr_buffer<char> txtbuf(buffer, 128);

        VERIFY_ARE_EQUAL(26, astream.read(txtbuf, 26).get());

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        VERIFY_ARE_EQUAL(0, astream.read(txtbuf, 26).get());

        astream.close().get();
    }

    TEST(async_on_sync_read_3)
    {
        Concurrency::streams::producer_consumer_buffer<char> trg;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << text;

        VERIFY_ARE_EQUAL(52, astream.read_to_delim(trg, '\n').get());

        char buffer[128];
        VERIFY_ARE_EQUAL(52, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }
        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'A', buffer[i + 26]);
        }

        astream.close().get();
    }

    TEST(async_on_sync_read_4)
    {
        Concurrency::streams::producer_consumer_buffer<char> trg;

        // There's one newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << text;

        VERIFY_ARE_EQUAL(26, astream.read_to_delim(trg, '\n').get());
        VERIFY_ARE_EQUAL('A', (char)astream.read().get());

        char buffer[128];
        VERIFY_ARE_EQUAL(26, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        astream.close().get();
    }

    TEST(async_on_sync_read_5)
    {
        Concurrency::streams::producer_consumer_buffer<char> trg;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << text;

        VERIFY_ARE_EQUAL(52, astream.read_to_delim(trg, '|').get());

        char buffer[128];
        VERIFY_ARE_EQUAL(52, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }
        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'A', buffer[i + 26]);
        }

        astream.close().get();
    }

    TEST(async_on_sync_read_6)
    {
        Concurrency::streams::producer_consumer_buffer<char> trg;

        // There's one delimiter in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz|ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << text;

        VERIFY_ARE_EQUAL(26, astream.read_to_delim(trg, '|').get());
        VERIFY_ARE_EQUAL('A', (char)astream.read().get());

        char buffer[128];
        VERIFY_ARE_EQUAL(26, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        astream.close().get();
    }

    TEST(async_on_sync_read_line_1)
    {
        Concurrency::streams::producer_consumer_buffer<char> trg;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << text;

        VERIFY_ARE_EQUAL(26, astream.read_line(trg).get());
        VERIFY_ARE_EQUAL('A', (char)astream.read().get());

        char buffer[128];
        VERIFY_ARE_EQUAL(26, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        astream.close().get();
    }

    TEST(async_on_sync_read_to_end_1)
    {
        Concurrency::streams::producer_consumer_buffer<char> trg;

        // There's one newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << text;

        VERIFY_ARE_EQUAL(53, astream.read_to_end(trg).get());

        char buffer[128];
        VERIFY_ARE_EQUAL(53, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'A', buffer[i + 27]);
        }

        astream.close().get();
    }

    TEST(ostream_write_single_char)
    {
        std::stringstream stream;

        Concurrency::streams::stdio_ostream<char> os(stream);

        bool elements_equal = true;

        for (char ch = 'a'; ch <= 'z'; ch++)
        {
            elements_equal = elements_equal && (ch == os.write(ch).get());
        }

        VERIFY_IS_TRUE(elements_equal);

        VERIFY_ARE_EQUAL(stream.str(), "abcdefghijklmnopqrstuvwxyz");

        os.close().get();
    }

    TEST(ostream_write_buffer)
    {
        std::stringstream stream;

        Concurrency::streams::stdio_ostream<char> os(stream);

        const char* text = "abcdefghijklmnopqrstuvwxyz";
        size_t len = strlen(text);

        Concurrency::streams::rawptr_buffer<char> txtbuf(text, len);

        VERIFY_ARE_EQUAL(os.write(txtbuf, len).get(), len);

        VERIFY_ARE_EQUAL(stream.str(), "abcdefghijklmnopqrstuvwxyz");

        os.close().get();
    }

    TEST(ostream_output_print_string)
    {
        std::stringstream stream;

        Concurrency::streams::stdio_ostream<char> os(stream);

        os.print("abcdefghijklmnopqrstuvwxyz").wait();

        VERIFY_ARE_EQUAL(stream.str(), "abcdefghijklmnopqrstuvwxyz");

        os.close().get();
    }

    TEST(ostream_output_print_types)
    {
        std::stringstream stream;

        Concurrency::streams::stdio_ostream<char> os(stream);

        auto a = os.print("data: ");
        auto b = os.print(10);
        auto c = os.print(",");
        auto d = os.print(true);
        (a && b && c && d).wait();

        VERIFY_ARE_EQUAL(stream.str(), "data: 10,1");

        os.close().get();
    }

    TEST(ostream_output_print_line_string)
    {
        std::stringstream stream;

        Concurrency::streams::stdio_ostream<char> os(stream);

        os.print_line("abcdefghijklmnopqrstuvwxyz").wait();

        VERIFY_ARE_EQUAL(stream.str(), "abcdefghijklmnopqrstuvwxyz\n");

        os.close().get();
    }

    TEST(ostream_output_print_line_types)
    {
        std::stringstream stream;

        Concurrency::streams::stdio_ostream<char> os(stream);

        auto a = os.print_line("data: ");
        auto b = os.print_line(10);
        auto c = os.print_line(",");
        auto d = os.print_line(true);
        (a && b && c && d).wait();

        VERIFY_ARE_EQUAL(stream.str(), "data: \n10\n,\n1\n");

        os.close().get();
    }

    TEST(istream_extract_string)
    {
        const char* text = " abc defgsf ";

        std::stringstream stream;
        stream << text;

        Concurrency::streams::stdio_istream<char> is(stream);

        std::string str1 = is.extract<std::string>().get();
        std::string str2 = is.extract<std::string>().get();

        VERIFY_ARE_EQUAL(str1, "abc");
        VERIFY_ARE_EQUAL(str2, "defgsf");

        is.close().get();
    }

    TEST(stdio_istream_error)
    {
        std::ifstream inFile;
        inFile.open("stdio_istream_error.txt");
        concurrency::streams::stdio_istream<char> is(inFile);

        concurrency::streams::container_buffer<std::string> buffer;
        VERIFY_ARE_EQUAL(0, is.read_to_end(buffer).get());
        VERIFY_IS_TRUE(is.is_eof());
        VERIFY_IS_TRUE(is.is_open());

        is.close().wait();
    }

    TEST(stdio_istream_setstate)
    {
        std::ifstream inFile;
        inFile.open("stdio_istream_setstate.txt");
        concurrency::streams::stdio_istream<char> is(inFile);
        inFile.setstate(std::ios::failbit);

        concurrency::streams::container_buffer<std::string> buffer;
        VERIFY_ARE_EQUAL(0, is.read_to_end(buffer).get());
        VERIFY_IS_TRUE(is.is_eof());
        VERIFY_IS_TRUE(is.is_open());

        is.close().wait();
    }

    TEST(stdio_istream_close)
    {
        std::ifstream inFile;
        inFile.open("stdio_istream_close.txt");
        concurrency::streams::stdio_istream<char> is(inFile);
        inFile.close();

        concurrency::streams::container_buffer<std::string> buffer;
        VERIFY_ARE_EQUAL(0, is.read_to_end(buffer).get());
        // Won't fix bug TFS 639208
        // VERIFY_IS_FALSE(is.is_open());
        VERIFY_IS_TRUE(is.is_eof());
    }

    TEST(sync_on_async_close_early)
    {
        concurrency::streams::container_buffer<std::string> buffer;
        concurrency::streams::async_ostream<char> os(buffer);
        buffer.close();

        os << 10 << std::endl;
        VERIFY_IS_TRUE((std::ios::badbit & os.rdstate()) == std::ios::badbit);
    }

    TEST(sync_on_async_close_with_exception)
    {
        const std::string& data("abc123");

        // Try with a read.
        {
            concurrency::streams::container_buffer<std::string> buffer(data);
            concurrency::streams::async_istream<char> inputStream(buffer);
            buffer.close(std::ios::in, std::make_exception_ptr(std::invalid_argument("test exception"))).wait();
            const size_t tempBufSize = 4;
            char tempBuf[tempBufSize];
            inputStream.read(&tempBuf[0], tempBufSize);
            VERIFY_ARE_EQUAL(std::ios::failbit | std::ios::eofbit, inputStream.rdstate());
        }

        // Try with a write.
        {
            concurrency::streams::container_buffer<std::string> buffer(data);
            concurrency::streams::async_ostream<char> outputStream(buffer);
            buffer.close(std::ios::in, std::make_exception_ptr(std::invalid_argument("test exception"))).wait();
            const size_t tempBufSize = 4;
            char tempBuf[tempBufSize];
            outputStream.write(&tempBuf[0], tempBufSize);
            VERIFY_ARE_EQUAL(std::ios::badbit, outputStream.rdstate());
        }
    }

#if (!defined(_WIN32) || !defined(CPPREST_EXCLUDE_WEBSOCKETS)) && !defined(__cplusplus_winrt)
    TEST(ostream_full_throw_exception)
    {
        char tgt_buffer[5];
        boost::interprocess::bufferstream limited_stream(
            tgt_buffer, sizeof(tgt_buffer), ::std::ios_base::out | std::ios_base::binary);
        concurrency::streams::stdio_ostream<char> os_wrapper(limited_stream);
        concurrency::streams::streambuf<char> os_streambuf = os_wrapper.streambuf();

        // There's one newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ";

        std::stringstream stream;
        Concurrency::streams::stdio_istream<char> astream(stream);

        stream << text;

        VERIFY_THROWS(astream.read_to_end(os_streambuf).get(), std::exception);
    }
#endif
}
} // namespace streams
} // namespace functional
} // namespace tests
