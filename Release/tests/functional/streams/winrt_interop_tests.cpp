/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests for winrt interop streams.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

using namespace concurrency::streams;
using namespace utility;
using namespace ::pplx;

#if defined(__cplusplus_winrt)
using namespace Windows::Storage;
#endif

namespace tests
{
namespace functional
{
namespace streams
{
SUITE(winrt_interop_tests)
{
    TEST(read_in)
    {
        producer_consumer_buffer<char> buf;
        auto ostream = buf.create_ostream();
        std::string strData("abcdefghij");
        buf.putn_nocopy((char*)&strData[0], strData.size() * sizeof(char)).wait();

        auto dr = ref new Windows::Storage::Streams::DataReader(winrt_stream::create_input_stream(buf));
        dr->ByteOrder = Windows::Storage::Streams::ByteOrder::LittleEndian;

        {
            VERIFY_ARE_EQUAL(10, pplx::create_task(dr->LoadAsync(10)).get());

            auto value = dr->ReadString(5);
            VERIFY_ARE_EQUAL(utility::string_t(value->Data()), U("abcde"));
            value = dr->ReadString(5);
            VERIFY_ARE_EQUAL(utility::string_t(value->Data()), U("fghij"));
        }
        {
            ostream.write(char(11)).wait();
            ostream.write(char(17)).wait();

            VERIFY_ARE_EQUAL(2, pplx::create_task(dr->LoadAsync(2)).get());

            auto ival = dr->ReadByte();
            VERIFY_ARE_EQUAL(ival, 11);
            ival = dr->ReadByte();
            VERIFY_ARE_EQUAL(ival, 17);
        }
        {
            for (int i = 0; i < 100; i++)
            {
                ostream.write(char(i)).wait();
            }

            VERIFY_ARE_EQUAL(100, pplx::create_task(dr->LoadAsync(100)).get());

            auto arr = ref new Platform::Array<unsigned char, 1>(100);
            dr->ReadBytes(arr);

            for (int i = 0; i < 100; i++)
            {
                VERIFY_ARE_EQUAL(arr[i], i);
            }
        }
        buf.close(std::ios_base::out);
    }

    TEST(read_rand)
    {
        producer_consumer_buffer<char> buf;
        auto ostream = buf.create_ostream();
        std::string strData("abcdefghij");
        buf.putn_nocopy((char*)&strData[0], strData.size() * sizeof(char)).wait();

        auto dr = ref new Windows::Storage::Streams::DataReader(winrt_stream::create_random_access_stream(buf));
        dr->ByteOrder = Windows::Storage::Streams::ByteOrder::LittleEndian;

        {
            VERIFY_ARE_EQUAL(10, pplx::create_task(dr->LoadAsync(10)).get());

            auto value = dr->ReadString(5);
            VERIFY_ARE_EQUAL(utility::string_t(value->Data()), U("abcde"));
            value = dr->ReadString(5);
            VERIFY_ARE_EQUAL(utility::string_t(value->Data()), U("fghij"));
        }
        {
            ostream.write(char(11)).wait();
            ostream.write(char(17)).wait();

            VERIFY_ARE_EQUAL(2, pplx::create_task(dr->LoadAsync(2)).get());

            auto ival = dr->ReadByte();
            VERIFY_ARE_EQUAL(ival, 11);
            ival = dr->ReadByte();
            VERIFY_ARE_EQUAL(ival, 17);
        }
        {
            for (int i = 0; i < 100; i++)
            {
                ostream.write(char(i)).wait();
            }

            VERIFY_ARE_EQUAL(100, pplx::create_task(dr->LoadAsync(100)).get());
            auto arr = ref new Platform::Array<unsigned char, 1>(100);
            dr->ReadBytes(arr);

            for (int i = 0; i < 100; i++)
            {
                VERIFY_ARE_EQUAL(arr[i], i);
            }
        }
        buf.close(std::ios_base::out);
    }

    pplx::task<bool> StoreAndFlush(Windows::Storage::Streams::DataWriter ^ dw)
    {
        return pplx::create_task(dw->StoreAsync()).then([dw](unsigned int) {
            return pplx::create_task(dw->FlushAsync());
        });
    }

    TEST(write_out)
    {
        producer_consumer_buffer<char> buf;

        auto dw = ref new Windows::Storage::Streams::DataWriter(winrt_stream::create_output_stream(buf));
        dw->ByteOrder = Windows::Storage::Streams::ByteOrder::LittleEndian;

        auto value = ref new ::Platform::String(U("10 4711 -10.0 hello!"));
        dw->WriteString(value);
        dw->WriteByte(11); // Take care to make this a non-character!
        dw->WriteUInt16(17);
        dw->WriteUInt32(4711);
        VERIFY_IS_TRUE(StoreAndFlush(dw).get());
        buf.close(std::ios_base::out);

        auto istream = buf.create_istream();
        VERIFY_ARE_EQUAL(10, istream.extract<unsigned int>().get());
        VERIFY_ARE_EQUAL(4711, istream.extract<int>().get());
        VERIFY_ARE_EQUAL(-10.0, istream.extract<double>().get());
        VERIFY_ARE_EQUAL(utility::string_t(U("hello!")), istream.extract<utility::string_t>().get());
        VERIFY_ARE_EQUAL(11, istream.read().get());
        uint16_t int16;
        buf.getn((char*)&int16, sizeof(int16)).wait();
        VERIFY_ARE_EQUAL(17, int16);
        uint32_t int32;
        buf.getn((char*)&int32, sizeof(int32)).wait();
        VERIFY_ARE_EQUAL(4711, int32);
    }

    TEST(write_rand)
    {
        producer_consumer_buffer<char> buf;

        auto dw = ref new Windows::Storage::Streams::DataWriter(winrt_stream::create_random_access_stream(buf));
        dw->ByteOrder = Windows::Storage::Streams::ByteOrder::LittleEndian;

        auto value = ref new ::Platform::String(U("10 4711 -10.0 hello!"));
        dw->WriteString(value);
        dw->WriteByte(11); // Take care to make this a non-character!
        dw->WriteUInt16(17);
        dw->WriteUInt32(4711);
        VERIFY_IS_TRUE(StoreAndFlush(dw).get());
        buf.close(std::ios_base::out);

        auto istream = buf.create_istream();
        VERIFY_ARE_EQUAL(10, istream.extract<unsigned int>().get());
        VERIFY_ARE_EQUAL(4711, istream.extract<int>().get());
        VERIFY_ARE_EQUAL(-10.0, istream.extract<double>().get());
        VERIFY_ARE_EQUAL(utility::string_t(U("hello!")), istream.extract<utility::string_t>().get());
        VERIFY_ARE_EQUAL(11, istream.read().get());
        uint16_t int16;
        buf.getn((char*)&int16, sizeof(int16)).wait();
        VERIFY_ARE_EQUAL(17, int16);
        uint32_t int32;
        buf.getn((char*)&int32, sizeof(int32)).wait();
        VERIFY_ARE_EQUAL(4711, int32);
    }

    TEST(read_write_attributes)
    {
        {
            container_buffer<std::string> buf("test data");
            auto rastr = winrt_stream::create_random_access_stream(buf);
            VERIFY_IS_TRUE(rastr->CanRead);
            VERIFY_IS_FALSE(rastr->CanWrite);
            VERIFY_ARE_EQUAL(rastr->Position, 0);

            VERIFY_ARE_EQUAL(rastr->Size, 9);
            rastr->Size = 1024U;
            VERIFY_ARE_EQUAL(rastr->Size, 9);
        }
        {
            container_buffer<std::string> buf;
            auto rastr = winrt_stream::create_random_access_stream(buf);
            VERIFY_IS_FALSE(rastr->CanRead);
            VERIFY_IS_TRUE(rastr->CanWrite);
            VERIFY_ARE_EQUAL(rastr->Position, 0);

            VERIFY_ARE_EQUAL(rastr->Size, 0);
            rastr->Size = 1024U;
            VERIFY_ARE_EQUAL(rastr->Size, 1024U);
            VERIFY_ARE_EQUAL(buf.collection().size(), 1024U);
        }
        {
            producer_consumer_buffer<uint8_t> buf;
            auto rastr = winrt_stream::create_random_access_stream(buf);
            VERIFY_IS_TRUE(rastr->CanRead);
            VERIFY_IS_TRUE(rastr->CanWrite);
            VERIFY_ARE_EQUAL(rastr->Position, 0);

            VERIFY_ARE_EQUAL(rastr->Size, 0);
            rastr->Size = 1024U;
            VERIFY_ARE_EQUAL(rastr->Size, 1024U);
        }
    }

    TEST(cant_write)
    {
        container_buffer<std::string> buf("test data");

        auto ostr = winrt_stream::create_output_stream(buf);
        auto dw = ref new Windows::Storage::Streams::DataWriter(ostr);
        dw->ByteOrder = Windows::Storage::Streams::ByteOrder::LittleEndian;

        auto value = ref new ::Platform::String(U("10 4711 -10.0 hello!"));
        dw->WriteString(value);

        VERIFY_IS_FALSE(StoreAndFlush(dw).get());
    }

    TEST(cant_read)
    {
        container_buffer<std::string> buf;
        auto ostream = buf.create_ostream();
        ostream.print<int>(10);

        auto istr = winrt_stream::create_input_stream(buf);
        auto dr = ref new Windows::Storage::Streams::DataReader(istr);
        dr->ByteOrder = Windows::Storage::Streams::ByteOrder::LittleEndian;

        VERIFY_ARE_EQUAL(0, pplx::create_task(dr->LoadAsync(2)).get());
    }

} // SUITE

} // namespace streams
} // namespace functional
} // namespace tests
