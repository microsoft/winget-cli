/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests for async memory stream buffer operations.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"
#if defined(__cplusplus_winrt)
#include <wrl.h>
#endif
#ifdef _WIN32

#include <Windows.h>
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

template<class StreamBufferType>
void streambuf_putc(StreamBufferType& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());

    std::basic_string<typename StreamBufferType::char_type> s;
    s.push_back((typename StreamBufferType::char_type)0);
    s.push_back((typename StreamBufferType::char_type)1);
    s.push_back((typename StreamBufferType::char_type)2);
    s.push_back((typename StreamBufferType::char_type)3);

    // Verify putc synchronously
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[0], wbuf.putc(s[0]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[1], wbuf.putc(s[1]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[2], wbuf.putc(s[2]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[3], wbuf.putc(s[3]).get());

    VERIFY_ARE_EQUAL(s.size(), wbuf.in_avail());

    // Verify putc async
    size_t count = 10;
    auto seg2 = [&count](typename StreamBufferType::int_type) { return (--count > 0); };
    auto seg1 = [&s, &wbuf, seg2]() { return wbuf.putc(s[0]).then(seg2); };
    pplx::details::_do_while(seg1).wait();

    VERIFY_ARE_EQUAL(s.size() + 10, wbuf.in_avail());

    VERIFY_IS_TRUE(wbuf.close().get());
    VERIFY_IS_FALSE(wbuf.can_write());

    // verify putc after close
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), wbuf.putc(s[0]).get());
}

template<class CharType>
void streambuf_putc(concurrency::streams::rawptr_buffer<CharType>& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());
    typedef concurrency::streams::rawptr_buffer<CharType> StreamBufferType;

    std::basic_string<CharType> s;
    s.push_back((CharType)0);
    s.push_back((CharType)1);
    s.push_back((CharType)2);
    s.push_back((CharType)3);

    // Verify putc synchronously
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[0], wbuf.putc(s[0]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[1], wbuf.putc(s[1]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[2], wbuf.putc(s[2]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[3], wbuf.putc(s[3]).get());

    VERIFY_ARE_EQUAL(s.size(), wbuf.block().size());

    // Verify putc async
    size_t count = 10;
    auto seg2 = [&count](typename StreamBufferType::int_type) { return (--count > 0); };
    auto seg1 = [&s, &wbuf, seg2]() { return wbuf.putc(s[0]).then(seg2); };
    pplx::details::_do_while(seg1).wait();

    VERIFY_ARE_EQUAL(s.size() + 10, wbuf.block().size());

    VERIFY_IS_TRUE(wbuf.close().get());
    VERIFY_IS_FALSE(wbuf.can_write());

    // verify putc after close
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), wbuf.putc(s[0]).get());
}

template<class CollectionType>
void streambuf_putc(concurrency::streams::container_buffer<CollectionType>& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());
    typedef concurrency::streams::container_buffer<CollectionType> StreamBufferType;
    typedef typename concurrency::streams::container_buffer<CollectionType>::char_type CharType;

    std::basic_string<CharType> s;
    s.push_back((CharType)0);
    s.push_back((CharType)1);
    s.push_back((CharType)2);
    s.push_back((CharType)3);

    // Verify putc synchronously
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[0], wbuf.putc(s[0]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[1], wbuf.putc(s[1]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[2], wbuf.putc(s[2]).get());
    VERIFY_ARE_EQUAL((typename StreamBufferType::int_type)s[3], wbuf.putc(s[3]).get());

    VERIFY_ARE_EQUAL(s.size(), wbuf.collection().size());

    // Verify putc async
    size_t count = 10;
    auto seg2 = [&count](typename StreamBufferType::int_type) { return (--count > 0); };
    auto seg1 = [&s, &wbuf, seg2]() { return wbuf.putc(s[0]).then(seg2); };
    pplx::details::_do_while(seg1).wait();

    VERIFY_ARE_EQUAL(s.size() + 10, wbuf.collection().size());

    wbuf.close().get();
    VERIFY_IS_FALSE(wbuf.can_write());

    // verify putc after close
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), wbuf.putc(s[0]).get());
}

template<class StreamBufferType>
void streambuf_putn(StreamBufferType& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());

    std::basic_string<typename StreamBufferType::char_type> s;
    s.push_back((typename StreamBufferType::char_type)0);
    s.push_back((typename StreamBufferType::char_type)1);
    s.push_back((typename StreamBufferType::char_type)2);
    s.push_back((typename StreamBufferType::char_type)3);

    VERIFY_ARE_EQUAL(s.size(), wbuf.putn_nocopy(s.data(), s.size()).get());
    VERIFY_ARE_EQUAL(s.size() * 1, wbuf.in_avail());

    VERIFY_ARE_EQUAL(s.size(), wbuf.putn_nocopy(s.data(), s.size()).get());
    VERIFY_ARE_EQUAL(s.size() * 2, wbuf.in_avail());

    int count = 10;
    auto seg2 = [&count](size_t) { return (--count > 0); };
    auto seg1 = [&s, &wbuf, seg2]() { return wbuf.putn_nocopy(s.data(), s.size()).then(seg2); };
    pplx::details::_do_while(seg1).wait();
    VERIFY_ARE_EQUAL(s.size() * 12, wbuf.in_avail());

    wbuf.close().get();
    VERIFY_IS_FALSE(wbuf.can_write());

    // verify putn after close
    VERIFY_ARE_EQUAL(0, wbuf.putn_nocopy(s.data(), s.size()).get());
}

template<class CharType>
void streambuf_putn(concurrency::streams::rawptr_buffer<CharType>& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());

    typedef concurrency::streams::rawptr_buffer<CharType> StreamBufferType;

    std::basic_string<CharType> s;
    s.push_back((CharType)0);
    s.push_back((CharType)1);
    s.push_back((CharType)2);
    s.push_back((CharType)3);

    VERIFY_ARE_EQUAL(s.size(), wbuf.putn_nocopy(s.data(), s.size()).get());

    VERIFY_ARE_EQUAL(s.size(), wbuf.putn_nocopy(s.data(), s.size()).get());

    int count = 10;
    auto seg2 = [&count](size_t) { return (--count > 0); };
    auto seg1 = [&s, &wbuf, seg2]() { return wbuf.putn_nocopy(s.data(), s.size()).then(seg2); };
    pplx::details::_do_while(seg1).wait();

    wbuf.close().get();
    VERIFY_IS_FALSE(wbuf.can_write());

    // verify putn after close
    VERIFY_ARE_EQUAL(0, wbuf.putn_nocopy(s.data(), s.size()).get());
}

template<class CollectionType>
void streambuf_putn(concurrency::streams::container_buffer<CollectionType>& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());
    typedef concurrency::streams::container_buffer<CollectionType> StreamBufferType;
    typedef typename concurrency::streams::container_buffer<CollectionType>::char_type CharType;

    std::basic_string<CharType> s;
    s.push_back((CharType)0);
    s.push_back((CharType)1);
    s.push_back((CharType)2);
    s.push_back((CharType)3);

    VERIFY_ARE_EQUAL(s.size(), wbuf.putn_nocopy(s.data(), s.size()).get());

    VERIFY_ARE_EQUAL(s.size(), wbuf.putn_nocopy(s.data(), s.size()).get());

    int count = 10;
    auto seg2 = [&count](size_t) { return (--count > 0); };
    auto seg1 = [&s, &wbuf, seg2]() { return wbuf.putn_nocopy(s.data(), s.size()).then(seg2); };
    pplx::details::_do_while(seg1).wait();

    wbuf.close().get();
    VERIFY_IS_FALSE(wbuf.can_write());

    // verify putn_nocopy after close
    VERIFY_ARE_EQUAL(0, wbuf.putn_nocopy(s.data(), s.size()).get());
}

template<class StreamBufferType>
void streambuf_alloc_commit(StreamBufferType& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());

    VERIFY_ARE_EQUAL(0, wbuf.in_avail());

    size_t allocSize = 10;
    size_t commitSize = 2;

    for (size_t i = 0; i < allocSize / commitSize; i++)
    {
        // Allocate space for 10 chars
        auto data = wbuf.alloc(allocSize);

        VERIFY_IS_TRUE(data != nullptr);

        // commit 2
        wbuf.commit(commitSize);

        VERIFY_ARE_EQUAL((i + 1) * commitSize, wbuf.in_avail());
    }

    VERIFY_ARE_EQUAL(allocSize, wbuf.in_avail());

    wbuf.close().get();
    VERIFY_IS_FALSE(wbuf.can_write());
}

template<class CollectionType>
void streambuf_alloc_commit(concurrency::streams::container_buffer<CollectionType>& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());

    VERIFY_ARE_EQUAL(0, wbuf.collection().size());

    size_t allocSize = 10;
    size_t commitSize = 2;

    for (size_t i = 0; i < allocSize / commitSize; i++)
    {
        // Allocate space for 10 chars
        auto data = wbuf.alloc(allocSize);

        VERIFY_IS_TRUE(data != nullptr);

        // commit 2
        wbuf.commit(commitSize);

        VERIFY_IS_TRUE((i + 1) * commitSize <= wbuf.collection().size());
    }

    VERIFY_IS_TRUE(allocSize <= wbuf.collection().size());

    wbuf.close().get();
    VERIFY_IS_FALSE(wbuf.can_write());
}

