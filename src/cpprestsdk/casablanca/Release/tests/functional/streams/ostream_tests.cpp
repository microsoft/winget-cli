/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests for async output stream operations.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

using namespace concurrency::streams;

#if defined(__cplusplus_winrt)
using namespace Windows::Storage;
#endif

namespace tests
{
namespace functional
{
namespace streams
{
using namespace utility;
using namespace ::pplx;

//
// The following two functions will help mask the differences between non-WinRT environments and
// WinRT: on the latter, a file path is typically not used to open files. Rather, a UI element is used
// to get a 'StorageFile' reference and you go from there. However, to test the library properly,
// we need to get a StorageFile reference somehow, and one way to do that is to create all the files
// used in testing in the Documents folder.
//
template<typename _CharType>
pplx::task<concurrency::streams::basic_ostream<_CharType>> OPENSTR_W(const utility::string_t& name,
                                                                     std::ios_base::openmode mode = std::ios_base::out)
{
#if !defined(__cplusplus_winrt)
    return concurrency::streams::file_stream<_CharType>::open_ostream(name, mode);
#else
    auto file = pplx::create_task(KnownFolders::DocumentsLibrary->CreateFileAsync(
                                      ref new Platform::String(name.c_str()), CreationCollisionOption::ReplaceExisting))
                    .get();

    return concurrency::streams::file_stream<_CharType>::open_ostream(file, mode);
#endif
}

#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4100) // Because of '_Prot' in WinRT builds.
#endif
template<typename _CharType>
pplx::task<concurrency::streams::basic_istream<_CharType>> OPENSTR_R(const utility::string_t& name,
                                                                     std::ios_base::openmode mode = std::ios_base::in)
{
#if !defined(__cplusplus_winrt)
    return concurrency::streams::file_stream<_CharType>::open_istream(name, mode);
#else
    auto file =
        pplx::create_task(KnownFolders::DocumentsLibrary->GetFileAsync(ref new Platform::String(name.c_str()))).get();

    return concurrency::streams::file_stream<_CharType>::open_istream(file, mode);
#endif
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

SUITE(ostream_tests)
{
    TEST(BasicTest1)
    {
        auto open = OPENSTR_W<uint8_t>(U("BasicTest1.txt"));
        auto basic_stream = open.get();
        VERIFY_IS_TRUE(basic_stream.can_seek());
        auto a = basic_stream.print(10);
        auto b = basic_stream.print("-suffix");
        (a && b).wait();
        auto cls = basic_stream.close();
        cls.get();
        VERIFY_IS_TRUE(cls.is_done());
    }

    TEST(BasicTest2)
    {
        auto open = OPENSTR_W<uint8_t>(U("BasicTest2.txt"));

        auto cls = open.then([](pplx::task<concurrency::streams::ostream> op) -> pplx::task<void> {
            auto basic_stream = op.get();
            auto a = basic_stream.print(10);
            auto b = basic_stream.print("-suffix");
            (a && b).wait();
            return basic_stream.close();
        });

        cls.get();

        VERIFY_IS_TRUE(cls.is_done());
    }

    TEST(WriteSingleCharTest2)
    {
        auto open = OPENSTR_W<uint8_t>(U("WriteSingleCharStrTest1.txt"));
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());

        bool elements_equal = true;

        for (uint8_t ch = 'a'; ch <= 'z'; ch++)
        {
            elements_equal = elements_equal && (ch == stream.write(ch).get());
        }

        VERIFY_IS_TRUE(elements_equal);

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
    }

    TEST(WriteBufferTest1)
    {
        auto open = OPENSTR_W<uint8_t>(U("WriteBufferStrTest1.txt"));
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());

        std::vector<uint8_t> vect;

        for (char ch = 'a'; ch <= 'z'; ch++)
        {
            vect.push_back(ch);
        }

        size_t vsz = vect.size();

        concurrency::streams::container_stream<std::vector<uint8_t>>::buffer_type txtbuf(std::move(vect),
                                                                                         std::ios_base::in);

        VERIFY_ARE_EQUAL(stream.write(txtbuf, vsz).get(), vsz);

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
    }

