/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests for async input stream operations.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#include "unittestpp.h"
#include <float.h>

#ifdef max
#undef max
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
using namespace concurrency::streams;

// Used to prepare data for file-stream read tests

utility::string_t get_full_name(const utility::string_t& name)
{
#if defined(__cplusplus_winrt)
    // On WinRT, we must compensate for the fact that we will be accessing files in the
    // Documents folder
    auto file = pplx::create_task(KnownFolders::DocumentsLibrary->CreateFileAsync(
                                      ref new Platform::String(name.c_str()), CreationCollisionOption::ReplaceExisting))
                    .get();
    return file->Path->Data();
#else
    return name;
#endif
}

void fill_file(const utility::string_t& name, size_t repetitions = 1)
{
    std::fstream stream(get_full_name(name), std::ios_base::out | std::ios_base::trunc);

    for (size_t i = 0; i < repetitions; i++)
        stream << "abcdefghijklmnopqrstuvwxyz";
}

void fill_file_with_lines(const utility::string_t& name, const std::string& end, size_t repetitions = 1)
{
    std::fstream stream(get_full_name(name), std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);

    for (size_t i = 0; i < repetitions; i++)
        stream << "abcdefghijklmnopqrstuvwxyz" << end;
}

#ifdef _WIN32

// Disabling warning in test because we check for nullptr.
#pragma warning(push)
#pragma warning(disable : 6387)
void fill_file_w(const utility::string_t& name, size_t repetitions = 1)
{
    FILE* stream = nullptr;
    _wfopen_s(&stream, get_full_name(name).c_str(), L"w");
    if (stream == nullptr)
    {
        VERIFY_IS_TRUE(false, "FILE pointer is null");
    }

    for (size_t i = 0; i < repetitions; i++)
        for (wchar_t ch = L'a'; ch <= L'z'; ++ch)
            fwrite(&ch, sizeof(wchar_t), 1, stream);

    fclose(stream);
}
#pragma warning(pop)

#endif