template<class CharType>
void streambuf_alloc_commit(concurrency::streams::rawptr_buffer<CharType>& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());

    VERIFY_ARE_EQUAL(0, wbuf.block().size());

    size_t allocSize = 10;
    size_t commitSize = 2;

    for (size_t i = 0; i < allocSize / commitSize; i++)
    {
        // Allocate space for 10 chars
        auto data = wbuf.alloc(allocSize);

        VERIFY_IS_TRUE(data != nullptr);

        // commit 2
        wbuf.commit(commitSize);

        VERIFY_IS_TRUE((i + 1) * commitSize <= wbuf.block().size());
    }

    VERIFY_IS_TRUE(allocSize <= wbuf.block().size());

    VERIFY_IS_TRUE(wbuf.close().get());
    VERIFY_IS_FALSE(wbuf.can_write());
}
template<class StreamBufferType>
void streambuf_seek_write(StreamBufferType& wbuf)
{
    VERIFY_IS_TRUE(wbuf.can_write());
    VERIFY_IS_TRUE(wbuf.can_seek());

    auto beg = wbuf.seekoff(0, std::ios_base::beg, std::ios_base::out);
    auto cur = wbuf.seekoff(0, std::ios_base::cur, std::ios_base::out);

    // current should be at the begining
    VERIFY_ARE_EQUAL(beg, cur);

    auto end = wbuf.seekoff(0, std::ios_base::end, std::ios_base::out);
    VERIFY_ARE_EQUAL(end, wbuf.seekpos(end, std::ios_base::out));

    wbuf.close().get();
    VERIFY_IS_FALSE(wbuf.can_write());
    VERIFY_IS_FALSE(wbuf.can_seek());
}

template<class StreamBufferType>
void streambuf_getc(StreamBufferType& rbuf, typename StreamBufferType::char_type contents)
{
    VERIFY_IS_TRUE(rbuf.can_read());

    auto c = rbuf.getc().get();

    VERIFY_ARE_EQUAL(c, contents);

    // Calling getc again should return the same character (getc do not advance read head)
    VERIFY_ARE_EQUAL(c, rbuf.getc().get());

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());

    // getc should return eof after close
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), rbuf.getc().get());
}

template<class StreamBufferType>
void streambuf_sgetc(StreamBufferType& rbuf, typename StreamBufferType::char_type contents)
{
    VERIFY_IS_TRUE(rbuf.can_read());

    auto c = rbuf.sgetc();

    VERIFY_ARE_EQUAL(c, contents);

    // Calling getc again should return the same character (getc do not advance read head)
    VERIFY_ARE_EQUAL(c, rbuf.sgetc());

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());

    // sgetc should return eof after close
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), rbuf.sgetc());
}

template<class StreamBufferType>
void streambuf_bumpc(StreamBufferType& rbuf, const std::vector<typename StreamBufferType::char_type>& contents)
{
    VERIFY_IS_TRUE(rbuf.can_read());

    auto c = rbuf.bumpc().get();

    VERIFY_ARE_EQUAL(c, contents[0]);

    // Calling bumpc again should return the next character
    // Read till eof
    auto d = rbuf.bumpc().get();

    size_t index = 1;

    while (d != StreamBufferType::traits::eof())
    {
        VERIFY_ARE_EQUAL(d, contents[index]);
        d = rbuf.bumpc().get();
        index++;
    }

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());

    // operation should return eof after close
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), rbuf.bumpc().get());
}

template<class StreamBufferType>
void streambuf_sbumpc(StreamBufferType& rbuf, const std::vector<typename StreamBufferType::char_type>& contents)
{
    VERIFY_IS_TRUE(rbuf.can_read());

    auto c = rbuf.sbumpc();

    VERIFY_ARE_EQUAL(c, contents[0]);

    // Calling sbumpc again should return the next character
    // Read till eof
    auto d = rbuf.sbumpc();

    size_t index = 1;

    while (d != StreamBufferType::traits::eof())
    {
        VERIFY_ARE_EQUAL(d, contents[index]);
        d = rbuf.sbumpc();
        index++;
    }

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());

    // operation should return eof after close
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), rbuf.sbumpc());
}

template<class StreamBufferType>
void streambuf_nextc(StreamBufferType& rbuf, const std::vector<typename StreamBufferType::char_type>& contents)
{
    VERIFY_IS_TRUE(rbuf.can_read());

    auto c = rbuf.nextc().get();

    VERIFY_ARE_EQUAL(c, contents[1]);

    // Calling getc should return the same contents as before.
    VERIFY_ARE_EQUAL(c, rbuf.getc().get());

    size_t index = 1;

    while (c != StreamBufferType::traits::eof())
    {
        VERIFY_ARE_EQUAL(c, contents[index]);
        c = rbuf.nextc().get();
        index++;
    }

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());

    // operation should return eof after close
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), rbuf.nextc().get());
}

template<class StreamBufferType>
void streambuf_ungetc(StreamBufferType& rbuf, const std::vector<typename StreamBufferType::char_type>& contents)
{
    VERIFY_IS_TRUE(rbuf.can_read());

    // ungetc from the begining should return eof
    VERIFY_ARE_EQUAL(StreamBufferType::traits::eof(), rbuf.ungetc().get());

    VERIFY_ARE_EQUAL(contents[0], rbuf.bumpc().get());
    VERIFY_ARE_EQUAL(contents[1], rbuf.getc().get());

    auto c = rbuf.ungetc().get();

    // ungetc could be unsupported!
    if (c != StreamBufferType::traits::eof())
    {
        VERIFY_ARE_EQUAL(contents[0], c);
    }

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());
}

template<class StreamBufferType>
void streambuf_getn(StreamBufferType& rbuf, const std::vector<typename StreamBufferType::char_type>& contents)
{
    VERIFY_IS_TRUE(rbuf.can_read());
    VERIFY_IS_FALSE(rbuf.can_write());

    auto ptr = new typename StreamBufferType::char_type[contents.size()];
    VERIFY_ARE_EQUAL(contents.size(), rbuf.getn(ptr, contents.size()).get());

    // We shouldn't be able to read any more
    VERIFY_ARE_EQUAL(0, rbuf.getn(ptr, contents.size()).get());

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());

    // We shouldn't be able to read any more
    VERIFY_ARE_EQUAL(0, rbuf.getn(ptr, contents.size()).get());

    delete[] ptr;
}

template<class StreamBufferType>
void streambuf_acquire_release(StreamBufferType& rbuf, const std::vector<typename StreamBufferType::char_type>&)
{
    VERIFY_IS_TRUE(rbuf.can_read());

    typename StreamBufferType::char_type* ptr = nullptr;
    size_t size = 0;
    rbuf.acquire(ptr, size);

    if (ptr != nullptr)
    {
        VERIFY_IS_TRUE(size > 0);
        rbuf.release(ptr, size - 1);

        rbuf.acquire(ptr, size);
        VERIFY_IS_TRUE(size > 0);
        rbuf.release(ptr, 0);

        rbuf.acquire(ptr, size);
        VERIFY_IS_TRUE(size > 0);
        rbuf.release(ptr, size);
    }
    else
    {
        // This shouldn't crash
        rbuf.release(ptr, size);
    }

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());
}

template<class StreamBufferType>
void streambuf_seek_read(StreamBufferType& rbuf)
{
    VERIFY_IS_TRUE(rbuf.can_read());
    VERIFY_IS_TRUE(rbuf.can_seek());

    auto beg = rbuf.seekoff(0, std::ios_base::beg, std::ios_base::in);
    auto cur = rbuf.seekoff(0, std::ios_base::cur, std::ios_base::in);

    // current should be at the begining
    VERIFY_ARE_EQUAL(beg, cur);

    auto end = rbuf.seekoff(0, std::ios_base::end, std::ios_base::in);
    VERIFY_ARE_EQUAL(end, rbuf.seekpos(end, std::ios_base::in));

    rbuf.close().get();
    VERIFY_IS_FALSE(rbuf.can_read());
    VERIFY_IS_FALSE(rbuf.can_seek());
}

template<class StreamBufferType>
void streambuf_putn_getn(StreamBufferType& rwbuf)
{
    VERIFY_IS_TRUE(rwbuf.is_open());
    VERIFY_IS_TRUE(rwbuf.can_read());
    VERIFY_IS_TRUE(rwbuf.can_write());
    VERIFY_IS_FALSE(rwbuf.is_eof());
    std::basic_string<typename StreamBufferType::char_type> s;
    s.push_back((typename StreamBufferType::char_type)0);
    s.push_back((typename StreamBufferType::char_type)1);
    s.push_back((typename StreamBufferType::char_type)2);
    s.push_back((typename StreamBufferType::char_type)3);

    typename StreamBufferType::char_type ptr[4];

    auto readTask = pplx::create_task([&s, &ptr, &rwbuf]() {
        VERIFY_ARE_EQUAL(rwbuf.getn(ptr, 4).get(), 4);

        for (size_t i = 0; i < 4; i++)
        {
            VERIFY_ARE_EQUAL(s[i], ptr[i]);
        }
        VERIFY_IS_FALSE(rwbuf.is_eof());
        VERIFY_ARE_EQUAL(rwbuf.getc().get(), std::char_traits<char>::eof());
        VERIFY_IS_TRUE(rwbuf.is_eof());
    });

    auto writeTask = pplx::create_task([&s, &rwbuf]() {
        VERIFY_ARE_EQUAL(rwbuf.putn_nocopy(s.data(), s.size()).get(), s.size());
        rwbuf.close(std::ios_base::out);
    });

    writeTask.wait();
    readTask.wait();

    rwbuf.close().get();
}

template<class StreamBufferType>
void streambuf_acquire_alloc(StreamBufferType& rwbuf)
{
    VERIFY_IS_TRUE(rwbuf.is_open());
    VERIFY_IS_TRUE(rwbuf.can_read());
    VERIFY_IS_TRUE(rwbuf.can_write());

    {
        // There should be nothing to read
        typename StreamBufferType::char_type* ptr = nullptr;
        size_t count = 0;
        rwbuf.acquire(ptr, count);
        VERIFY_ARE_EQUAL(count, 0);
        rwbuf.release(ptr, count);
    }

    auto writeTask = pplx::create_task([&rwbuf]() {
        auto ptr = rwbuf.alloc(8);
        VERIFY_IS_TRUE(ptr != nullptr);
        rwbuf.commit(4);
    });

    typename StreamBufferType::char_type* ptr = nullptr;
    auto readTask = pplx::create_task([&rwbuf, &ptr, writeTask]() {
        size_t count = 0;

        int repeat = 10;
        while ((count == 0) && (repeat-- > 0))
        {
            rwbuf.acquire(ptr, count);
        }

        if (count == 0)
        {
            writeTask.wait();
        }

        rwbuf.acquire(ptr, count);
        VERIFY_ARE_EQUAL(count, 4);
        rwbuf.release(ptr, count);
    });

    writeTask.wait();
    readTask.wait();

    rwbuf.close().get();
}