    TEST(WriteBufferAndSyncTest1)
    {
        auto open = OPENSTR_W<uint8_t>(U("WriteBufferAndSyncStrTest1.txt"));
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());

        std::vector<char> vect;

        for (char ch = 'a'; ch <= 'z'; ch++)
        {
            vect.push_back(ch);
        }

        size_t vsz = vect.size();
        concurrency::streams::rawptr_buffer<uint8_t> txtbuf(reinterpret_cast<const uint8_t*>(&vect[0]), vsz);

        auto write = stream.write(txtbuf, vsz);
        stream.flush().get();

        VERIFY_ARE_EQUAL(write.get(), vect.size());
        VERIFY_IS_TRUE(write.is_done());

        auto close = stream.close();
        close.get();

        VERIFY_IS_TRUE(close.is_done());
    }

    TEST(tell_bug)
    {
        auto count = OPENSTR_W<uint8_t>(U("tell_bug.txt"), std::ios_base::out | std::ios_base::trunc)
                         .then([=](concurrency::streams::ostream os) -> std::streamoff {
                             os.print("A");
                             auto val = os.tell();
                             os.close().get();
                             return val;
                         })
                         .get();

        VERIFY_ARE_EQUAL(std::streamoff(1), count);
    }

    TEST(iostream_container_buffer1)
    {
        concurrency::streams::container_buffer<std::vector<char>> buf;

        auto os = buf.create_ostream();
        os.write('a');
        os.write('b');
        os.close();

        auto is = concurrency::streams::container_stream<std::vector<char>>::open_istream(std::move(buf.collection()));
        VERIFY_ARE_EQUAL(is.read().get(), 'a');
        VERIFY_ARE_EQUAL(is.read().get(), 'b');
    }

    TEST(iostream_container_buffer2)
    {
        concurrency::streams::container_buffer<std::vector<char>> buf;

        {
            auto os = buf.create_ostream();
            os.write('a');
            os.write('b');
            os.close();
        }

        {
            auto is =
                concurrency::streams::container_stream<std::vector<char>>::open_istream(std::move(buf.collection()));

            is.read()
                .then([&is](concurrency::streams::basic_ostream<char>::int_type c) {
                    VERIFY_ARE_EQUAL(c, 'a');
                    return is.read();
                })
                .then([&is](concurrency::streams::basic_ostream<char>::int_type c) -> pplx::task<void> {
                    VERIFY_ARE_EQUAL(c, 'b');
                    return is.close();
                })
                .wait();
        }
    }

    TEST(extract_on_space)
    {
        const int number1 = 42;
        const int number2 = 123;

        auto open = OPENSTR_W<uint8_t>(U("SpaceWithNumber.txt"), std::ios::trunc);
        auto stream = open.get();
        VERIFY_IS_TRUE(open.is_done());
        stream.print("  \r").wait();
        stream.print(number1).wait();
        stream.print("\n \t").wait();
        stream.print(number2).wait();
        stream.print(" \f \v ").wait();
        stream.close().wait();

        auto istream = OPENSTR_R<uint8_t>(U("SpaceWithNumber.txt")).get();
        VERIFY_IS_TRUE(istream.can_seek());
        VERIFY_ARE_EQUAL(number1, istream.extract<int>().get());
        VERIFY_ARE_EQUAL(number2, istream.extract<long long>().get());
    }

    TEST(file_sequential_write)
    {
        auto open = OPENSTR_W<uint8_t>(U("WriteFileSequential.txt"), std::ios::trunc);
        auto stream = open.get();

        VERIFY_IS_TRUE(open.is_done());

        std::vector<pplx::task<size_t>> v;
        for (int i = 0; i < 100; i++)
        {
            v.push_back(stream.print(i));
            v.push_back(stream.print(' '));
        }
        pplx::when_all(v.begin(), v.end()).wait();
        stream.close().wait();
        auto istream = OPENSTR_R<uint8_t>(U("WriteFileSequential.txt")).get();
        for (int i = 0; i < 100; i++)
        {
            int int_read = istream.extract<int>().get();
            if (int_read != i)
            {
                // This will fail
                VERIFY_ARE_EQUAL(int_read, i);

                // This return statment will prevent the test from hanging,
                // cause if the numbers are merged there will be less than 100 numbers,
                // and reading from the file will block
                return;
            }
            istream.read().get();
        }
    }