//
// The following functions will help mask the differences between non-WinRT environments and
// WinRT: on the latter, a file path is typically not used to open files. Rather, a UI element is used
// to get a 'StorageFile' reference and you go from there. However, to test the library properly,
// we need to get a StorageFile reference somehow, and one way to do that is to create all the files
// used in testing in the Documents folder.
//
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4100) // Because of '_Prot' in WinRT builds.
#endif
template<typename _CharType>
pplx::task<streams::streambuf<_CharType>> OPEN_R(const utility::string_t& name, int _Prot = DEFAULT_PROT)
{
#if !defined(__cplusplus_winrt)
    return streams::file_buffer<_CharType>::open(name, std::ios_base::in, _Prot);
#else
    auto file =
        pplx::create_task(KnownFolders::DocumentsLibrary->GetFileAsync(ref new Platform::String(name.c_str()))).get();

    return streams::file_buffer<_CharType>::open(file, std::ios_base::in);
#endif
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
SUITE(istream_tests)
{
    // Tests using memory stream buffers.
    TEST(stream_read_1)
    {
        producer_consumer_buffer<char> rbuf;

        VERIFY_ARE_EQUAL(26u, rbuf.putn_nocopy("abcdefghijklmnopqrstuvwxyz", 26).get());

        istream stream(rbuf);

        VERIFY_IS_FALSE(stream.can_seek());

        for (char c = 'a'; c <= 'z'; c++)
        {
            char ch = (char)stream.read().get();
            VERIFY_ARE_EQUAL(c, ch);
        }

        stream.close().get();
    }

    TEST(fstream_read_1)
    {
        utility::string_t fname = U("fstream_read_1.txt");
        fill_file(fname);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        VERIFY_IS_TRUE(stream.is_open());

        for (char c = 'a'; c <= 'z'; c++)
        {
            char ch = (char)stream.read().get();
            VERIFY_ARE_EQUAL(c, ch);
        }

        stream.close().get();
    }

    TEST(stream_read_1_fail)
    {
        producer_consumer_buffer<char> rbuf;

        VERIFY_ARE_EQUAL(26u, rbuf.putn_nocopy("abcdefghijklmnopqrstuvwxyz", 26).get());

        istream stream(rbuf);
        rbuf.close(std::ios_base::in).get();

        VERIFY_THROWS(stream.read().get(), std::runtime_error);
        // Closing again should not throw.
        stream.close().wait();
    }

    TEST(stream_read_2)
    {
        producer_consumer_buffer<char> rbuf;

        VERIFY_ARE_EQUAL(26u, rbuf.putn_nocopy("abcdefghijklmnopqrstuvwxyz", 26).get());

        istream stream(rbuf);

        uint8_t buffer[128];
        streams::rawptr_buffer<uint8_t> tbuf(buffer, 128);

        VERIFY_ARE_EQUAL(26u, stream.read(tbuf, 26).get());

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        rbuf.close(std::ios_base::out).get();

        VERIFY_ARE_EQUAL(0u, stream.read(tbuf, 26).get());

        stream.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(fstream_read_2)
    {
        utility::string_t fname = U("fstream_read_2.txt");
        fill_file(fname);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        char buffer[128];
        streams::rawptr_buffer<char> tbuf(buffer, 128);

        VERIFY_ARE_EQUAL(26u, stream.read(tbuf, 26).get());

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        VERIFY_ARE_EQUAL(0u, stream.read(tbuf, 26).get());

        stream.close().get();
    }

    TEST(stream_read_3)
    {
        producer_consumer_buffer<char> rbuf;

        // There's no newline int the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        rbuf.close(std::ios_base::out).get();

        istream stream(rbuf);

        uint8_t buffer[128];
        streams::rawptr_buffer<uint8_t> tbuf(buffer, 128);

        VERIFY_ARE_EQUAL(52u, stream.read(tbuf, sizeof(buffer)).get());

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }
        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'A', buffer[i + 26]);
        }

        stream.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(stream_read_3_fail)
    {
        producer_consumer_buffer<char> rbuf;

        // There's no newline int the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        rbuf.close(std::ios_base::out).get();

        istream stream(rbuf);

        uint8_t buffer[128];
        streams::rawptr_buffer<uint8_t> tbuf(buffer, 128);
        tbuf.close(std::ios_base::out).get();

        VERIFY_THROWS(stream.read(tbuf, sizeof(buffer)).get(), std::runtime_error);

        stream.close().get();
    }

    TEST(stream_read_4)
    {
        producer_consumer_buffer<char> rbuf;
        producer_consumer_buffer<uint8_t> trg;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> stream = rbuf;

        VERIFY_ARE_EQUAL(52u, stream.read_to_delim(trg, '\n').get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(52u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }
        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'A', buffer[i + 26]);
        }

        stream.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(fstream_read_4)
    {
        producer_consumer_buffer<uint8_t> trg;

        utility::string_t fname = U("fstream_read_4.txt");
        fill_file(fname, 2);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        VERIFY_ARE_EQUAL(52u, stream.read_to_delim(trg, '\n').get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(52u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }
        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i + 26]);
        }

        stream.close().get();
    }

    TEST(stream_read_4_fail)
    {
        producer_consumer_buffer<char> rbuf;
        producer_consumer_buffer<uint8_t> trg;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> stream = rbuf;

        trg.close(std::ios::out).get();

        VERIFY_THROWS(stream.read_to_delim(trg, '\n').get(), std::runtime_error);

        stream.close().get();
    }

    TEST(stream_read_5)
    {
        producer_consumer_buffer<char> rbuf;
        producer_consumer_buffer<uint8_t> trg;

        // There's one newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\n\nABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());

        istream stream(rbuf);

        VERIFY_IS_FALSE(stream.is_eof());
        VERIFY_ARE_EQUAL(26u, stream.read_to_delim(trg, '\n').get());
        VERIFY_IS_FALSE(stream.is_eof());
        VERIFY_ARE_EQUAL(0u, stream.read_to_delim(trg, '\n').get());
        VERIFY_IS_FALSE(stream.is_eof());
        VERIFY_ARE_EQUAL('A', (char)rbuf.getc().get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(26u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        stream.close().get();
    }

    TEST(fstream_read_5)
    {
        producer_consumer_buffer<uint8_t> trg;

        utility::string_t fname = U("fstream_read_5.txt");
        fill_file_with_lines(fname, "\n", 2);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        VERIFY_ARE_EQUAL(26u, stream.read_to_delim(trg, '\n').get());
        VERIFY_ARE_EQUAL('a', (char)stream.read().get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(26u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        stream.close().get();
    }

    TEST(stream_readline_1)
    {
        producer_consumer_buffer<char> rbuf;
        producer_consumer_buffer<uint8_t> trg;

        // There's one newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());

        istream stream(rbuf);

        VERIFY_ARE_EQUAL(26u, stream.read_line(trg).get());
        VERIFY_ARE_EQUAL('A', (char)rbuf.getc().get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(26u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        stream.close().get();
    }

    TEST(stream_readline_1_fail)
    {
        producer_consumer_buffer<char> rbuf;
        producer_consumer_buffer<uint8_t> trg;

        // There's one newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());

        istream stream(rbuf);

        trg.close(std::ios_base::out).get();

        VERIFY_THROWS(stream.read_line(trg).get(), std::runtime_error);

        stream.close().get();
    }

    TEST(stream_readline_2)
    {
        producer_consumer_buffer<char> rbuf;
        producer_consumer_buffer<uint8_t> trg;

        // There's one newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\r\n\r\nABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());

        istream stream(rbuf);

        VERIFY_IS_FALSE(stream.is_eof());
        VERIFY_ARE_EQUAL(26u, stream.read_line(trg).get());
        VERIFY_IS_FALSE(stream.is_eof());
        VERIFY_ARE_EQUAL(0u, stream.read_line(trg).get());
        VERIFY_IS_FALSE(stream.is_eof());
        VERIFY_ARE_EQUAL('A', (char)rbuf.getc().get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(26u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        stream.close().get();
    }

    TEST(fstream_readline_1)
    {
        producer_consumer_buffer<uint8_t> trg;

        utility::string_t fname = U("fstream_readline_1.txt");
        fill_file_with_lines(fname, "\n", 2);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        VERIFY_ARE_EQUAL(26u, stream.read_line(trg).get());
        VERIFY_ARE_EQUAL('a', (char)stream.read().get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(26u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        stream.close().get();
    }

    TEST(fstream_readline_2)
    {
        producer_consumer_buffer<uint8_t> trg;

        utility::string_t fname = U("fstream_readline_2.txt");
        fill_file_with_lines(fname, "\r\n", 2);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        VERIFY_ARE_EQUAL(26u, stream.read_line(trg).get());
        VERIFY_ARE_EQUAL('a', (char)stream.read().get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(26u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        stream.close().get();
    }

    TEST(stream_read_6)
    {
        producer_consumer_buffer<char> rbuf;
        producer_consumer_buffer<uint8_t> trg;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        rbuf.close(std::ios_base::out).get();

        istream stream(rbuf);

        VERIFY_ARE_EQUAL(52u, stream.read_to_delim(trg, '|').get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(52u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }
        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'A', buffer[i + 26]);
        }

        stream.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(stream_read_7)
    {
        producer_consumer_buffer<char> rbuf;
        producer_consumer_buffer<uint8_t> trg;

        // There's one delimiter in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz|ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());

        istream stream(rbuf);

        VERIFY_ARE_EQUAL(26u, stream.read_to_delim(trg, '|').get());
        VERIFY_ARE_EQUAL('A', (char)rbuf.getc().get());

        uint8_t buffer[128];
        VERIFY_ARE_EQUAL(26u, trg.in_avail());
        trg.getn(buffer, trg.in_avail()).get();

        for (int i = 0; i < 26; i++)
        {
            VERIFY_ARE_EQUAL((char)i + 'a', buffer[i]);
        }

        stream.close().get();
    }

    TEST(stream_read_to_end_1)
    {
        // Create a really large (200KB) stream and read into a stream buffer.
        // It should not take a long time to do this test.

        producer_consumer_buffer<char> rbuf;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        for (int i = 0; i < 4096; ++i)
        {
            VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        }

        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> stream = rbuf;

        streams::stringstreambuf sbuf;
        auto& target = sbuf.collection();

        VERIFY_ARE_EQUAL(len * 4096, stream.read_to_end(sbuf).get());
        VERIFY_ARE_EQUAL(len * 4096, target.size());

        stream.close().get();
        sbuf.close().get();
    }

    TEST(stream_read_to_end_1_fail)
    {
        producer_consumer_buffer<char> rbuf;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        for (int i = 0; i < 4096; ++i)
        {
            VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        }

        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> stream = rbuf;

        streams::stringstreambuf sbuf;
        sbuf.close(std::ios_base::out).get();

        VERIFY_THROWS(stream.read_to_end(sbuf).get(), std::runtime_error);

        stream.close().get();
        // This should not throw
        sbuf.close().wait();
    }

    TEST(fstream_read_to_end_1)
    {
        // Create a really large (100KB) stream and read into a stream buffer.
        // It should not take a long time to do this test.

        utility::string_t fname = U("fstream_read_to_end_1.txt");
        fill_file(fname, 4096);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        streams::stringstreambuf sbuf;
        VERIFY_IS_FALSE(stream.is_eof());
        auto& target = sbuf.collection();

        VERIFY_ARE_EQUAL(26 * 4096, stream.read_to_end(sbuf).get());
        VERIFY_ARE_EQUAL(26 * 4096, target.size());
        VERIFY_IS_TRUE(stream.is_eof());

        stream.close().get();
        sbuf.close().get();
    }

    TEST(fstream_read_to_end_2)
    {
        // Read a file to end with is_eof tests.
        utility::string_t fname = U("fstream_read_to_end_2.txt");
        fill_file(fname);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        streams::stringstreambuf sbuf;
        int c;
        while (c = stream.read().get(), !stream.is_eof())
            sbuf.putc(static_cast<char>(c)).get();
        auto& target = sbuf.collection();
        VERIFY_ARE_EQUAL(26, target.size());
        VERIFY_IS_TRUE(stream.is_eof());

        stream.close().get();
        sbuf.close().get();
    }

    TEST(fstream_read_to_end_3)
    {
        // Async Read a file to end with is_eof tests.
        utility::string_t fname = U("fstream_read_to_end_3.txt");
        fill_file(fname, 1);

        streams::basic_istream<char> stream = OPEN_R<char>(fname).get().create_istream();

        streams::stringstreambuf sbuf;
        // workaround VC10 's bug.
        auto lambda2 = [](int) { return true; };
        auto lambda1 = [sbuf, stream, lambda2](int val) mutable -> pplx::task<bool> {
            if (!stream.is_eof())
                return sbuf.putc(static_cast<char>(val)).then(lambda2);
            else
                return pplx::task_from_result(false);
        };
        pplx::details::_do_while([=]() -> pplx::task<bool> { return stream.read().then(lambda1); }).wait();

        auto& target = sbuf.collection();
        VERIFY_ARE_EQUAL(26, target.size());
        VERIFY_IS_TRUE(stream.is_eof());

        stream.close().get();
        sbuf.close().get();
    }

    TEST(stream_read_to_delim_flush)
    {
        producer_consumer_buffer<char> rbuf;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz|ABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> stream = rbuf;

        producer_consumer_buffer<char> sbuf;

        char chars[128];

        VERIFY_ARE_EQUAL(26u, stream.read_to_delim(sbuf, '|').get());
        // The read_to_delim() should have flushed, so we should be getting what's there,
        // less than we asked for.
        VERIFY_ARE_EQUAL(26u, sbuf.getn(chars, 100).get());

        stream.close().get();
        sbuf.close().get();
    }

    TEST(stream_read_line_flush)
    {
        producer_consumer_buffer<char> rbuf;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyz\nABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> stream = rbuf;

        producer_consumer_buffer<char> sbuf;

        char chars[128];

        VERIFY_ARE_EQUAL(26u, stream.read_line(sbuf).get());
        // The read_line() should have flushed, so we should be getting what's there,
        // less than we asked for.
        VERIFY_ARE_EQUAL(26u, sbuf.getn(chars, 100).get());

        stream.close().get();
        sbuf.close().get();
    }

    TEST(stream_read_to_end_flush)
    {
        producer_consumer_buffer<char> rbuf;
        streams::basic_istream<char> stream = rbuf;

        // There's no newline in the input.
        const char* text = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        size_t len = strlen(text);

        VERIFY_ARE_EQUAL(len, rbuf.putn_nocopy(text, len).get());

        rbuf.close(std::ios_base::out).get();

        producer_consumer_buffer<char> sbuf;

        char chars[128];

        VERIFY_ARE_EQUAL(len, stream.read_to_end(sbuf).get());
        // The read_to_end() should have flushed, so we should be getting what's there,
        // less than we asked for.
        VERIFY_ARE_EQUAL(len, sbuf.getn(chars, len * 2).get());

        stream.close().get();
        sbuf.close().get();
    }

    TEST(istream_extract_string)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = " abc defgsf ";

        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> is = rbuf;
        std::string str1 = is.extract<std::string>().get();
        std::string str2 = is.extract<std::string>().get();

        VERIFY_ARE_EQUAL(str1, "abc");
        VERIFY_ARE_EQUAL(str2, "defgsf");
    }
#ifdef _WIN32 // On Linux, this becomes the exact copy of istream_extract_string1, hence disabled
    TEST(istream_extract_wstring_1)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = " abc defgsf ";

        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> is = rbuf;
        utility::string_t str1 = is.extract<utility::string_t>().get();
        utility::string_t str2 = is.extract<utility::string_t>().get();

        VERIFY_ARE_EQUAL(str1, L"abc");
        VERIFY_ARE_EQUAL(str2, L"defgsf");
    }

    TEST(istream_extract_wstring_2) // On Linux, this becomes the exact copy of istream_extract_string2, hence disabled
    {
        producer_consumer_buffer<signed char> rbuf;
        const char* text = " abc defgsf ";

        size_t len = strlen(text);
        rbuf.putn_nocopy((const signed char*)text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> is = rbuf;
        utility::string_t str1 = is.extract<utility::string_t>().get();
        utility::string_t str2 = is.extract<utility::string_t>().get();

        VERIFY_ARE_EQUAL(str1, L"abc");
        VERIFY_ARE_EQUAL(str2, L"defgsf");
    }

    TEST(istream_extract_wstring_3)
    {
        producer_consumer_buffer<unsigned char> rbuf;
        const char* text = " abc defgsf ";

        size_t len = strlen(text);
        rbuf.putn_nocopy((const unsigned char*)text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> is = rbuf;
        utility::string_t str1 = is.extract<utility::string_t>().get();
        utility::string_t str2 = is.extract<utility::string_t>().get();

        VERIFY_ARE_EQUAL(str1, L"abc");
        VERIFY_ARE_EQUAL(str2, L"defgsf");
    }

#endif

    TEST(istream_extract_int64)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = "1024 -17134711";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<char> is = rbuf;
        int64_t i1 = is.extract<int64_t>().get();
        int64_t i2 = is.extract<int64_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, -17134711);
    }

    TEST(istream_extract_uint64)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = "1024 12000000000";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::istream is(rbuf);
        uint64_t i1 = is.extract<uint64_t>().get();
        uint64_t i2 = is.extract<uint64_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, (uint64_t)12000000000);
    }
#ifdef _WIN32
    TEST(istream_extract_int64w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L"1024 -17134711";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        int64_t i1 = is.extract<int64_t>().get();
        int64_t i2 = is.extract<int64_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, -17134711);
    }

    TEST(istream_extract_uint64w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L"1024 12000000000";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        uint64_t i1 = is.extract<uint64_t>().get();
        uint64_t i2 = is.extract<uint64_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, (uint64_t)12000000000);
    }
#endif

    TEST(istream_extract_int32)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = "1024 -17134711 12000000000";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::istream is(rbuf);
        int32_t i1 = is.extract<int32_t>().get();
        int32_t i2 = is.extract<int32_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, -17134711);
        VERIFY_THROWS(is.extract<int32_t>().get(), std::range_error);
    }

    TEST(istream_extract_uint32)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = "1024 3000000000 12000000000";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::istream is(rbuf);
        uint32_t i1 = is.extract<uint32_t>().get();
        uint32_t i2 = is.extract<uint32_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024u);
        VERIFY_ARE_EQUAL(i2, (uint32_t)3000000000);
        VERIFY_THROWS(is.extract<uint32_t>().get(), std::range_error);
    }
#ifdef _WIN32
    TEST(istream_extract_int32w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L"1024 -17134711 12000000000";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        int32_t i1 = is.extract<int32_t>().get();
        int32_t i2 = is.extract<int32_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, -17134711);
        VERIFY_THROWS(is.extract<int32_t>().get(), std::range_error);
    }

    TEST(istream_extract_uint32w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L"1024 3000000000 12000000000";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        uint32_t i1 = is.extract<uint32_t>().get();
        uint32_t i2 = is.extract<uint32_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024u);
        VERIFY_ARE_EQUAL(i2, 3000000000u);
        VERIFY_THROWS(is.extract<uint32_t>().get(), std::range_error);
    }