template<class StreamBufferType>
void streambuf_close(StreamBufferType& rwbuf)
{
    VERIFY_IS_TRUE(rwbuf.is_open());

    bool can_rd = rwbuf.can_read();
    bool can_wr = rwbuf.can_write();

    if (can_wr)
    {
        // Close the write head
        rwbuf.close(std::ios_base::out).get();
        VERIFY_IS_FALSE(rwbuf.can_write());

        if (can_rd)
        {
            VERIFY_IS_FALSE(rwbuf.can_write());
            VERIFY_IS_TRUE(rwbuf.can_read());

            // The buffer should still be open for read
            VERIFY_IS_TRUE(rwbuf.is_open());

            // Closing the write head again should not throw
            rwbuf.close(std::ios_base::out).wait();

            // The read head should still be open
            VERIFY_IS_TRUE(rwbuf.can_read());
        }
    }

    if (can_rd)
    {
        // Close the read head
        rwbuf.close(std::ios_base::in).get();
        VERIFY_IS_FALSE(rwbuf.can_read());

        // Closing the read head again should not throw
        rwbuf.close(std::ios_base::in).wait();
    }

    // The buffer should now be closed
    VERIFY_IS_FALSE(rwbuf.is_open());
}

template<class StreamBufferType>
void streambuf_close_read_with_pending_read(StreamBufferType& rwbuf)
{
    VERIFY_IS_TRUE(rwbuf.is_open());
    VERIFY_IS_TRUE(rwbuf.can_read());
    VERIFY_IS_TRUE(rwbuf.can_write());

    // Write 4 characters
    std::basic_string<typename StreamBufferType::char_type> s;
    s.push_back((typename StreamBufferType::char_type)0);
    s.push_back((typename StreamBufferType::char_type)1);
    s.push_back((typename StreamBufferType::char_type)2);
    s.push_back((typename StreamBufferType::char_type)3);

    VERIFY_ARE_EQUAL(s.size(), rwbuf.putn_nocopy(s.data(), s.size()).get());
    VERIFY_ARE_EQUAL(s.size() * 1, rwbuf.in_avail());

    // Try to read 8 characters - this should block
    typename StreamBufferType::char_type data[8];
    auto readTask = rwbuf.getn(data, 8);

    // Close the write head
    rwbuf.close(std::ios_base::out).get();
    VERIFY_IS_FALSE(rwbuf.can_write());

    // The buffer should still be open for read
    VERIFY_IS_TRUE(rwbuf.is_open());

    // The read head should still be open
    VERIFY_IS_TRUE(rwbuf.can_read());

    // Closing the write head should trigger the completion of the read request
    VERIFY_ARE_EQUAL(4, readTask.get());

    // Close the read head
    rwbuf.close(std::ios_base::in).get();
    VERIFY_IS_FALSE(rwbuf.can_read());

    // The buffer should now be closed
    VERIFY_IS_FALSE(rwbuf.is_open());
}

template<class StreamBufferType>
void streambuf_close_write_with_pending_read(StreamBufferType& rwbuf)
{
    VERIFY_IS_TRUE(rwbuf.is_open());
    VERIFY_IS_TRUE(rwbuf.can_read());
    VERIFY_IS_TRUE(rwbuf.can_write());

    // Write 4 characters
    std::basic_string<typename StreamBufferType::char_type> s;
    s.push_back((typename StreamBufferType::char_type)0);
    s.push_back((typename StreamBufferType::char_type)1);
    s.push_back((typename StreamBufferType::char_type)2);
    s.push_back((typename StreamBufferType::char_type)3);

    VERIFY_ARE_EQUAL(s.size(), rwbuf.putn_nocopy(s.data(), s.size()).get());
    VERIFY_ARE_EQUAL(s.size() * 1, rwbuf.in_avail());

    // Try to read 8 characters - this should block
    typename StreamBufferType::char_type data[8];
    auto readTask = rwbuf.getn(data, 8);

    // Close the read head
    rwbuf.close(std::ios_base::in).get();
    VERIFY_IS_FALSE(rwbuf.can_read());

    // The read task should not be completed
    VERIFY_IS_FALSE(readTask.is_done());

    // Close the write head
    rwbuf.close(std::ios_base::out).get();
    VERIFY_IS_FALSE(rwbuf.can_write());

    // Closing the write head should trigger the completion of the read request
    VERIFY_ARE_EQUAL(4, readTask.get());

    // The buffer should now be closed
    VERIFY_IS_FALSE(rwbuf.is_open());
}

template<class StreamBufferType>
void streambuf_close_parallel(StreamBufferType& rwbuf)
{
    VERIFY_IS_TRUE(rwbuf.is_open());
    VERIFY_IS_TRUE(rwbuf.can_read());
    VERIFY_IS_TRUE(rwbuf.can_write());

    // Close the read and write head in parallel
    auto closeReadTask = pplx::create_task([&rwbuf]() {
        VERIFY_IS_TRUE(rwbuf.can_read());

        // Close the read head
        rwbuf.close(std::ios_base::in).get();
        VERIFY_IS_FALSE(rwbuf.can_read());

        // Closing the read head again should not throw
        rwbuf.close(std::ios_base::in).wait();
    });

    auto closeWriteTask = pplx::create_task([&rwbuf]() {
        VERIFY_IS_TRUE(rwbuf.can_write());

        // Close the write head
        rwbuf.close(std::ios_base::out).get();
        VERIFY_IS_FALSE(rwbuf.can_write());

        // Closing the write head again should not throw
        rwbuf.close(std::ios_base::out).wait();
    });

    closeReadTask.wait();
    closeWriteTask.wait();

    // The buffer should now be closed
    VERIFY_IS_FALSE(rwbuf.is_open());
}

streams::producer_consumer_buffer<uint8_t> create_producer_consumer_buffer_with_data(const std::vector<uint8_t>& s)
{
    streams::producer_consumer_buffer<uint8_t> buf;
    VERIFY_ARE_EQUAL(buf.putn_nocopy(s.data(), s.size()).get(), s.size());
    buf.close(std::ios_base::out).get();
    return buf;
}

