/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests for async file stream buffer operations.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#ifdef _WIN32
#include "CppSparseFile.h"
#endif

#if defined(__cplusplus_winrt)
using namespace Windows::Storage;
#endif

#ifdef _WIN32
#define DEFAULT_PROT (int)std::ios_base::_Openprot
#else
#define DEFAULT_PROT 0
#define _SH_DENYRW 0x20
#endif

namespace tests
{
namespace functional
{
namespace streams
{
using namespace utility;
using namespace ::pplx;

// Used to prepare data for read tests

utility::string_t get_full_name(const utility::string_t& name);

void fill_file(const utility::string_t& name, size_t repetitions = 1);
#ifdef _WIN32
void fill_file_w(const utility::string_t& name, size_t repetitions = 1);
#endif

//
// The following two functions will help mask the differences between non-WinRT environments and
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
pplx::task<concurrency::streams::streambuf<_CharType>> OPEN(const utility::string_t& name,
                                                            std::ios::ios_base::openmode mode,
                                                            int _Prot = DEFAULT_PROT)
{
#if !defined(__cplusplus_winrt)
    return concurrency::streams::file_buffer<_CharType>::open(name, mode, _Prot);
#else
    try
    {
        if ((mode & std::ios::out))
        {
            auto file =
                pplx::create_task(KnownFolders::DocumentsLibrary->CreateFileAsync(
                                      ref new Platform::String(name.c_str()), CreationCollisionOption::ReplaceExisting))
                    .get();

            return concurrency::streams::file_buffer<_CharType>::open(file, mode);
        }
        else
        {
            auto file =
                pplx::create_task(KnownFolders::DocumentsLibrary->GetFileAsync(ref new Platform::String(name.c_str())))
                    .get();

            return concurrency::streams::file_buffer<_CharType>::open(file, mode);
        }
    }
    catch (Platform::Exception ^ exc)
    {
        // The create_system_error API expects a WIN32 error code NOT an HRESULT.
        if (exc->HResult == 0x80070002)
        {
            throw utility::details::create_system_error(ERROR_FILE_NOT_FOUND);
        }
        else
        {
            // Some other unexpected error code was encountered, fail immediately.
            // Throw statement is still included after because compiler warns about not
            // all paths returning a value.
            VERIFY_IS_TRUE(false);
            throw utility::details::create_system_error(exc->HResult);
        }
    }
#endif
}

template<typename _CharType>
pplx::task<concurrency::streams::streambuf<_CharType>> OPEN_W(const utility::string_t& name, int _Prot = DEFAULT_PROT)
{
    return OPEN<_CharType>(name, std::ios_base::out | std::ios_base::trunc, _Prot);
}

template<typename _CharType>
pplx::task<concurrency::streams::streambuf<_CharType>> OPEN_R(const utility::string_t& name, int _Prot = DEFAULT_PROT)
{
    return OPEN<_CharType>(name, std::ios_base::in, _Prot);
}

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

SUITE(file_buffer_tests)
{
    TEST(OpenCloseTest1)
    {
        // Test using single-byte strings
        auto open = OPEN_W<char>(U("OpenCloseTest1.txt"));

        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());
        VERIFY_IS_TRUE(stream.is_open());

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(OpenForReadDoesntCreateFile1)
    {
        utility::string_t fname = U("OpenForReadDoesntCreateFile1.txt");

        VERIFY_THROWS_SYSTEM_ERROR(OPEN_R<char>(fname).get(), std::errc::no_such_file_or_directory);

        std::ifstream is;
        VERIFY_IS_NULL(is.rdbuf()->open(fname.c_str(), std::ios::in));
    }

    TEST(OpenForReadDoesntCreateFile2)
    {
        utility::string_t fname = U("OpenForReadDoesntCreateFile2.txt");

        VERIFY_THROWS_SYSTEM_ERROR(OPEN<char>(fname, std::ios_base::in | std::ios_base::binary).get(),
                                   std::errc::no_such_file_or_directory);

        std::ifstream is;
        VERIFY_IS_NULL(is.rdbuf()->open(fname.c_str(), std::ios::in | std::ios_base::binary));
    }

    TEST(WriteSingleCharTest1)
    {
        auto open = OPEN_W<char>(U("WriteSingleCharTest1.txt"));
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());
        VERIFY_IS_TRUE(stream.is_open());

        bool elements_equal = true;
        for (uint8_t ch = 'a'; ch <= 'z'; ch++)
        {
            elements_equal = elements_equal && (ch == stream.putc(ch).get());
        }

        VERIFY_IS_TRUE(elements_equal);

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
        VERIFY_IS_FALSE(stream.is_open());
    }
#ifdef _WIN32
    TEST(WriteSingleCharTest1w)
    {
        auto open = OPEN_W<wchar_t>(U("WriteSingleCharTest1w.txt"));
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());
        VERIFY_IS_TRUE(stream.is_open());