#endif

    TEST(istream_extract_int16)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = "1024 -4711 100000";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::istream is(rbuf);
        int16_t i1 = is.extract<int16_t>().get();
        int16_t i2 = is.extract<int16_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, -4711);
        VERIFY_THROWS(is.extract<int16_t>().get(), std::range_error);
    }

    TEST(istream_extract_uint16)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = "1024 50000 100000";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::istream is(rbuf);
        uint16_t i1 = is.extract<uint16_t>().get();
        uint16_t i2 = is.extract<uint16_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, 50000);
        VERIFY_THROWS(is.extract<uint16_t>().get(), std::range_error);
    }

#ifdef _WIN32
    TEST(istream_extract_int16w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L"1024 -4711 100000";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        int16_t i1 = is.extract<int16_t>().get();
        int16_t i2 = is.extract<int16_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, -4711);
        VERIFY_THROWS(is.extract<int16_t>().get(), std::range_error);
    }

    TEST(istream_extract_uint16w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L"1024 50000 100000";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        uint16_t i1 = is.extract<uint16_t>().get();
        uint16_t i2 = is.extract<uint16_t>().get();

        VERIFY_ARE_EQUAL(i1, 1024);
        VERIFY_ARE_EQUAL(i2, 50000);
        VERIFY_THROWS(is.extract<uint16_t>().get(), std::range_error);
    }