SUITE(memstream_tests)
{
    TEST(string_buffer_putc)
    {
        stringstreambuf buf;
        streambuf_putc(buf);
    }

    TEST(charptr_buffer_putc_fail)
    {
        char chars[26];
        rawptr_buffer<char> buf(chars, 26);
        VERIFY_ARE_EQUAL(buf.putn_nocopy("abcdefghijklmnopqrstuvwxyz", 26).get(), 26);
        VERIFY_ARE_EQUAL(buf.putc('a').get(), std::char_traits<char>::eof());
    }

    TEST(wstring_buffer_putc)
    {
        wstringstreambuf buf;
        streambuf_putc(buf);
    }

    TEST(string_buffer_putn)
    {
        stringstreambuf buf;
        streambuf_putn(buf);
    }
    TEST(wstring_buffer_putn)
    {
        wstringstreambuf buf;
        streambuf_putn(buf);
    }
    TEST(charptr_buffer_putn)
    {
        {
            char chars[128];
            rawptr_buffer<char> buf(chars, sizeof(chars));
            streambuf_putn(buf);
        }
        {
            uint8_t chars[128];
            rawptr_buffer<uint8_t> buf(chars, sizeof(chars));
            streambuf_putn(buf);
        }
        {
            utf16char chars[128];
            rawptr_buffer<utf16char> buf(chars, sizeof(chars) / sizeof(utf16char));
            streambuf_putn(buf);
        }
    }
    TEST(charptr_buffer_putn_fail)
    {
        {
            char chars[128];
            rawptr_buffer<char> buf(chars, 12);
            VERIFY_THROWS(buf.putn_nocopy("abcdefghijklmnopqrstuvwxyz", 26).get(), std::runtime_error);
        }
    }

    TEST(bytevec_buffer_putn)
    {
        {
            container_buffer<std::vector<uint8_t>> buf;
            streambuf_putn(buf);
        }
        {
            container_buffer<std::vector<char>> buf;
            streambuf_putn(buf);
        }
        {
            container_buffer<std::vector<utf16char>> buf;
            streambuf_putn(buf);
        }
    }
    TEST(mem_buffer_putn)
    {
        {
            streams::producer_consumer_buffer<char> buf;
            streambuf_putn(buf);
        }

        {
            streams::producer_consumer_buffer<uint8_t> buf;
            streambuf_putn(buf);
        }

        {
            streams::producer_consumer_buffer<utf16char> buf;
            streambuf_putn(buf);
        }
    }

    TEST(string_buffer_alloc_commit)
    {
        stringstreambuf buf;
        streambuf_alloc_commit(buf);
    }

    TEST(wstring_buffer_alloc_commit)
    {
        wstringstreambuf buf;
        streambuf_alloc_commit(buf);
    }

    TEST(mem_buffer_alloc_commit)
    {
        {
            streams::producer_consumer_buffer<char> buf;
            streambuf_alloc_commit(buf);
        }

        {
            streams::producer_consumer_buffer<uint8_t> buf;
            streambuf_alloc_commit(buf);
        }

        {
            streams::producer_consumer_buffer<utf16char> buf;
            streambuf_alloc_commit(buf);
        }
    }

    TEST(string_buffer_seek_write)
    {
        stringstreambuf buf;
        streambuf_seek_write(buf);
    }
    TEST(wstring_buffer_seek_write)
    {
        wstringstreambuf buf;
        streambuf_seek_write(buf);
    }
    TEST(charptr_buffer_seek_write)
    {
        {
            char chars[128];
            rawptr_buffer<char> buf(chars, sizeof(chars));
            streambuf_seek_write(buf);
        }
        {
            uint8_t chars[128];
            rawptr_buffer<uint8_t> buf(chars, sizeof(chars));
            streambuf_seek_write(buf);
        }
        {
            utf16char chars[128];
            rawptr_buffer<utf16char> buf(chars, sizeof(chars) / sizeof(utf16char));
            streambuf_seek_write(buf);
        }
    }
    TEST(bytevec_buffer_seek_write)
    {
        {
            container_buffer<std::vector<uint8_t>> buf;
            streambuf_seek_write(buf);
        }
        {
            container_buffer<std::vector<char>> buf;
            streambuf_seek_write(buf);
        }
        {
            container_buffer<std::vector<utf16char>> buf;
            streambuf_seek_write(buf);
        }
    }
    TEST(mem_buffer_seek_write)
    {
        streams::producer_consumer_buffer<char> buf;
        VERIFY_IS_FALSE(buf.can_seek());
    }

    TEST(string_buffer_getc)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_getc(buf, v[0]);
    }
    TEST(wstring_buffer_getc)
    {
        utility::string_t s(U("Hello World"));
        std::vector<utility::char_t> v(std::begin(s), std::end(s));
        streams::wstringstreambuf buf(s);
        streambuf_getc(buf, v[0]);
    }
    TEST(charptr_buffer_getc)
    {
        {
            char chars[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
            rawptr_buffer<char> buf(chars, sizeof(chars), std::ios::in);
            streambuf_getc(buf, chars[0]);
        }
        {
            uint8_t chars[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
            rawptr_buffer<uint8_t> buf(chars, sizeof(chars), std::ios::in);
            streambuf_getc(buf, chars[0]);
        }
        {
            utf16char chars[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
            rawptr_buffer<utf16char> buf(chars, sizeof(chars) / sizeof(utf16char), std::ios::in);
            streambuf_getc(buf, chars[0]);
        }
    }
    TEST(bytevec_buffer_getc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        container_buffer<std::vector<uint8_t>> buf(s);
        streambuf_getc(buf, s[0]);
    }
    TEST(mem_buffer_getc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        streams::producer_consumer_buffer<uint8_t> buf = create_producer_consumer_buffer_with_data(s);
        streambuf_getc(buf, s[0]);
    }

    TEST(string_buffer_sgetc)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_sgetc(buf, v[0]);
    }
    TEST(wstring_buffer_sgetc)
    {
        utility::string_t s(U("Hello World"));
        std::vector<utility::char_t> v(std::begin(s), std::end(s));
        streams::wstringstreambuf buf(s);
        streambuf_sgetc(buf, v[0]);
    }
    TEST(charptr_buffer_sgetc)
    {
        char chars[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        rawptr_buffer<char> buf(chars, sizeof(chars), std::ios::in);
        streambuf_sgetc(buf, chars[0]);
    }
    TEST(bytevec_buffer_sgetc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        container_buffer<std::vector<uint8_t>> buf(s);
        streambuf_sgetc(buf, s[0]);
    }
    TEST(mem_buffer_sgetc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        streams::producer_consumer_buffer<uint8_t> buf = create_producer_consumer_buffer_with_data(s);
        streambuf_sgetc(buf, s[0]);
    }

    TEST(string_buffer_bumpc)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_bumpc(buf, v);
    }
    TEST(wstring_buffer_bumpc)
    {
        utility::string_t s(U("Hello World"));
        std::vector<utility::char_t> v(std::begin(s), std::end(s));
        streams::wstringstreambuf buf(s);
        streambuf_bumpc(buf, v);
    }
    TEST(charptr_buffer_bumpc)
    {
        uint8_t chars[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(chars), std::end(chars));
        rawptr_buffer<uint8_t> buf(chars, sizeof(chars), std::ios::in);
        streambuf_bumpc(buf, s);
    }
    TEST(bytevec_buffer_bumpc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        container_buffer<std::vector<uint8_t>> buf(s);
        streambuf_bumpc(buf, s);
    }

    TEST(mem_buffer_bumpc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        streams::producer_consumer_buffer<uint8_t> buf = create_producer_consumer_buffer_with_data(s);
        streambuf_bumpc(buf, s);
    }

    TEST(string_buffer_sbumpc)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_sbumpc(buf, v);
    }
    TEST(wstring_buffer_sbumpc)
    {
        utility::string_t s(U("Hello World"));
        std::vector<utility::char_t> v(std::begin(s), std::end(s));
        streams::wstringstreambuf buf(s);
        streambuf_sbumpc(buf, v);
    }
    TEST(charptr_buffer_sbumpc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        rawptr_buffer<uint8_t> buf(data, sizeof(data), std::ios::in);
        streambuf_sbumpc(buf, s);
    }
    TEST(bytevec_buffer_sbumpc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        container_buffer<std::vector<uint8_t>> buf(s);
        streambuf_sbumpc(buf, s);
    }

    TEST(mem_buffer_sbumpc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        streams::producer_consumer_buffer<uint8_t> buf = create_producer_consumer_buffer_with_data(s);
        streambuf_sbumpc(buf, s);
    }
    TEST(string_buffer_nextc)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_nextc(buf, v);
    }
    TEST(wstring_buffer_nextc)
    {
        utility::string_t s(U("Hello World"));
        std::vector<utility::char_t> v(std::begin(s), std::end(s));
        streams::wstringstreambuf buf(s);
        streambuf_nextc(buf, v);
    }
    TEST(charptr_buffer_nextc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        rawptr_buffer<uint8_t> buf(data, sizeof(data), std::ios::in);
        streambuf_nextc(buf, s);
    }
    TEST(bytevec_buffer_nextc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        container_buffer<std::vector<uint8_t>> buf(s);
        streambuf_nextc(buf, s);
    }
    TEST(mem_buffer_nextc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        streams::producer_consumer_buffer<uint8_t> buf = create_producer_consumer_buffer_with_data(s);
        streambuf_nextc(buf, s);
    }

    TEST(string_buffer_ungetc)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_ungetc(buf, v);
    }
    TEST(wstring_buffer_ungetc)
    {
        utility::string_t s(U("Hello World"));
        std::vector<utility::char_t> v(std::begin(s), std::end(s));
        streams::wstringstreambuf buf(s);
        streambuf_ungetc(buf, v);
    }
    TEST(charptr_buffer_ungetc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        rawptr_buffer<uint8_t> buf(data, sizeof(data), std::ios::in);
        streambuf_ungetc(buf, s);
    }
    TEST(bytevec_buffer_ungetc)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        container_buffer<std::vector<uint8_t>> buf(s);
        streambuf_ungetc(buf, s);
    }

    TEST(string_buffer_getn)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_getn(buf, v);
    }
    TEST(wstring_buffer_getn)
    {
        utility::string_t s(U("Hello World"));
        std::vector<utility::char_t> v(std::begin(s), std::end(s));
        streams::wstringstreambuf buf(s);
        streambuf_getn(buf, v);
    }
    TEST(charptr_buffer_getn)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        rawptr_buffer<uint8_t> buf(data, sizeof(data), std::ios::in);
        streambuf_getn(buf, s);
    }
    TEST(bytevec_buffer_getn)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        container_buffer<std::vector<uint8_t>> buf(s);
        streambuf_getn(buf, s);
    }
    TEST(mem_buffer_getn)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        streams::producer_consumer_buffer<uint8_t> buf = create_producer_consumer_buffer_with_data(s);
        streambuf_getn(buf, s);
    }

    TEST(string_buffer_acquire_release)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_acquire_release(buf, v);
    }
    TEST(wstring_buffer_acquire_release)
    {
        utility::string_t s(U("Hello World"));
        std::vector<utility::char_t> v(std::begin(s), std::end(s));
        streams::wstringstreambuf buf(s);
        streambuf_acquire_release(buf, v);
    }
    TEST(charptr_buffer_acquire_release)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        rawptr_buffer<uint8_t> buf(data, sizeof(data), std::ios::in);
        streambuf_acquire_release(buf, s);
    }
    TEST(bytevec_buffer_acquire_release)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        container_buffer<std::vector<uint8_t>> buf(s);
        streambuf_acquire_release(buf, s);
    }
    TEST(mem_buffer_acquire_release)
    {
        uint8_t data[] = {'H', 'e', 'l', 'l', 'o', ' ', 'W', 'o', 'r', 'l', 'd'};
        std::vector<uint8_t> s(std::begin(data), std::end(data));
        streams::producer_consumer_buffer<uint8_t> buf = create_producer_consumer_buffer_with_data(s);
        streambuf_acquire_release(buf, s);
    }
    TEST(string_buffer_seek_read)
    {
        std::string s("Hello World");
        std::vector<char> v(std::begin(s), std::end(s));
        streams::stringstreambuf buf(s);
        streambuf_seek_read(buf);
    }

    TEST(mem_buffer_putn_getn)
    {
        streams::producer_consumer_buffer<uint8_t> buf;
        streambuf_putn_getn(buf);
    }

    TEST(mem_buffer_acquire_alloc)
    {
        streams::producer_consumer_buffer<uint8_t> buf;
        streambuf_acquire_alloc(buf);
    }

    TEST(string_buffer_close)
    {
        streams::stringstreambuf buf;
        streambuf_close(buf);
    }
    TEST(wstring_buffer_close)
    {
        streams::wstringstreambuf buf;
        streambuf_close(buf);
    }
    TEST(bytevec_buffer_close)
    {
        container_buffer<std::vector<uint8_t>> buf;
        streambuf_close(buf);
    }
    TEST(mem_buffer_close)
    {
        streams::producer_consumer_buffer<uint8_t> buf;
        streambuf_close(buf);
    }

    TEST(mem_buffer_close_read_with_pending_read)
    {
        streams::producer_consumer_buffer<uint8_t> buf;
        streambuf_close_read_with_pending_read(buf);
    }

    TEST(mem_buffer_close_write_with_pending_read)
    {
        streams::producer_consumer_buffer<uint8_t> buf;
        streambuf_close_write_with_pending_read(buf);
    }

    TEST(mem_buffer_close_parallel)
    {
        streams::producer_consumer_buffer<uint8_t> buf;
        streambuf_close_parallel(buf);
    }

    TEST(mem_buffer_close_destroy)
    {
        std::vector<pplx::task<void>> taskVector;

        for (int i = 0; i < 1000; i++)
        {
            streams::producer_consumer_buffer<uint8_t> buf;
            taskVector.push_back(buf.close());
        }

        pplx::when_all(std::begin(taskVector), std::end(taskVector)).wait();
    }

    TEST(string_buffer_ctor)
    {
        std::string src("abcdef ghij");
        auto instream = streams::stringstream::open_istream(src);

        streams::stringstreambuf sbuf;
        auto outstream = sbuf.create_ostream();

        for (;;)
        {
            const int count = 4;
            char temp[count];
            streams::rawptr_buffer<char> buf1(temp, count);
            streams::rawptr_buffer<char> buf2(temp, count, std::ios::in);
            auto size = instream.read(buf1, count).get();
            VERIFY_IS_TRUE(size <= count);
            VERIFY_ARE_EQUAL(size, outstream.write(buf2, size).get());

            if (size != count) break;
        }

        auto& dest = sbuf.collection();
        VERIFY_ARE_EQUAL(src, dest);
    }

    TEST(vec_buffer_ctor)
    {
        std::string srcstr("abcdef ghij");
        std::vector<uint8_t> src(begin(srcstr), end(srcstr));
        auto instream = streams::bytestream::open_istream(src);

        container_buffer<std::vector<uint8_t>> sbuf;
        auto outstream = sbuf.create_ostream();

        for (;;)
        {
            const int count = 4;
            uint8_t temp[count];
            streams::rawptr_buffer<uint8_t> buf1(temp, count);
            streams::rawptr_buffer<uint8_t> buf2(temp, count, std::ios::in);
            auto size = instream.read(buf1, count).get();
            VERIFY_IS_TRUE(size <= count);
            VERIFY_ARE_EQUAL(size, outstream.write(buf2, size).get());

            if (size != count) break;
        }

        auto& dest = sbuf.collection();
        VERIFY_ARE_EQUAL(src, dest);
    }

    TEST(charptr_buffer_ctor_1)
    {
        char chars[] = {'a', 'b', 'c', 'd', 'e', 'f', ' ', 'g', 'h', 'i', 'j'};
        auto instream = streams::rawptr_stream<char>::open_istream(chars, sizeof(chars));

        stringstreambuf sbuf;
        auto outstream = sbuf.create_ostream();

        for (;;)
        {
            const int count = 4;
            char temp[count];
            streams::rawptr_buffer<char> buf1(temp, count);
            streams::rawptr_buffer<char> buf2(temp, count, std::ios::in);
            auto size = instream.read(buf1, count).get();
            VERIFY_IS_TRUE(size <= count);
            VERIFY_ARE_EQUAL(size, outstream.write(buf2, size).get());

            if (size != count) break;
        }

        auto& dest = sbuf.collection();
        VERIFY_ARE_EQUAL(memcmp(chars, &(dest)[0], sizeof(chars)), 0);
    }

    TEST(charptr_buffer_ctor_2)
    {
        char chars[] = {'a', 'b', 'c', 'd', 'e', 'f', ' ', 'g', 'h', 'i', 'j'};
        auto instream = streams::rawptr_stream<char>::open_istream(chars, sizeof(chars));

        stringstreambuf sbuf;
        auto outstream = sbuf.create_ostream();

        for (;;)
        {
            const int count = 4;
            char temp[count];
            streams::rawptr_buffer<char> buf1(temp, count);
            streams::rawptr_buffer<char> buf2(temp, count, std::ios::in);
            auto size = instream.read(buf1, count).get();
            VERIFY_IS_TRUE(size <= count);
            VERIFY_ARE_EQUAL(size, outstream.write(buf2, size).get());

            if (size != count) break;
        }

        auto& dest = sbuf.collection();
        VERIFY_ARE_EQUAL(memcmp(chars, &(dest)[0], sizeof(chars)), 0);
    }

    TEST(charptr_buffer_ctor_3)
    {
        char chars[128];
        memset(chars, 0, sizeof(chars));

        rawptr_buffer<char> buf(chars, sizeof(chars));

        auto outstream = buf.create_ostream();

        auto t1 = outstream.print("Hello ");
        auto t2 = outstream.print(10);
        auto t3 = outstream.print(" Again!");
        (t1 && t2 && t3).wait();

        std::string result(chars);

        VERIFY_ARE_EQUAL(result, "Hello 10 Again!");
    }

    TEST(validate_stream_mode)
    {
        VERIFY_THROWS(concurrency::streams::container_buffer<std::vector<char>>(std::ios::in | std::ios::out),
                      std::invalid_argument);
        std::vector<char> vc;
        VERIFY_THROWS(concurrency::streams::container_buffer<std::vector<char>>(vc, std::ios::in | std::ios::out),
                      std::invalid_argument);
    }

    TEST(write_stream_test_1)
    {
        char chars[128];
        memset(chars, 0, sizeof(chars));

        auto stream = streams::rawptr_stream<char>::open_ostream(chars, sizeof(chars));

        std::vector<uint8_t> vect;

        for (char ch = 'a'; ch <= 'z'; ch++)
        {
            vect.push_back(ch);
        }

        size_t vsz = vect.size();

        concurrency::streams::container_stream<std::vector<uint8_t>>::buffer_type txtbuf(std::move(vect),
                                                                                         std::ios_base::in);

        VERIFY_ARE_EQUAL(stream.write(txtbuf, vsz).get(), vsz);
        VERIFY_ARE_EQUAL(strcmp(chars, "abcdefghijklmnopqrstuvwxyz"), 0);

        auto close = stream.close();

        VERIFY_IS_TRUE(close.is_done());
    }

    TEST(mem_buffer_large_data)
    {
        // stream large amounts of data
        // If the stream stores all the data then we will run out of VA space!
        streams::producer_consumer_buffer<char> membuf;

        const size_t size = 4 * 1024 * 1024; // 4 MB
        char* ptr = new char[size];

        // stream 4 GB
        for (size_t i = 0; i < 1024; i++)
        {
            // Fill some random positions in the buffer
            ptr[i + 0] = 'a';
            ptr[i + 100] = 'b';

            VERIFY_ARE_EQUAL(size, membuf.putn_nocopy(ptr, size).get());

            // overwrite the values in ptr
            ptr[i + 0] = 'c';
            ptr[i + 100] = 'd';

            VERIFY_ARE_EQUAL(size, membuf.getn(ptr, size).get());

            VERIFY_ARE_EQUAL(ptr[i + 0], 'a');
            VERIFY_ARE_EQUAL(ptr[i + 100], 'b');
        }

        delete[] ptr;
    }