        bool elements_equal = true;
        for (wchar_t ch = L'a'; ch <= L'z'; ch++)
        {
            elements_equal = elements_equal && (ch == stream.putc(ch).get());
        }

        VERIFY_IS_TRUE(elements_equal);

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
        VERIFY_IS_FALSE(stream.is_open());
    }
#endif

    TEST(WriteBufferTest1)
    {
        auto open = OPEN_W<char>(U("WriteBufferTest1.txt"));
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());
        VERIFY_IS_TRUE(stream.is_open());

        std::vector<char> vect;

        for (uint8_t ch = 'a'; ch <= 'z'; ch++)
        {
            vect.push_back(ch);
        }

        VERIFY_ARE_EQUAL(stream.putn_nocopy(&vect[0], vect.size()).get(), vect.size());

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
        VERIFY_IS_FALSE(stream.is_open());
    }
#ifdef _WIN32
    TEST(WriteBufferTest1w)
    {
        auto open = OPEN_W<wchar_t>(U("WriteBufferTest1w.txt"));
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());
        VERIFY_IS_TRUE(stream.is_open());

        std::vector<wchar_t> vect;

        for (wchar_t ch = L'a'; ch <= L'z'; ch++)
        {
            vect.push_back(ch);
        }

        VERIFY_ARE_EQUAL(stream.putn_nocopy(&vect[0], vect.size()).get(), vect.size());

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
        VERIFY_IS_FALSE(stream.is_open());
    }