#endif

    TEST(istream_extract_int8)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = "0 -125 512";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<unsigned char> is(rbuf);
        int8_t i1 = is.extract<int8_t>().get();
        int8_t i2 = is.extract<int8_t>().get();

        VERIFY_ARE_EQUAL(i1, '0');
        VERIFY_ARE_EQUAL(i2, '-');
    }

    TEST(istream_extract_uint8)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = "0 150 512";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::basic_istream<unsigned char> is(rbuf);
        uint8_t i1 = is.extract<uint8_t>().get();
        uint8_t i2 = is.extract<uint8_t>().get();

        VERIFY_ARE_EQUAL(i1, '0');
        VERIFY_ARE_EQUAL(i2, '1');
    }

#ifdef _WIN32
    TEST(istream_extract_int8w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L"0 -125 512";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        int8_t i1 = is.extract<int8_t>().get();
        int8_t i2 = is.extract<int8_t>().get();

        VERIFY_ARE_EQUAL(i1, '0');
        VERIFY_ARE_EQUAL(i2, '-');
    }

    TEST(istream_extract_uint8w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L"0 150 512";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        uint8_t i1 = is.extract<uint8_t>().get();
        uint8_t i2 = is.extract<uint8_t>().get();

        VERIFY_ARE_EQUAL(i1, '0');
        VERIFY_ARE_EQUAL(i2, '1');
    }