#ifdef _WIN32

    class ISequentialStream_bridge
#if defined(__cplusplus_winrt)
        : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,
                                              ISequentialStream>
#endif
    {
    public:
        ISequentialStream_bridge(streambuf<char> buf) : m_buffer(buf) {}

        // ISequentialStream implementation
        virtual HRESULT STDMETHODCALLTYPE Read(void* pv, ULONG cb, ULONG* pcbRead)
        {
            size_t count = m_buffer.getn((char*)pv, (size_t)cb).get();
            if (pcbRead != nullptr) *pcbRead = (ULONG)count;
            return S_OK;
        }

        virtual HRESULT STDMETHODCALLTYPE Write(const void* pv, ULONG cb, ULONG* pcbWritten)
        {
            size_t count = m_buffer.putn_nocopy((const char*)pv, (size_t)cb).get();
            if (pcbWritten != nullptr) *pcbWritten = (ULONG)count;
            return S_OK;
        }

    private:
        streambuf<char> m_buffer;
    };

    template<typename _StreamBufferType>
    void IStreamTest1()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text = "This is a test";
        size_t len = text.size();

        ULONG pcbWritten = 0;

        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text[0], (ULONG)text.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(text.size(), pcbWritten);

        text = " - but this is not";
        len += text.size();
        pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text[0], (ULONG)text.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(text.size(), pcbWritten);

        char buf[128];
        memset(buf, 0, sizeof(buf));

        rbuf.getn((char*)buf, len).wait();

        VERIFY_ARE_EQUAL(0, strcmp("This is a test - but this is not", buf));

        rbuf.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(membuf_IStreamTest1) { IStreamTest1<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest2()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text = "This is a test";
        size_t len = text.size();

        ULONG pcbWritten = 0;

        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text[0], (ULONG)text.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(text.size(), pcbWritten);

        text = " - but this is not";
        len += text.size();
        pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text[0], (ULONG)text.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(text.size(), pcbWritten);

        char buf[128];
        memset(buf, 0, sizeof(buf));

        rbuf.getn((char*)buf, len).wait();

        VERIFY_ARE_EQUAL(0, strcmp("This is a test - but this is not", buf));

        rbuf.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(membuf_IStreamTest2) { IStreamTest2<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest3()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text1 = "This is a test";
        size_t len1 = text1.size();
        std::string text2 = " - but this is not";
        size_t len2 = text2.size();

        char buf[128];
        memset(buf, 0, sizeof(buf));

        // The read happens before the write.

        auto read = rbuf.getn((char*)buf, len1 + len2);

        ULONG pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text1[0], (ULONG)text1.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(len1, pcbWritten);
        pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text2[0], (ULONG)text2.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(len2, pcbWritten);

        read.wait();

        // We may or may not read data from both writes here. It depends on the
        // stream in use. Both are correct behaviors.
        if (read.get() == len1 + len2)
            VERIFY_ARE_EQUAL(0, strcmp("This is a test - but this is not", (char*)buf));
        else
            VERIFY_ARE_EQUAL(0, strcmp("This is a test", (char*)buf));

        rbuf.close().get();
    }

    TEST(membuf_IStreamTest3) { IStreamTest3<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest4()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text1 = "This is a test";
        size_t len1 = text1.size();
        std::string text2 = " - but this is not";
        size_t len2 = text2.size();

        char buf1[128];
        memset(buf1, 0, sizeof(buf1));
        char buf2[128];
        memset(buf2, 0, sizeof(buf2));

        // The read happens before the write.

        auto read1 = rbuf.getn(buf1, 8);
        auto read2 = rbuf.getn(buf2, 12);

        ULONG pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text1[0], (ULONG)text1.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(len1, pcbWritten);
        pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text2[0], (ULONG)text2.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(len2, pcbWritten);

        VERIFY_ARE_EQUAL(8u, read1.get());
        // Different results depending on stream implementation. Both correct.
        VERIFY_IS_TRUE(read2.get() == 12u || read2.get() == 6u);

        VERIFY_ARE_EQUAL(0, strcmp("This is ", (char*)buf1));
        if (read2.get() == 12u)
            VERIFY_ARE_EQUAL(0, strcmp("a test - but", (char*)buf2));
        else
            VERIFY_ARE_EQUAL(0, strcmp("a test", (char*)buf2));

        rbuf.close().get();
    }

    TEST(membuf_IStreamTest4) { IStreamTest4<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest5()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text1 = "This is a test";
        size_t len1 = text1.size();

        char buf1[128];
        memset(buf1, 0, sizeof(buf1));

        // The read happens before the write.

        auto read1 = rbuf.getn(buf1, 28);

        ULONG pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text1[0], (ULONG)text1.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(len1, pcbWritten);

        // We close the stream buffer before enough bytes have been written.

        rbuf.close().get();

        VERIFY_ARE_EQUAL(len1, read1.get());
        VERIFY_ARE_EQUAL(len1, strlen((char*)buf1));
        VERIFY_ARE_EQUAL(0, strcmp("This is a test", (char*)buf1));
    }

    TEST(membuf_IStreamTest5) { IStreamTest5<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest6()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text1 = "abcdefghijklmnopqrstuvwxyz";
        size_t len1 = text1.size();

        ULONG pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text1[0], (ULONG)text1.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(len1, pcbWritten);

        bool validated = true;
        for (int expected = 'a'; expected <= 'z'; expected++)
        {
            validated = validated && (expected == rbuf.bumpc().get());
        }

        VERIFY_IS_TRUE(validated);
        rbuf.close().get();
    }

    TEST(membuf_IStreamTest6) { IStreamTest6<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest7()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::vector<task<int>> reads;

        for (int i = 0; i < 26; i++)
        {
            reads.push_back(rbuf.bumpc());
        }

        std::string text1 = "abcdefghijklmnopqrstuvwxyz";
        size_t len1 = text1.size();

        ULONG pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text1[0], (ULONG)text1.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(len1, pcbWritten);

        bool validated = true;
        for (int i = 0; i < 26; i++)
        {
            int expected = 'a' + i;
            validated = validated && (expected == reads[i].get());
        }

        VERIFY_IS_TRUE(validated);
        rbuf.close().get();
    }

    TEST(membuf_IStreamTest7) { IStreamTest7<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest8()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text1 = "This is a test";
        size_t len1 = text1.size();

        char buf1[128];
        memset(buf1, 0, sizeof(buf1));

        // The read happens before the write.

        auto read1 = rbuf.getn(buf1, 28);
        auto read2 = rbuf.getn(buf1, 8);

        ULONG pcbWritten = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Write((const void*)&text1[0], (ULONG)text1.size(), &pcbWritten));
        VERIFY_ARE_EQUAL(len1, pcbWritten);

        // We close the stream buffer before enough bytes have been written.
        // Make sure that the first read results in fewer bytes than requested
        // and that the second read returns -1.

        rbuf.close(std::ios_base::out).get();

        VERIFY_ARE_EQUAL(len1, read1.get());
        VERIFY_ARE_EQUAL(-0, (int)read2.get());
        VERIFY_ARE_EQUAL(len1, strlen((char*)buf1));
        VERIFY_ARE_EQUAL(0, strcmp("This is a test", (char*)buf1));
    }

    TEST(membuf_IStreamTest8) { IStreamTest8<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest9()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        VERIFY_ARE_EQUAL((int)'a', rbuf.putc('a').get());
        VERIFY_ARE_EQUAL((int)'n', rbuf.putc('n').get());
        VERIFY_ARE_EQUAL((int)'q', rbuf.putc('q').get());
        VERIFY_ARE_EQUAL((int)'s', rbuf.putc('s').get());

        VERIFY_ARE_EQUAL(4u, rbuf.in_avail());

        std::string chars(32, '\0');
        ULONG pcbRead = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Read(&chars[0], 4, &pcbRead));
        VERIFY_ARE_EQUAL(4u, pcbRead);

        VERIFY_ARE_EQUAL("anqs", chars.c_str());

        rbuf.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(membuf_IStreamTest9) { IStreamTest9<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest10()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text(128, '\0');
        strcpy_s(&text[0], 128, "This is a test");
        size_t len1 = strlen(&text[0]);

        VERIFY_ARE_EQUAL(len1, rbuf.putn_nocopy(&text[0], len1).get());

        strcpy_s(&text[0], 128, " - but this is not");
        size_t len2 = strlen(&text[0]);

        VERIFY_ARE_EQUAL(len2, rbuf.putn_nocopy(&text[0], len2).get());

        VERIFY_ARE_EQUAL(len1 + len2, rbuf.in_avail());

        std::string chars(128, '\0');
        size_t was_available = rbuf.in_avail();
        ULONG pcbRead = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Read(&chars[0], (ULONG)was_available, &pcbRead));
        VERIFY_ARE_EQUAL(was_available, pcbRead);

        VERIFY_ARE_EQUAL("This is a test - but this is not", chars.c_str());

        rbuf.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(membuf_IStreamTest10) { IStreamTest10<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest11()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        char ch = 'a';

        auto seg2 = [&ch](int val) { return (val != -1) && (++ch <= 'z'); };
        auto seg1 = [=, &ch, &rbuf]() { return rbuf.putc(ch).then(seg2); };

        pplx::details::_do_while(seg1).wait();

        VERIFY_ARE_EQUAL(26u, rbuf.in_avail());

        std::string chars(128, '\0');
        size_t was_available = rbuf.in_avail();
        ULONG pcbRead = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Read(&chars[0], (ULONG)was_available, &pcbRead));
        VERIFY_ARE_EQUAL(was_available, pcbRead);

        VERIFY_ARE_EQUAL("abcdefghijklmnopqrstuvwxyz", chars.c_str());

        rbuf.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(membuf_IStreamTest11) { IStreamTest11<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest12()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        std::string text(128, '\0');
        strcpy_s(&text[0], 128, "This is a test");
        size_t len1 = strlen(&text[0]);

        VERIFY_ARE_EQUAL(len1, rbuf.putn_nocopy(&text[0], len1).get());

        strcpy_s(&text[0], 128, " - but this is not");
        size_t len2 = strlen(&text[0]);

        VERIFY_ARE_EQUAL(len2, rbuf.putn_nocopy(&text[0], len2).get());

        VERIFY_ARE_EQUAL(len1 + len2, rbuf.in_avail());

        std::string chars(128, '\0');
        size_t was_available = rbuf.in_avail();
        ULONG pcbRead = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Read(&chars[0], (ULONG)was_available, &pcbRead));
        VERIFY_ARE_EQUAL(was_available, pcbRead);

        VERIFY_ARE_EQUAL("This is a test - but this is not", chars.c_str());

        rbuf.close().get();
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(membuf_IStreamTest12) { IStreamTest12<streams::producer_consumer_buffer<char>>(); }

    template<typename _StreamBufferType>
    void IStreamTest13()
    {
        _StreamBufferType rbuf;
        ISequentialStream_bridge stream(rbuf);

        streams::basic_ostream<char> os(rbuf);

        auto a = os.print("This is a test");
        auto b = os.print(" ");
        auto c = os.print("- but this is not");
        (a && b && c).wait();

        VERIFY_ARE_EQUAL(32u, rbuf.in_avail());

        std::string chars(128, '\0');
        size_t was_available = rbuf.in_avail();
        ULONG pcbRead = 0;
        VERIFY_ARE_EQUAL(S_OK, stream.Read(&chars[0], (ULONG)was_available, &pcbRead));
        VERIFY_ARE_EQUAL(was_available, pcbRead);

        VERIFY_ARE_EQUAL("This is a test - but this is not", chars.c_str());

        os.close().get();

        // The read end should still be open
        VERIFY_IS_TRUE(rbuf.is_open());

        // close the read end
        rbuf.close(std::ios_base::in).get();

        // Now the buffer should no longer be open
        VERIFY_IS_FALSE(rbuf.is_open());
    }

    TEST(membuf_IStreamTest13) { IStreamTest13<streams::producer_consumer_buffer<char>>(); }