#endif

    TEST(WriteBufferAndSyncTest1)
    {
        auto open = OPEN_W<char>(U("WriteBufferAndSyncTest1.txt"));
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());
        VERIFY_IS_TRUE(stream.is_open());

        std::vector<char> vect;

        for (uint8_t ch = 'a'; ch <= 'z'; ch++)
        {
            vect.push_back(ch);
        }

        auto write = stream.putn_nocopy(&vect[0], vect.size());

        stream.sync().get();

        VERIFY_ARE_EQUAL(write.get(), vect.size());
        VERIFY_IS_TRUE(write.is_done());

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(ReadSingleChar_bumpc1)
    {
        utility::string_t fname = U("ReadSingleChar_bumpc1.txt");
        fill_file(fname);

        auto stream = OPEN_R<char>(fname).get();

        VERIFY_IS_TRUE(stream.is_open());

        uint8_t buf[10];
        memset(buf, 0, sizeof(buf));

        for (int i = 0; i < sizeof(buf); i++)
        {
            buf[i] = (uint8_t)stream.bumpc().get();
            VERIFY_ARE_EQUAL(buf[i], 'a' + i);
        }

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(SequentialReadWrite)
    {
        utility::string_t fname = U("SequentialReadWrite.txt");

        auto ostreamBuf = OPEN_W<char>(fname).get();

        VERIFY_IS_TRUE(ostreamBuf.is_open());

        for (int i = 0; i < 1000; i++)
        {
            ostreamBuf.putc(i % 26 + 'a');
            ostreamBuf.putn_nocopy("ABCDEFGHIJ", 10);
        }
        ostreamBuf.close().wait();
        VERIFY_IS_FALSE(ostreamBuf.is_open());

        auto istreamBuf = OPEN_R<char>(fname).get();
        std::vector<pplx::task<void>> t;

        for (int k = 0; k < 2; k++)
        {
            for (int i = 0; i < 1000; i++)
            {
                t.push_back(istreamBuf.getc().then([i, this](char c) { VERIFY_ARE_EQUAL(i % 26 + 'a', c); }));
                t.push_back(istreamBuf.bumpc().then([i, this](char c) { VERIFY_ARE_EQUAL(i % 26 + 'a', c); }));
                char* buffer = new char[11];
                t.push_back(istreamBuf.getn(buffer, 10).then([=](size_t n) {
                    VERIFY_ARE_EQUAL(10u, n);
                    VERIFY_ARE_EQUAL(std::string("ABCDEFGHIJ"), std::string(buffer, 10));
                    delete[] buffer;
                }));
            }
            istreamBuf.seekpos(0, std::ios::in);
        }
        istreamBuf.close().wait();
        VERIFY_IS_FALSE(istreamBuf.is_open());
        for (size_t i = 0; i < t.size(); i++)
            t[i].wait();
    }

#ifdef _WIN32
    TEST(ReadSingleChar_bumpcw)
    {
        utility::string_t fname = U("ReadSingleChar_bumpcw.txt");
        fill_file_w(fname);

        auto stream = OPEN_R<wchar_t>(fname).get();

        VERIFY_IS_TRUE(stream.is_open());

        wchar_t buf[10];
        memset(buf, 0, sizeof(buf));

        for (int i = 0; i < 10; i++)
        {
            buf[i] = stream.bumpc().get();
            VERIFY_ARE_EQUAL(buf[i], L'a' + i);
        }

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }
#endif

    TEST(ReadSingleChar_bumpc2)
    {
        // Test that seeking works.
        utility::string_t fname = U("ReadSingleChar_bumpc2.txt");
        fill_file(fname);

        auto stream = OPEN_R<char>(fname).get();

        VERIFY_IS_TRUE(stream.is_open());

        stream.seekpos(3, std::ios_base::in);

        uint8_t buf[10];
        memset(buf, 0, sizeof(buf));

        for (int i = 0; i < sizeof(buf); i++)
        {
            buf[i] = (uint8_t)stream.bumpc().get();
            VERIFY_ARE_EQUAL(buf[i], 'd' + i);
        }

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(filestream_length)
    {
        utility::string_t fname = U("ReadSingleChar_bumpc3.txt");
        fill_file(fname);

        auto stream = OPEN_R<char>(fname, _SH_DENYRW).get();
        stream.set_buffer_size(512);

        VERIFY_IS_TRUE(stream.is_open());

        test_stream_length(stream.create_istream(), 26);

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(ReadSingleChar_bumpc3)
    {
        // Test that seeking works.
        utility::string_t fname = U("ReadSingleChar_bumpc3.txt");
        fill_file(fname);

        auto stream = OPEN_R<char>(fname, _SH_DENYRW).get();
        stream.set_buffer_size(512);

        VERIFY_IS_TRUE(stream.is_open());

        stream.seekpos(2, std::ios_base::in);

        // Read a character asynchronously to get the buffer primed.
        stream.bumpc().get();

        auto ras = concurrency::streams::char_traits<char>::requires_async();

        for (int i = 3; i < 26; i++)
        {
            auto c = (uint8_t)stream.sbumpc();
            if (c != ras)
            {
                VERIFY_ARE_EQUAL(c, 'a' + i);
            }
        }

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(ReadSingleChar_nextc)
    {
        utility::string_t fname = U("ReadSingleChar_nextc.txt");
        fill_file(fname);

        auto stream = OPEN_R<char>(fname).get();

        VERIFY_IS_TRUE(stream.is_open());

        uint8_t buf[10];
        memset(buf, 0, sizeof(buf));

        for (int i = 0; i < sizeof(buf); i++)
        {
            buf[i] = (uint8_t)stream.nextc().get();
            VERIFY_ARE_EQUAL(buf[i], 'b' + i);
        }

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }
#ifdef _WIN32
    TEST(ReadSingleChar_nextcw)
    {
        utility::string_t fname = U("ReadSingleChar_nextcw.txt");
        fill_file_w(fname);

        auto stream = OPEN_R<wchar_t>(fname).get();

        VERIFY_IS_TRUE(stream.is_open());

        wchar_t buf[10];
        memset(buf, 0, sizeof(buf));

        for (int i = 0; i < 10; i++)
        {
            buf[i] = stream.nextc().get();
            VERIFY_ARE_EQUAL(buf[i], L'b' + i);
        }

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }
#endif

    TEST(ReadSingleChar_ungetc)
    {
        // Test that seeking works.
        utility::string_t fname = U("ReadSingleChar_ungetc.txt");
        fill_file(fname);

        auto stream = OPEN_R<char>(fname).get();

        VERIFY_IS_TRUE(stream.is_open());

        stream.seekpos(13, std::ios_base::in);

        uint8_t buf[10];
        memset(buf, 0, sizeof(buf));

        for (int i = 0; i < sizeof(buf); i++)
        {
            buf[i] = (uint8_t)stream.ungetc().get();
            VERIFY_ARE_EQUAL(buf[i], 'm' - i);
        }

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(ReadSingleChar_getc1)
    {
        utility::string_t fname = U("ReadSingleChar_getc1.txt");
        fill_file(fname);

        auto stream = OPEN_R<char>(fname, _SH_DENYRW).get();

        VERIFY_IS_TRUE(stream.is_open());

        uint8_t ch0 = (uint8_t)stream.getc().get();
        uint8_t ch1 = (uint8_t)stream.getc().get();

        VERIFY_ARE_EQUAL(ch0, ch1);

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(ReadSingleChar_getc2)
    {
        utility::string_t fname = U("ReadSingleChar_getc2.txt");
        fill_file(fname);

        auto stream = OPEN_R<char>(fname, _SH_DENYRW).get();

        VERIFY_IS_TRUE(stream.is_open());

        stream.seekpos(13, std::ios_base::in);

        uint8_t ch0 = (uint8_t)stream.getc().get();
        uint8_t ch1 = (uint8_t)stream.sgetc();

        VERIFY_ARE_EQUAL(ch0, ch1);

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

#ifdef _WIN32
    TEST(ReadSingleChar_getc1w)
    {
        utility::string_t fname = U("ReadSingleChar_getc1w.txt");
        fill_file_w(fname);

        auto stream = OPEN_R<wchar_t>(fname, _SH_DENYRW).get();

        VERIFY_IS_TRUE(stream.is_open());

        wchar_t ch0 = stream.getc().get();
        wchar_t ch1 = stream.getc().get();

        VERIFY_ARE_EQUAL(ch0, ch1);
        VERIFY_ARE_EQUAL(ch0, L'a');

        stream.seekpos(15, std::ios_base::in);

        ch0 = stream.getc().get();
        ch1 = stream.getc().get();

        VERIFY_ARE_EQUAL(ch0, ch1);
        VERIFY_ARE_EQUAL(ch0, L'p');

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(ReadSingleChar_getc2w)
    {
        utility::string_t fname = U("ReadSingleChar_getc2w.txt");
        fill_file_w(fname);

        auto stream = OPEN_R<wchar_t>(fname, _SH_DENYRW).get();

        VERIFY_IS_TRUE(stream.is_open());

        stream.seekpos(13, std::ios_base::in);

        wchar_t ch0 = stream.getc().get();
        wchar_t ch1 = stream.getc().get();

        VERIFY_ARE_EQUAL(ch0, ch1);
        VERIFY_ARE_EQUAL(ch0, L'n');

        stream.seekpos(5, std::ios_base::in);

        ch0 = stream.getc().get();
        ch1 = stream.getc().get();

        VERIFY_ARE_EQUAL(ch0, ch1);
        VERIFY_ARE_EQUAL(ch0, L'f');

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }
#endif

    TEST(ReadBuffer1)
    {
        // Test that seeking works.
        utility::string_t fname = U("ReadBuffer1.txt");
        fill_file(fname);

        // In order to get the implementation to buffer reads, we have to open the file
        // with protection against sharing.
        auto stream = OPEN_R<char>(fname, _SH_DENYRW).get();
        stream.set_buffer_size(512);

        VERIFY_IS_TRUE(stream.is_open());

        char buf[10];
        memset(buf, 0, sizeof(buf));

        auto read = stream.getn(buf, sizeof(buf)).then([=](pplx::task<size_t> op) -> size_t { return op.get(); });

        VERIFY_ARE_EQUAL(sizeof(buf), read.get());

        bool elements_equal = true;

        for (int i = 0; i < sizeof(buf); i++)
        {
            elements_equal = elements_equal && (buf[i] == 'a' + i);
        }

        VERIFY_IS_TRUE(elements_equal);

        stream.seekpos(3, std::ios_base::in);

        memset(buf, 0, sizeof(buf));

        read = stream.getn(buf, sizeof(buf)).then([=](pplx::task<size_t> op) -> size_t { return op.get(); });

        VERIFY_ARE_EQUAL(sizeof(buf), read.get());

        elements_equal = true;

        for (int i = 0; i < sizeof(buf); i++)
        {
            elements_equal = elements_equal && (buf[i] == 'd' + i);
        }

        VERIFY_IS_TRUE(elements_equal);

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

#ifdef _WIN32
    TEST(ReadBuffer1w)
    {
        // Test that seeking works.
        utility::string_t fname = U("ReadBuffer1w.txt");
        fill_file_w(fname);

        // In order to get the implementation to buffer reads, we have to open the file
        // with protection against sharing.
        auto stream = OPEN_R<wchar_t>(fname, _SH_DENYRW).get();

        VERIFY_IS_TRUE(stream.is_open());

        wchar_t buf[10];
        memset(buf, 0, sizeof(buf));

        auto read = stream.getn(buf, 10).then([=](pplx::task<size_t> op) -> size_t { return op.get(); });

        VERIFY_ARE_EQUAL(10u, read.get());

        bool elements_equal = true;

        for (int i = 0; i < 10; i++)
        {
            elements_equal = elements_equal && (buf[i] == L'a' + i);
        }

        VERIFY_IS_TRUE(elements_equal);

        stream.seekpos(3, std::ios_base::in);

        memset(buf, 0, sizeof(buf));

        read = stream.getn(buf, 10).then([=](pplx::task<size_t> op) -> size_t { return op.get(); });

        VERIFY_ARE_EQUAL(10u, read.get());

        elements_equal = true;

        for (int i = 0; i < 10; i++)
        {
            elements_equal = elements_equal && (buf[i] == L'd' + i);
        }

        VERIFY_IS_TRUE(elements_equal);

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }
#endif

    TEST(ReadBuffer2)
    {
        // Test that seeking works when the file is larger than the internal buffer size.
        utility::string_t fname = U("ReadBuffer2.txt");
        fill_file(fname, 30);

        // In order to get the implementation to buffer reads, we have to open the file
        // with protection against sharing.
        auto stream = OPEN_R<char>(fname, _SH_DENYRW).get();

        VERIFY_IS_TRUE(stream.is_open());

        char buf[10];
        memset(buf, 0, sizeof(buf));

        auto read = stream.getn(buf, sizeof(buf)).then([=](pplx::task<size_t> op) -> size_t { return op.get(); });

        VERIFY_ARE_EQUAL(sizeof(buf), read.get());

        bool elements_equal = true;

        for (int i = 0; i < sizeof(buf); i++)
        {
            elements_equal = elements_equal && (buf[i] == 'a' + i);
        }

        VERIFY_IS_TRUE(elements_equal);

        // Test that we can seek to a position near the end of the initial buffer,
        // read a chunk spanning the end of the buffer, and get a correct outcome.

        stream.seekpos(505, std::ios_base::in);

        memset(buf, 0, sizeof(buf));

        read = stream.getn(buf, sizeof(buf)).then([=](pplx::task<size_t> op) -> size_t { return op.get(); });

        VERIFY_ARE_EQUAL(sizeof(buf), read.get());

        elements_equal = true;

        for (int i = 0; i < sizeof(buf); i++)
        {
            elements_equal = elements_equal && (buf[i] == 'l' + i);
        }

        VERIFY_IS_TRUE(elements_equal);

        stream.close().get();

        VERIFY_IS_FALSE(stream.is_open());
    }

    TEST(SeekEnd1)
    {
        utility::string_t fname = U("SeekEnd1.txt");
        fill_file(fname, 30);

        // In order to get the implementation to buffer reads, we have to open the file
        // with protection against sharing.
        auto stream = OPEN_R<char>(fname).get();

        auto pos = stream.seekoff(0, std::ios_base::end, std::ios_base::in);

        VERIFY_ARE_EQUAL(30 * 26, pos);
    }

    TEST(IsEOFTest)
    {
        utility::string_t fname = U("IsEOFTest.txt");
        fill_file(fname, 30);

        auto stream = OPEN_R<char>(fname).get();
        VERIFY_IS_FALSE(stream.is_eof());
        stream.getc().wait();
        VERIFY_IS_FALSE(stream.is_eof());
        stream.seekoff(0, std::ios_base::end, std::ios_base::in);
        VERIFY_IS_FALSE(stream.is_eof());
        stream.getc().wait();
        VERIFY_IS_TRUE(stream.is_eof());
        stream.seekoff(0, std::ios_base::beg, std::ios_base::in);
        VERIFY_IS_TRUE(stream.is_eof());
        stream.getc().wait();
        VERIFY_IS_FALSE(stream.is_eof());
    }

    TEST(CloseWithException)
    {
        struct MyException
        {
        };
        auto streambuf = OPEN_W<char>(U("CloseExceptionTest.txt")).get();
        streambuf.close(std::ios::out, std::make_exception_ptr(MyException())).wait();
        VERIFY_THROWS(streambuf.putn_nocopy("this is good", 10).get(), MyException);
        VERIFY_THROWS(streambuf.putc('c').get(), MyException);

        streambuf = OPEN_R<char>(U("CloseExceptionTest.txt")).get();
        streambuf.close(std::ios::in, std::make_exception_ptr(MyException())).wait();
        char buf[100];
        VERIFY_THROWS(streambuf.getn(buf, 100).get(), MyException);
        VERIFY_THROWS(streambuf.getc().get(), MyException);
    }

    TEST(inout_regression_test)
    {
        std::string data = "abcdefghijklmn";
        concurrency::streams::streambuf<char> file_buf =
            OPEN<char>(U("inout_regression_test.txt"), std::ios_base::in | std::ios_base::out).get();
        file_buf.putn_nocopy(&data[0], data.size()).get();

        file_buf.bumpc().get(); // reads 'a'

        char readdata[256];
        memset(&readdata[0], '\0', 256);

        file_buf.seekoff(0, std::ios::beg, std::ios::in);
        auto data_read = file_buf.getn(&readdata[0], 3).get(); // reads 'bcd'. File contains the org string though!!!

        memset(&readdata[0], '\0', 256);

        file_buf.seekoff(0, std::ios::beg, std::ios::in);
        data_read = file_buf.getn(&readdata[0], 3).get(); // reads 'efg'. File contains org string 'abcdef..'.

        file_buf.close().wait();
    }

    TEST(seek_read_regression_test)
    {
        utility::string_t fname = U("seek_read_regression_test.txt");
        fill_file(fname, 100);

        char readdata[256];

        auto istream = OPEN_R<char>(fname, _SH_DENYRW).get().create_istream();
        istream.streambuf().set_buffer_size(128);

        {
            istream.seek(50, std::ios_base::beg);
            concurrency::streams::rawptr_buffer<char> block(readdata, sizeof(readdata));
            istream.read(block, 50).get();
        }

        {
            istream.seek(256, std::ios_base::beg);
            concurrency::streams::rawptr_buffer<char> block(readdata, sizeof(readdata));
            istream.read(block, 256).get();
        }

        istream.close().get();
    }

    TEST(file_size)
    {
        utility::string_t fname = U("file_size.txt");
        fill_file(fname, 100);
        auto istream = OPEN_R<char>(fname).get();
        VERIFY_IS_TRUE(istream.has_size());
        VERIFY_ARE_EQUAL(istream.size(), 2600);
    }

#ifdef _WIN32
    TEST(file_size_w)
    {
        utility::string_t fname = U("file_size_w.txt");
        fill_file_w(fname, 100);
        auto istream = OPEN_R<wchar_t>(fname).get();
        VERIFY_IS_TRUE(istream.has_size());
        VERIFY_ARE_EQUAL(istream.size(), 2600);
    }

    TEST(file_with_one_byte_size)
    {
        // Create a file with one byte.
        concurrency::streams::streambuf<char> file_buf = OPEN<char>(U("one_byte_file.txt"), std::ios_base::out).get();
        file_buf.putc('a').wait();
        file_buf.close().wait();

        // Try to read from file with a 2 byte character.
        concurrency::streams::basic_istream<wchar_t> inFile(OPEN<wchar_t>(U("one_byte_file.txt"), std::ios::in).get());
        concurrency::streams::container_buffer<std::wstring> buffer;
        VERIFY_ARE_EQUAL(inFile.read(buffer, 1).get(), 0);
        VERIFY_IS_TRUE(inFile.is_eof());
    }
#endif

#if defined(_WIN32) && (!defined(__cplusplus_winrt)) && defined(_WIN64)
    // since casablanca does not use sparse file apis we're not doing the reverse test (write one byte at 4Gb and verify
    // with std apis) because the file created would be too big
    TEST(read_one_byte_at_4G)
    {
        // Create a file with one byte.
        string_t filename = U("read_one_byte_at_4G.txt");
        // create a sparse file with sparse file apis
        auto handle = CreateSparseFile(filename.c_str());
        VERIFY_ARE_NOT_EQUAL(handle, INVALID_HANDLE_VALUE);

        // write 1 byte
        auto data = 'a';

        DWORD dwBytesWritten;
        LARGE_INTEGER i;
        i.QuadPart = 0x100000000;

        SetFilePointerEx(handle, i /*4GB*/, NULL, FILE_END);
        WriteFile(handle, &data, 1, &dwBytesWritten, NULL);

        CloseHandle(handle);

        // read the file with casablanca streams
        concurrency::streams::streambuf<char> file_buf = OPEN<char>(filename, std::ios_base::in).get();
        file_buf.seekoff(4 * 1024 * 1024 * 1024ll, ::std::ios_base::beg, ::std::ios_base::in);

        int aCharacter = file_buf.getc().get();
        file_buf.close().wait();

        VERIFY_ARE_EQUAL(aCharacter, data);
    }
#endif

#if !defined(_WIN32) && defined(__x86_64__)

    struct TidyStream
    {
        string_t _fileName;
        concurrency::streams::streambuf<char> _stream;

        TidyStream(string_t filename)
        {
            _fileName = filename;
            _stream = OPEN<char>(filename, std::ios_base::out | ::std::ios_base::in).get();
        }

        ~TidyStream()
        {
            _stream.close().wait();
            std::remove(_fileName.c_str());
        }
    };

    TEST(write_one_byte_at_4G)
    {
        // write using casablanca streams
        concurrency::streams::streambuf<char>::off_type pos = 4 * 1024 * 1024 * 1024ll;

        string_t filename = U("write_one_byte_at_4G.txt");
        TidyStream file_buf(filename);
        file_buf._stream.seekoff(pos, ::std::ios_base::beg, ::std::ios_base::out);
        file_buf._stream.putc('a').wait();
        file_buf._stream.sync().get();

        // verify with std streams
        std::fstream stream(get_full_name(filename), std::ios_base::in);
        stream.seekg(pos);
        char c;
        stream >> c;
        stream.close();
        VERIFY_ARE_EQUAL(c, 'a');
    }

    TEST(read_one_byte_at_4G)
    {
        // write with std stream
        concurrency::streams::streambuf<char>::off_type pos = 4 * 1024 * 1024 * 1024ll;
        // Create a file with one byte.
        string_t filename = U("read_one_byte_at_4G.txt");

        std::fstream stream(get_full_name(filename), std::ios_base::out);
        stream.seekg(pos);
        stream << 'a';
        stream.close();

        // verify with casablanca streams
        TidyStream file_buf(filename);
        file_buf._stream.seekoff(pos, ::std::ios_base::beg, ::std::ios_base::in);
        int aCharacter = file_buf._stream.getc().get();

        VERIFY_ARE_EQUAL(aCharacter, 'a');
    }

#endif

    TEST(alloc_acquire_not_supported)
    {
        concurrency::streams::streambuf<char> file_buf =
            OPEN<char>(U("alloc_not_supported.txt"), std::ios::out | std::ios::in).get();
        VERIFY_IS_TRUE(file_buf.alloc(1) == nullptr);
        char* temp;
        size_t size;
        VERIFY_IS_FALSE(file_buf.acquire(temp, size));
    }

    TEST(read_alloc_acquire_not_supported)
    {
        auto file_buf1 = OPEN<char>(U("read_acquire_not_supported1.txt"), std::ios::out | std::ios::in).get();
        auto file_buf2 = OPEN<char>(U("read_acquire_not_supported2.txt"), std::ios::out | std::ios::in).get();

        concurrency::streams::stringstreambuf data_buf("A");
        file_buf1.create_ostream().write(data_buf, 1).wait();
        file_buf1.sync().wait();
        file_buf2.create_ostream().write(file_buf1, 1).wait();
        file_buf2.sync().wait();

        file_buf2.create_istream().read(file_buf1, 1).wait();
        file_buf1.sync().wait();
        file_buf1.seekpos(0, std::ios::in);
        data_buf = concurrency::streams::stringstreambuf();
        file_buf1.create_istream().read(data_buf, 2).wait();
        const auto& data = data_buf.collection();
        VERIFY_ARE_EQUAL(data[0], 'A');
        VERIFY_ARE_EQUAL(data[1], 'A');

        file_buf1.close().wait();
        file_buf2.close().wait();
    }

    TEST(winrt_filestream_close)
    {
        std::string str("test data");
        auto t = OPEN_W<uint8_t>(U("file.txt")).then([this, str](concurrency::streams::ostream stream) {
            concurrency::streams::container_buffer<std::string> rbuf(str);
            concurrency::streams::istream is(rbuf);
            size_t size = 0;
            is.read(stream.streambuf(), 1).wait();
            while (!is.is_eof())
            {
                is.read(stream.streambuf(), 1).wait();
                size += 1;
            }

            return stream.flush().then([size, stream]() {
                stream.close();
                return size;
            });
        });

        VERIFY_ARE_EQUAL(t.get(), str.length());
    }
} // SUITE(file_buffer_tests)

} // namespace streams
} // namespace functional
} // namespace tests