#endif

    TEST(istream_extract_bool)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = " true false NOT_OK";
        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::istream is(rbuf);
        bool i1 = is.extract<bool>().get();
        bool i2 = is.extract<bool>().get();

        VERIFY_IS_TRUE(i1);
        VERIFY_IS_FALSE(i2);
        VERIFY_THROWS(is.extract<bool>().get(), std::runtime_error);
    }

    TEST(istream_extract_bool_from_number)
    {
        producer_consumer_buffer<char> rbuf;
        const char* text = " 1 0 NOT_OK";

        size_t len = strlen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::istream is(rbuf);
        bool i1 = is.extract<bool>().get();
        bool i2 = is.extract<bool>().get();

        VERIFY_IS_TRUE(i1);
        VERIFY_IS_FALSE(i2);
        // Make sure parsing consumes just the right amount of characters.
        VERIFY_ARE_EQUAL(7u, rbuf.in_avail());
        VERIFY_THROWS(is.extract<bool>().get(), std::runtime_error);
    }

#ifdef _WIN32
    TEST(istream_extract_bool_w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L" true false NOT_OK";
        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        bool i1 = is.extract<bool>().get();
        bool i2 = is.extract<bool>().get();

        VERIFY_IS_TRUE(i1);
        VERIFY_IS_FALSE(i2);
        VERIFY_THROWS(is.extract<bool>().get(), std::runtime_error);
    }

    TEST(istream_extract_bool_from_number_w)
    {
        producer_consumer_buffer<wchar_t> rbuf;
        const wchar_t* text = L" 1 0 NOT_OK";

        size_t len = wcslen(text);
        rbuf.putn_nocopy(text, len).wait();
        rbuf.close(std::ios_base::out).get();

        streams::wistream is(rbuf);
        bool i1 = is.extract<bool>().get();
        bool i2 = is.extract<bool>().get();

        VERIFY_IS_TRUE(i1);
        VERIFY_IS_FALSE(i2);
        // Make sure parsing consumes just the right amount of characters.
        VERIFY_ARE_EQUAL(7u, rbuf.in_avail());
        VERIFY_THROWS(is.extract<bool>().get(), std::runtime_error);
    }