    TEST(implied_out_mode)
    {
        auto ostr = OPENSTR_W<char>(U("implied_out_mode.txt"), std::ios::ios_base::app).get();

        std::string str = "abcd";
        concurrency::streams::stringstreambuf block(str);

        size_t s = ostr.write(block, str.size()).get();

        VERIFY_ARE_EQUAL(s, str.size());

        auto cls = ostr.close();

        cls.get();
        VERIFY_IS_TRUE(cls.is_done());
    }

    TEST(create_ostream_from_input_only)
    {
        container_buffer<std::string> sourceBuf("test data");
        VERIFY_THROWS(sourceBuf.create_ostream(), std::runtime_error);
    }

    TEST(streambuf_close_with_exception_write)
    {
        container_buffer<std::string> sourceBuf;
        sourceBuf.close(std::ios::out, std::make_exception_ptr(std::invalid_argument("custom exception"))).wait();

        const size_t size = 4;
        char targetBuf[size];
        auto t1 = sourceBuf.putn_nocopy(targetBuf, size);
        VERIFY_THROWS(t1.get(), std::invalid_argument);
    }

    TEST(stream_close_with_exception_write)
    {
        container_buffer<std::string> sourceBuf;
        auto outStream = sourceBuf.create_ostream();
        outStream.close(std::make_exception_ptr(std::invalid_argument("custom exception"))).wait();

        container_buffer<std::string> targetBuf("test data");
        auto t1 = outStream.write(targetBuf, 4);
        VERIFY_THROWS(t1.get(), std::invalid_argument);
    }

    TEST(input_after_close)
    {
        container_buffer<std::string> sourceBuf;
        auto outStream = sourceBuf.create_ostream();
        outStream.close().wait();

        container_buffer<std::string> targetBuf;

        auto t1 = outStream.flush();
        auto t2 = outStream.print('a');
        auto t3 = outStream.print(std::string("abc"));

        VERIFY_THROWS(t1.get(), std::runtime_error);
        VERIFY_THROWS(t2.get(), std::runtime_error);
        VERIFY_THROWS(t3.get(), std::runtime_error);
        VERIFY_THROWS(outStream.seek(0), std::runtime_error);
        VERIFY_THROWS(outStream.seek(0, std::ios::beg), std::runtime_error);
        VERIFY_THROWS(outStream.tell(), std::runtime_error);

        auto t4 = outStream.write('a');
        auto t5 = outStream.write(targetBuf, 1);
        VERIFY_THROWS(t4.get(), std::runtime_error);
        VERIFY_THROWS(t5.get(), std::runtime_error);
    }

    TEST(write_emptybuffer_to_ostream)
    {
        auto ofs = OPENSTR_W<char>(U("file.txt")).get();
        auto sbuf = concurrency::streams::producer_consumer_buffer<char>();
        auto result = ofs.write(sbuf, 0);
        VERIFY_ARE_EQUAL(result.get(), 0);
    }

    TEST(write_stream_twice)
    {
        producer_consumer_buffer<uint8_t> buf1;
        auto t1 = pplx::create_task([&] {
            buf1.alloc(8);
            buf1.alloc(9);
        });
        VERIFY_THROWS(t1.get(), std::logic_error);

        std::string strData("test string to write\n");
        container_buffer<std::string> buf2(std::move(strData));
        auto t2 = pplx::create_task([&] {
            buf2.commit(8);
            buf2.alloc(9);
        });
        VERIFY_THROWS(t2.get(), std::logic_error);

        rawptr_buffer<std::string> buf3;
        auto t3 = pplx::create_task([&] {
            buf3.commit(8);
            buf3.commit(9);
        });
        VERIFY_THROWS(t3.get(), std::logic_error);
    }

} // SUITE(ostream_tests)

} // namespace streams
} // namespace functional
} // namespace tests