#endif

    TEST(producer_consumer_buffer_flush_1)
    {
        streams::producer_consumer_buffer<char> rwbuf;

        VERIFY_IS_TRUE(rwbuf.is_open());
        VERIFY_IS_TRUE(rwbuf.can_read());
        VERIFY_IS_TRUE(rwbuf.can_write());

        char buf1[128], buf2[128];
        memset(buf1, 0, sizeof(buf1));
        memset(buf2, 0, sizeof(buf2));

        // The read happens before the write.

        auto read1 = rwbuf.getn(buf1, 128);
        auto read2 = rwbuf.getn(buf2, 128);

        std::string text1 = "This is a test";
        size_t len1 = text1.size();
        VERIFY_ARE_EQUAL(rwbuf.putn_nocopy(&text1[0], len1).get(), len1);
        rwbuf.sync().wait();

        std::string text2 = "- but this is not";
        size_t len2 = text2.size();
        VERIFY_ARE_EQUAL(rwbuf.putn_nocopy(&text2[0], len2).get(), len2);
        rwbuf.sync().wait();

        VERIFY_ARE_EQUAL(read1.get(), len1);
        VERIFY_ARE_EQUAL(read2.get(), len2);

        rwbuf.close().get();
    }

    TEST(producer_consumer_buffer_flush_2)
    {
        streams::producer_consumer_buffer<char> rwbuf;

        VERIFY_IS_TRUE(rwbuf.is_open());
        VERIFY_IS_TRUE(rwbuf.can_read());
        VERIFY_IS_TRUE(rwbuf.can_write());

        // The read happens after the write.

        std::string text1 = "This is a test";
        std::string text2 = "- but this is not";
        size_t len1 = text1.size();
        size_t len2 = text2.size();
        VERIFY_ARE_EQUAL(rwbuf.putn_nocopy(&text1[0], len1).get(), len1);
        VERIFY_ARE_EQUAL(rwbuf.putn_nocopy(&text2[0], len2).get(), len2);
        rwbuf.sync().wait();

        char buf1[128], buf2[128];
        memset(buf1, 0, sizeof(buf1));
        memset(buf2, 0, sizeof(buf2));

        auto read1 = rwbuf.getn(buf1, 128);

        VERIFY_ARE_EQUAL(read1.get(), len1 + len2);

        rwbuf.close().get();
    }

    TEST(producer_consumer_buffer_flush_3)
    {
        streams::producer_consumer_buffer<char> rwbuf;

        VERIFY_IS_TRUE(rwbuf.is_open());
        VERIFY_IS_TRUE(rwbuf.can_read());
        VERIFY_IS_TRUE(rwbuf.can_write());

        // The read happens before the write.

        char buf1[128], buf2[128];
        memset(buf1, 0, sizeof(buf1));
        memset(buf2, 0, sizeof(buf2));

        auto read1 = rwbuf.getn(buf1, 128);
        auto read2 = rwbuf.getn(buf2, 128);

        for (char c = 'a'; c <= 'z'; ++c)
            rwbuf.putc(c);
        rwbuf.sync().wait();
        for (char c = 'a'; c <= 'z'; ++c)
            rwbuf.putc(c);

        VERIFY_ARE_EQUAL(read1.get(), 26);

        rwbuf.close().get();

        VERIFY_ARE_EQUAL(read2.get(), 26);
    }

    TEST(producer_consumer_buffer_flush_4)
    {
        streams::producer_consumer_buffer<char> rwbuf;

        VERIFY_IS_TRUE(rwbuf.is_open());
        VERIFY_IS_TRUE(rwbuf.can_read());
        VERIFY_IS_TRUE(rwbuf.can_write());

        // The read happens after the write.

        for (char c = 'a'; c <= 'z'; ++c)
            rwbuf.putc(c);
        rwbuf.sync().wait();

        char buf1[128], buf2[128];
        memset(buf1, 0, sizeof(buf1));
        memset(buf2, 0, sizeof(buf2));

        auto read1 = rwbuf.getn(buf1, 20);
        auto read2 = rwbuf.getn(buf1, 128);

        VERIFY_ARE_EQUAL(read1.get(), 20);
        VERIFY_ARE_EQUAL(read2.get(), 6);

        rwbuf.close().get();
    }

    TEST(producer_consumer_buffer_flush_5)
    {
        streams::producer_consumer_buffer<char> rwbuf;

        VERIFY_IS_TRUE(rwbuf.is_open());
        VERIFY_IS_TRUE(rwbuf.can_read());
        VERIFY_IS_TRUE(rwbuf.can_write());

        // The read happens before the write.

        pplx::task<int> buf1[128];

        for (int i = 0; i < 128; ++i)
        {
            buf1[i] = rwbuf.bumpc();
        }

        for (char c = 'a'; c <= 'z'; ++c)
            rwbuf.putc(c);
        rwbuf.sync().wait();

        for (int i = 0; i < 26; ++i)
        {
            VERIFY_ARE_EQUAL('a' + i, buf1[i].get());
        }
        for (int i = 26; i < 128; ++i)
        {
            VERIFY_IS_FALSE(buf1[i].is_done());
        }
        rwbuf.close().get();
    }

    TEST(producer_consumer_buffer_flush_6)
    {
        streams::producer_consumer_buffer<char> rwbuf;

        VERIFY_IS_TRUE(rwbuf.is_open());
        VERIFY_IS_TRUE(rwbuf.can_read());
        VERIFY_IS_TRUE(rwbuf.can_write());

        // The read happens after the write.

        for (char c = 'a'; c <= 'z'; ++c)
            rwbuf.putc(c);
        rwbuf.sync().wait();

        pplx::task<int> buf1[128];

        for (int i = 0; i < 128; ++i)
        {
            buf1[i] = rwbuf.bumpc();
        }

        for (int i = 0; i < 26; ++i)
        {
            VERIFY_IS_TRUE(buf1[i].is_done());
        }
        for (int i = 26; i < 128; ++i)
        {
            VERIFY_IS_FALSE(buf1[i].is_done());
        }
        rwbuf.close().get();
    }

    TEST(producer_consumer_buffer_close_reader_early)
    {
        streams::producer_consumer_buffer<char> rwbuf;

        VERIFY_IS_TRUE(rwbuf.is_open());
        VERIFY_IS_TRUE(rwbuf.can_read());
        VERIFY_IS_TRUE(rwbuf.can_write());

        rwbuf.close(std::ios::in).wait();

        // Even though we have closed for read, we should
        // still be able to write.

        auto size = rwbuf.in_avail();

        for (char c = 'a'; c <= 'z'; ++c)
            VERIFY_ARE_EQUAL((int)c, rwbuf.putc(c).get());

        VERIFY_ARE_EQUAL(size, rwbuf.in_avail());

        std::string text1 = "This is a test";
        size_t len1 = text1.size();
        VERIFY_ARE_EQUAL(rwbuf.putn_nocopy(&text1[0], len1).get(), len1);

        VERIFY_ARE_EQUAL(size, rwbuf.in_avail());

        rwbuf.close().get();
    }

    TEST(container_buffer_exception_propagation)
    {
        struct MyException
        {
        };
        {
            streams::stringstreambuf rwbuf(std::string("this is the test"));
            rwbuf.close(std::ios::out, std::make_exception_ptr(MyException())).wait();
            char buffer[100];
            VERIFY_ARE_EQUAL(rwbuf.getn(buffer, 100).get(), 16);
            VERIFY_THROWS(rwbuf.getn(buffer, 100).get(), MyException);
            VERIFY_THROWS(rwbuf.getc().get(), MyException);
            VERIFY_IS_FALSE(rwbuf.exception() == nullptr);
        }
        {
            streams::stringstreambuf rwbuf(std::string("this is the test"));
            rwbuf.close(std::ios::in, std::make_exception_ptr(MyException()));
            char buffer[100];
            VERIFY_THROWS(rwbuf.getn(buffer, 100).get(), MyException);
            VERIFY_THROWS(rwbuf.getc().get(), MyException);
            VERIFY_IS_FALSE(rwbuf.exception() == nullptr);
        }

        {
            streams::stringstreambuf rwbuf;
            rwbuf.putn_nocopy("this is the test", 16);
            rwbuf.close(std::ios::out, std::make_exception_ptr(MyException()));
            VERIFY_THROWS(rwbuf.putn_nocopy("this is the test", 16).get(), MyException);
            VERIFY_THROWS(rwbuf.putc('c').get(), MyException);
            VERIFY_IS_FALSE(rwbuf.exception() == nullptr);
        }
    }

    TEST(producer_consumer_buffer_exception_propagation)
    {
        struct MyException
        {
        };
        {
            streams::producer_consumer_buffer<char> rwbuf;
            rwbuf.putn_nocopy("this is the test", 16);
            rwbuf.close(std::ios::out, std::make_exception_ptr(MyException()));
            char buffer[100];
            VERIFY_ARE_EQUAL(rwbuf.getn(buffer, 100).get(), 16);
            VERIFY_THROWS(rwbuf.getn(buffer, 100).get(), MyException);
            VERIFY_THROWS(rwbuf.getc().get(), MyException);
            VERIFY_IS_FALSE(rwbuf.exception() == nullptr);
        }
        {
            streams::producer_consumer_buffer<char> rwbuf;
            rwbuf.putn_nocopy("this is the test", 16);
            rwbuf.close(std::ios::in, std::make_exception_ptr(MyException()));
            char buffer[100];
            VERIFY_THROWS(rwbuf.getn(buffer, 100).get(), MyException);
            VERIFY_THROWS(rwbuf.getc().get(), MyException);
            VERIFY_IS_FALSE(rwbuf.exception() == nullptr);
        }

        {
            streams::producer_consumer_buffer<char> rwbuf;
            rwbuf.putn_nocopy("this is the test", 16);
            rwbuf.close(std::ios::out, std::make_exception_ptr(MyException()));
            VERIFY_THROWS(rwbuf.putn_nocopy("this is the test", 16).get(), MyException);
            VERIFY_THROWS(rwbuf.putc('c').get(), MyException);
            VERIFY_IS_FALSE(rwbuf.exception() == nullptr);
        }
    }

    TEST(producer_consumer_alloc_after_close)
    {
        producer_consumer_buffer<char> buffer;
        buffer.close().wait();
        VERIFY_IS_TRUE(buffer.alloc(2) == nullptr);

        buffer = producer_consumer_buffer<char>();
        buffer.close(std::ios::out);
        VERIFY_IS_TRUE(buffer.alloc(2) == nullptr);
    }

    TEST(producer_consumer_acquire_after_close)
    {
        char* temp = nullptr;
        size_t size = 0;
        producer_consumer_buffer<char> buffer;
        buffer.close().wait();
        VERIFY_IS_FALSE(buffer.acquire(temp, size));
        VERIFY_IS_TRUE(nullptr == temp);
        VERIFY_ARE_EQUAL(0, size);
        buffer.release(temp, size);

        buffer = producer_consumer_buffer<char>();
        buffer.close(std::ios::out);
        temp = (char*)1;
        size = 1;
        VERIFY_IS_TRUE(buffer.acquire(temp, size));
        VERIFY_IS_TRUE(nullptr == temp);
        VERIFY_ARE_EQUAL(0, size);
        buffer.release(temp, size);
    }

    TEST(create_buffers_inout_error)
    {
        VERIFY_THROWS(container_buffer<std::string>(std::ios::in | std::ios::out), std::invalid_argument);
        VERIFY_THROWS(container_buffer<std::string>("test data", std::ios::in | std::ios::out), std::invalid_argument);
        char* data = nullptr;
        VERIFY_THROWS(rawptr_buffer<char>(data, 2, std::ios::in | std::ios::out), std::invalid_argument);
    }

    TEST(memstream_length)
    {
        producer_consumer_buffer<unsigned char> rbuf;
        auto istr = rbuf.create_istream();

        auto curr = istr.tell();
        VERIFY_ARE_EQUAL((long long)curr, 0);
    }

    TEST(buffer_size)
    {
        {
            container_buffer<std::string> buf("test data");
            VERIFY_IS_TRUE(buf.has_size());
            VERIFY_ARE_EQUAL(buf.size(), 9);
            buf.seekoff(1024, std::ios::beg, std::ios::in);
            VERIFY_ARE_EQUAL(buf.size(), 9);
        }
        {
            container_buffer<std::string> buf;
            VERIFY_IS_TRUE(buf.has_size());
            VERIFY_ARE_EQUAL(buf.size(), 0);
            buf.seekoff(1024, std::ios::beg, std::ios::out);
            VERIFY_ARE_EQUAL(buf.size(), 1024);
            VERIFY_ARE_EQUAL(buf.collection().size(), 1024);
        }
        {
            producer_consumer_buffer<uint8_t> buf;
            VERIFY_IS_FALSE(buf.has_size());
            VERIFY_ARE_EQUAL(buf.size(), 0);
        }
    }

    TEST(rawptr_alloc_after_close)
    {
        char data[2];
        rawptr_buffer<char> buffer(&data[0], sizeof(data), std::ios::out);
        buffer.close().wait();
        VERIFY_IS_TRUE(buffer.alloc(2) == nullptr);

        buffer = rawptr_buffer<char>(&data[0], sizeof(data), std::ios::out);
        buffer.close(std::ios::out);
        VERIFY_IS_TRUE(buffer.alloc(2) == nullptr);
    }

    TEST(rawptr_alloc_too_large)
    {
        char data[4];
        rawptr_buffer<char> buffer(&data[0], sizeof(data), std::ios::out);
        VERIFY_IS_TRUE(buffer.alloc(10) == nullptr);
    }

    TEST(rawptr_buffer_acquire_after_close)
    {
        char* temp = nullptr;
        size_t size = 0;
        char data[2];
        rawptr_buffer<char> buffer(&data[0], sizeof(data), std::ios::in);
        buffer.close().wait();
        VERIFY_IS_FALSE(buffer.acquire(temp, size));
        VERIFY_IS_TRUE(nullptr == temp);
        VERIFY_ARE_EQUAL(0, size);
        buffer.release(temp, size);

        buffer = rawptr_buffer<char>(nullptr, 0, std::ios::in);
        temp = (char*)1;
        size = 1;
        VERIFY_IS_TRUE(buffer.acquire(temp, size));
        VERIFY_IS_TRUE(nullptr == temp);
        VERIFY_ARE_EQUAL(0, size);
        buffer.release(temp, size);
    }

    TEST(container_buffer_alloc_after_close)
    {
        container_buffer<std::string> buffer;
        buffer.close().wait();
        VERIFY_IS_TRUE(buffer.alloc(2) == nullptr);

        buffer = container_buffer<std::string>();
        buffer.close(std::ios::out);
        VERIFY_IS_TRUE(buffer.alloc(2) == nullptr);
    }

    TEST(container_buffer_acquire_after_close)
    {
        char* temp = nullptr;
        size_t size = 0;
        container_buffer<std::string> buffer("test data");
        buffer.close().wait();
        VERIFY_IS_FALSE(buffer.acquire(temp, size));
        VERIFY_IS_TRUE(nullptr == temp);
        VERIFY_ARE_EQUAL(0, size);
        buffer.release(temp, size);

        buffer = container_buffer<std::string>(std::ios::in);
        temp = (char*)1;
        size = 1;
        VERIFY_IS_TRUE(buffer.acquire(temp, size));
        VERIFY_IS_TRUE(nullptr == temp);
        VERIFY_ARE_EQUAL(0, size);
        buffer.release(temp, size);
    }

    TEST(bytestream_length)
    {
        // test byte stream
        std::string s("12345");
        auto istr = bytestream::open_istream(s);
        test_stream_length(istr, s.size());
    }

    TEST(read_pending_close_with_exception)
    {
        producer_consumer_buffer<char> sourceBuf;

        const size_t size = 4;
        char buf[size];
        memset(&buf[0], '0', size);
        auto firstRead = sourceBuf.getn(buf, size);
        sourceBuf.putc('a').wait();

        sourceBuf.close(std::ios::in | std::ios::out, std::make_exception_ptr(std::runtime_error("test exception")))
            .wait();
        VERIFY_ARE_EQUAL(firstRead.get(), 1);
        VERIFY_ARE_EQUAL(buf[0], 'a');
        VERIFY_ARE_EQUAL(buf[1], '0');
        VERIFY_ARE_EQUAL(buf[2], '0');
        VERIFY_ARE_EQUAL(buf[3], '0');

        VERIFY_THROWS(sourceBuf.getn(buf, size).get(), std::runtime_error);
        VERIFY_ARE_EQUAL(buf[0], 'a');
        VERIFY_ARE_EQUAL(buf[1], '0');
        VERIFY_ARE_EQUAL(buf[2], '0');
        VERIFY_ARE_EQUAL(buf[3], '0');
    }

    TEST(close_on_one_head_write)
    {
        producer_consumer_buffer<char> sourceBuf;
        sourceBuf.putc('a').wait();

        auto ostream = sourceBuf.create_ostream();

        ostream.close().wait();

        // Check that the exception is generated by the 'get(),' not the operation.
        auto t1 = sourceBuf.putc('b');
        auto t2 = ostream.write('b');
        VERIFY_ARE_EQUAL(t1.get(), streams::streambuf<char>::traits::eof());
        VERIFY_THROWS(t2.get(), std::runtime_error);
        VERIFY_ARE_EQUAL(sourceBuf.getc().get(), 'a');
    }

    TEST(close_on_one_head_read)
    {
        producer_consumer_buffer<char> sourceBuf;
        sourceBuf.putc('a').wait();

        auto istream = sourceBuf.create_istream();

        istream.close().wait();

        // Check that the exception is generated by the 'get(),' not the operation.
        auto t1 = sourceBuf.bumpc();
        auto t2 = istream.read();
        VERIFY_ARE_EQUAL(t1.get(), streams::streambuf<char>::traits::eof());
        VERIFY_THROWS(t2.get(), std::runtime_error);
        VERIFY_ARE_EQUAL(sourceBuf.putc('a').get(), 'a');
    }

    TEST(close_with_exception_on_one_head_write)
    {
        producer_consumer_buffer<char> sourceBuf;
        sourceBuf.putc('a').wait();

        auto ostream = sourceBuf.create_ostream();

        ostream.close(std::make_exception_ptr(std::invalid_argument("test exception"))).wait();

        // Check that the exception is generated by the 'get(),' not the operation.
        auto t1 = sourceBuf.putc('b');
        auto t2 = ostream.write('b');
        VERIFY_THROWS(t1.get(), std::invalid_argument);
        VERIFY_THROWS(t2.get(), std::invalid_argument);
        VERIFY_ARE_EQUAL(sourceBuf.getc().get(), 'a');
    }

    TEST(close_with_exception_on_one_head_read)
    {
        producer_consumer_buffer<char> sourceBuf;
        sourceBuf.putc('a').wait();

        auto istream = sourceBuf.create_istream();

        istream.close(std::make_exception_ptr(std::invalid_argument("test exception"))).wait();

        // Check that the exception is generated by the 'get(),' not the operation.
        auto t1 = sourceBuf.bumpc();
        auto t2 = istream.read();
        VERIFY_THROWS(t1.get(), std::invalid_argument);
        VERIFY_THROWS(t2.get(), std::invalid_argument);
        VERIFY_ARE_EQUAL(sourceBuf.putc('a').get(), 'a');
    }

    TEST(close_twice)
    {
        // This test passes if it does not generate an exception.
        {
            producer_consumer_buffer<char> sourceBuf;
            sourceBuf.close(std::ios::in).wait();
            sourceBuf.close(std::ios::in).wait();
        }
        {
            producer_consumer_buffer<char> sourceBuf;
            sourceBuf.close(std::ios::out).wait();
            sourceBuf.close(std::ios::out).wait();
        }
        {
            producer_consumer_buffer<char> sourceBuf;
            sourceBuf.close().wait();
            sourceBuf.close().wait();
        }
    }
}

} // namespace streams
} // namespace functional
} // namespace tests