#endif

    template<typename _CharType, typename _LongType>
    void istream_extract_long_impl(streambuf<_CharType> buf)
    {
        basic_istream<_CharType> is(buf);
        const _LongType v1 = is.template extract<_LongType>().get();
        const _LongType v2 = is.template extract<_LongType>().get();

        VERIFY_ARE_EQUAL(123, v1);
        VERIFY_ARE_EQUAL(-567, v2);
        VERIFY_THROWS(is.template extract<_LongType>().get(), std::runtime_error);
    }

    TEST(istream_extract_long)
    {
        istream_extract_long_impl<char, long>(
            container_buffer<std::string>("123 -567 120000000000000000000000000000000000000000000000"));
#ifdef _WIN32
        istream_extract_long_impl<wchar_t, long>(container_buffer<std::wstring>(L"123 -567 12000000000"));
#endif
    }

    template<typename _CharType, typename _LongType>
    void istream_extract_unsigned_long_impl(streambuf<_CharType> buf)
    {
        basic_istream<_CharType> is(buf);
        const _LongType v1 = is.template extract<_LongType>().get();
        const _LongType v2 = is.template extract<_LongType>().get();

        VERIFY_ARE_EQUAL(876, v1);
        VERIFY_ARE_EQUAL(3, v2);
        VERIFY_THROWS(is.template extract<_LongType>().get(), std::runtime_error);
    }

    TEST(istream_extract_unsigned_long)
    {
        istream_extract_unsigned_long_impl<char, unsigned long>(container_buffer<std::string>("876 3 -44"));
#ifdef _WIN32
        istream_extract_unsigned_long_impl<wchar_t, unsigned long>(container_buffer<std::wstring>(L"876 3 -44"));
#endif
    }

    TEST(istream_extract_long_long)
    {
        istream_extract_long_impl<char, long long>(container_buffer<std::string>("123 -567 92233720368547758078"));
#ifdef _WIN32
        istream_extract_long_impl<wchar_t, long long>(container_buffer<std::wstring>(L"123 -567 92233720368547758078"));
#endif
    }

    TEST(istream_extract_unsigned_long_long)
    {
        istream_extract_unsigned_long_impl<char, unsigned long long>(container_buffer<std::string>("876 3 -44"));
#ifdef _WIN32
        istream_extract_unsigned_long_impl<wchar_t, unsigned long long>(container_buffer<std::wstring>(L"876 3 -44"));
#endif
    }

    template<typename T>
    void compare_floating(T expected, T actual, T relativeDiff)
    {
        // http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
        if (expected != actual)
        {
            const auto diff = fabs(expected - actual);
            const auto absExpected = fabs(expected);
            const auto absActual = fabs(actual);
            const auto largest = absExpected > absActual ? absExpected : absActual;
            if (diff > largest * relativeDiff)
            {
                VERIFY_IS_TRUE(false);
            }
        }
    }
    void compare_double(double expected, double actual) { compare_floating(expected, actual, DBL_EPSILON); }
    void compare_float(float expected, float actual) { compare_floating(expected, actual, FLT_EPSILON); }

    TEST(extract_floating_point)
    {
        std::string test_string;
        test_string.append(" 9.81E05 3.14");
        test_string.append(" 2.71.5"); // two numbers merged after comma
        test_string.append(" 6E+4.5"); // two numbers merged in exponent
        test_string.append(" 6E-4.5"); // two numbers merged in exponent
        test_string.append(" 3.14 -10 +42.0 -1234.567 .01 +0 -0");
#ifndef __APPLE__
        test_string.append(" 12345678901234567890123456789012345678901234567890"); // a big number
#endif
        test_string.append(" 9.81E05 6.0221413e+23 1.6e-14"); // numbers with exponent
        test_string.append(" 6.");                            // a number ending with a dot

        std::stringstream std_istream;
        std_istream << test_string;

        producer_consumer_buffer<uint8_t> buff, bufd;
        auto ostream_float = buff.create_ostream();
        auto istream_float = buff.create_istream();
        auto ostream_double = bufd.create_ostream();
        auto istream_double = bufd.create_istream();

        ostream_float.print(test_string);
        ostream_double.print(test_string);
        ostream_float.close().wait();
        ostream_double.close().wait();

        do
        {
            double expected = 0;
            std_istream >> expected;

            const auto actual = istream_double.extract<double>().get();
            compare_double(expected, actual);

            if (actual <= (std::numeric_limits<float>::max)())
                compare_float(float(expected), istream_float.extract<float>().get());
            else
                VERIFY_THROWS(istream_float.extract<float>().get(), std::exception);

            // Checking positive and negative zero's by dividing 1 with it. They should result in positive and negative
            // infinity.
            if (expected == 0) VERIFY_ARE_EQUAL(1 / expected, 1 / actual);
        } while (!std_istream.eof());
    }

    TEST(extract_floating_point_with_exceptions)
    {
        std::vector<std::pair<std::string, std::string>> tests;
        tests.push_back(std::pair<std::string, std::string>("a", "Invalid character 'a'"));
        tests.push_back(std::pair<std::string, std::string>("x", "Invalid character 'x'"));
        tests.push_back(std::pair<std::string, std::string>("e", "Invalid character 'e'"));
        tests.push_back(std::pair<std::string, std::string>("E", "Invalid character 'E'"));
        tests.push_back(std::pair<std::string, std::string>("6.022e+t", "Invalid character 't' in exponent"));
        tests.push_back(std::pair<std::string, std::string>("9.81e-.", "Invalid character '.' in exponent"));
        tests.push_back(std::pair<std::string, std::string>("9.81e-", "Incomplete exponent"));
        tests.push_back(std::pair<std::string, std::string>("1.2e+", "Incomplete exponent"));
        tests.push_back(std::pair<std::string, std::string>("10E+-23", "The exponent sign already set"));
        tests.push_back(std::pair<std::string, std::string>("15E-+45", "The exponent sign already set"));
        tests.push_back(std::pair<std::string, std::string>("5.34e", "Incomplete exponent"));
        tests.push_back(std::pair<std::string, std::string>("2E+308", "The value is too big"));
        tests.push_back(std::pair<std::string, std::string>("-2E+308", "The value is too big"));
        tests.push_back(std::pair<std::string, std::string>("1E-324", "The value is too small"));
        tests.push_back(std::pair<std::string, std::string>("-1E-324", "The value is too small"));

        for (auto iter = tests.begin(); iter != tests.end(); iter++)
        {
            std::stringstream std_istream;
            std_istream << iter->first;
            VERIFY_IS_TRUE(std_istream.good());
            double x;
            std_istream >> x;
            VERIFY_IS_FALSE(std_istream.good());

            producer_consumer_buffer<uint8_t> buf;
            auto stream = buf.create_ostream();
            auto istream_double = buf.create_istream();

            stream.print(iter->first);
            stream.close().wait();

            try
            {
                istream_double.extract<double>().get();
                VERIFY_IS_TRUE(false, "No exception has been thrown");
            }
            catch (const std::exception& exc)
            {
                VERIFY_ARE_EQUAL(std::string(exc.what()), iter->second);
            }
            catch (...)
            {
                VERIFY_IS_TRUE(false, "A wrong exception has been thrown");
            }
        }
    }

    TEST(streambuf_read_delim)
    {
        producer_consumer_buffer<uint8_t> rbuf;
        std::string s("Hello  World"); // there are 2 spaces here

        streams::stringstreambuf data;

        streams::istream is(rbuf);

        auto t = is.read_to_delim(data, ' ')
                     .then([&data, is](size_t size) -> pplx::task<size_t> {
                         std::string r("Hello");
                         VERIFY_ARE_EQUAL(size, r.size());
                         VERIFY_IS_FALSE(is.is_eof());

                         auto& s2 = data.collection();
                         VERIFY_ARE_EQUAL(s2, r);
                         return is.read_to_delim(data, ' ');
                     })
                     .then([&data, is](size_t size) -> pplx::task<size_t> {
                         VERIFY_ARE_EQUAL(size, 0);
                         VERIFY_IS_FALSE(is.is_eof());
                         return is.read_to_delim(data, ' ');
                     })
                     .then([&data, is](size_t size) -> void {
                         VERIFY_ARE_EQUAL(size, 5);
                         VERIFY_IS_TRUE(is.is_eof());
                     });
        rbuf.putn_nocopy((uint8_t*)s.data(), s.size()).wait();
        rbuf.close(std::ios_base::out).get();
        t.wait();
    }

    TEST(uninitialized_stream)
    {
        streams::basic_ostream<uint8_t> test_ostream;
        streams::basic_istream<uint8_t> test_istream;

        VERIFY_IS_FALSE(test_ostream.is_valid());
        VERIFY_IS_FALSE(test_istream.is_valid());

        VERIFY_THROWS(test_istream.read(), std::logic_error);
        VERIFY_THROWS(test_ostream.flush(), std::logic_error);

        test_istream.close().wait();
        test_ostream.close().wait();
    }

    TEST(uninitialized_streambuf)
    {
        streams::streambuf<uint8_t> strbuf;

        // The bool operator shall not throw
        VERIFY_IS_TRUE(!strbuf);

        // All operations should throw
        uint8_t* ptr = nullptr;
        size_t count = 0;

        VERIFY_THROWS(strbuf.acquire(ptr, count), std::invalid_argument);
        VERIFY_THROWS(strbuf.release(ptr, count), std::invalid_argument);

        VERIFY_THROWS(strbuf.alloc(count), std::invalid_argument);
        VERIFY_THROWS(strbuf.commit(count), std::invalid_argument);

        VERIFY_THROWS(strbuf.can_read(), std::invalid_argument);
        VERIFY_THROWS(strbuf.can_write(), std::invalid_argument);
        VERIFY_THROWS(strbuf.can_seek(), std::invalid_argument);

        VERIFY_THROWS(strbuf.is_eof(), std::invalid_argument);
        VERIFY_THROWS(strbuf.is_open(), std::invalid_argument);

        VERIFY_THROWS(strbuf.in_avail(), std::invalid_argument);
        VERIFY_THROWS(strbuf.get_base(), std::invalid_argument);

        VERIFY_THROWS(strbuf.putc('a').get(), std::invalid_argument);
        VERIFY_THROWS(strbuf.putn_nocopy(ptr, count).get(), std::invalid_argument);
        VERIFY_THROWS(strbuf.sync().get(), std::invalid_argument);

        VERIFY_THROWS(strbuf.getc().get(), std::invalid_argument);
        VERIFY_THROWS(strbuf.ungetc().get(), std::invalid_argument);
        VERIFY_THROWS(strbuf.bumpc().get(), std::invalid_argument);
        VERIFY_THROWS(strbuf.nextc().get(), std::invalid_argument);
        VERIFY_THROWS(strbuf.getn(ptr, count).get(), std::invalid_argument);

        VERIFY_THROWS(strbuf.close().get(), std::invalid_argument);

        // The destructor shall not throw
    }

    TEST(create_istream_from_output_only)
    {
        container_buffer<std::string> sourceBuf;
        VERIFY_THROWS(sourceBuf.create_istream(), std::runtime_error);
    }

    TEST(extract_close_with_exception)
    {
        container_buffer<std::string> sourceBuf(std::ios::in);
        auto inStream = sourceBuf.create_istream();
        inStream.close(std::make_exception_ptr(std::invalid_argument("test exception"))).wait();
        auto extractTask = inStream.extract<std::string>();
        VERIFY_THROWS(extractTask.get(), std::invalid_argument);
    }

    TEST(streambuf_close_with_exception_read)
    {
        container_buffer<std::string> sourceBuf("test data string");
        sourceBuf.close(std::ios::in, std::make_exception_ptr(std::invalid_argument("custom exception")));

        const size_t size = 4;
        char targetBuf[size];
        auto t = sourceBuf.getn(targetBuf, size);
        VERIFY_THROWS(t.get(), std::invalid_argument);
    }

    TEST(stream_close_with_exception_read)
    {
        container_buffer<std::string> sourceBuf("test data string");
        auto inStream = sourceBuf.create_istream();
        inStream.close(std::make_exception_ptr(std::invalid_argument("custom exception")));

        container_buffer<std::string> targetBuf;
        auto t1 = inStream.read(targetBuf, 4);
        VERIFY_THROWS(t1.get(), std::invalid_argument);
        VERIFY_THROWS(inStream.streambuf().sbumpc(), std::invalid_argument);
        VERIFY_THROWS(inStream.streambuf().sgetc(), std::invalid_argument);
    }

    TEST(istream_input_after_close)
    {
        container_buffer<std::string> sourceBuf("test data");
        auto inStream = sourceBuf.create_istream();
        inStream.close().wait();

        container_buffer<std::string> targetBuf;
        VERIFY_THROWS(inStream.peek().get(), std::runtime_error);
        VERIFY_THROWS(inStream.read(targetBuf, 1).get(), std::runtime_error);
        VERIFY_THROWS(inStream.read_line(targetBuf).get(), std::runtime_error);
        VERIFY_THROWS(inStream.read_to_delim(targetBuf, '-').get(), std::runtime_error);
        VERIFY_THROWS(inStream.read_to_end(targetBuf).get(), std::runtime_error);
        VERIFY_THROWS(inStream.seek(0), std::runtime_error);
        VERIFY_THROWS(inStream.seek(1, std::ios::beg), std::runtime_error);
        VERIFY_THROWS(inStream.tell(), std::runtime_error);
        VERIFY_THROWS(inStream.extract<std::string>().get(), std::runtime_error);
    }

    TEST(extract_from_empty_stream)
    {
        container_buffer<std::string> sourceBuf(std::ios::in);
        auto inStream = sourceBuf.create_istream();

        VERIFY_THROWS(inStream.extract<int64_t>().get(), std::range_error);
        VERIFY_THROWS(inStream.extract<char>().get(), std::runtime_error);
        VERIFY_THROWS(inStream.extract<unsigned char>().get(), std::runtime_error);
        VERIFY_THROWS(inStream.extract<signed char>().get(), std::runtime_error);
        VERIFY_THROWS(inStream.extract<bool>().get(), std::runtime_error);

        const std::string strValue = inStream.extract<std::string>().get();
        VERIFY_ARE_EQUAL("", strValue);
#ifdef _WIN32
        const std::wstring wstrValue = inStream.extract<std::wstring>().get();
        VERIFY_ARE_EQUAL(L"", wstrValue);
#endif
    }

    TEST(seek_after_eof)
    {
        container_buffer<std::string> sourceBuf(std::ios::in);
        VERIFY_ARE_EQUAL(basic_istream<char>::traits::eof(), sourceBuf.seekoff(1, std::ios::cur, std::ios::in));
    }

} // SUITE(istream_tests)

} // namespace streams
} // namespace functional
} // namespace tests
