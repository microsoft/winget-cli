/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Adapter classes for async and STD stream buffers, used to connect std-based and async-based APIs.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

#include "cpprest/astreambuf.h"
#include "cpprest/streams.h"
#include "pplx/pplxtasks.h"

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 4250)
#endif

namespace Concurrency
{
namespace streams
{
template<typename CharType>
class stdio_ostream;
template<typename CharType>
class stdio_istream;

namespace details
{
/// <summary>
/// The basic_stdio_buffer class serves to support interoperability with STL stream buffers.
/// Sitting atop a std::streambuf, which does all the I/O, instances of this class may read
/// and write data to standard iostreams. The class itself should not be used in application
/// code, it is used by the stream definitions farther down in the header file.
/// </summary>
template<typename _CharType>
class basic_stdio_buffer : public streambuf_state_manager<_CharType>
{
    typedef concurrency::streams::char_traits<_CharType> traits;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;
    /// <summary>
    /// Private constructor
    /// </summary>
    basic_stdio_buffer(_In_ std::basic_streambuf<_CharType>* streambuf, std::ios_base::openmode mode)
        : streambuf_state_manager<_CharType>(mode), m_buffer(streambuf)
    {
    }

public:
    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~basic_stdio_buffer()
    {
        this->_close_read();
        this->_close_write();
    }

private:
    //
    // The functions overridden below here are documented elsewhere.
    // See astreambuf.h for further information.
    //
    virtual bool can_seek() const { return this->is_open(); }
    virtual bool has_size() const { return false; }

    virtual size_t in_avail() const { return (size_t)m_buffer->in_avail(); }

    virtual size_t buffer_size(std::ios_base::openmode) const { return 0; }
    virtual void set_buffer_size(size_t, std::ios_base::openmode) { return; }

    virtual pplx::task<bool> _sync() { return pplx::task_from_result(m_buffer->pubsync() == 0); }

    virtual pplx::task<int_type> _putc(_CharType ch) { return pplx::task_from_result(m_buffer->sputc(ch)); }
    virtual pplx::task<size_t> _putn(const _CharType* ptr, size_t size)
    {
        return pplx::task_from_result((size_t)m_buffer->sputn(ptr, size));
    }

    size_t _sgetn(_Out_writes_(size) _CharType* ptr, _In_ size_t size) const { return m_buffer->sgetn(ptr, size); }
    virtual size_t _scopy(_Out_writes_(size) _CharType*, _In_ size_t size)
    {
        (void)(size);
        return (size_t)-1;
    }

    virtual pplx::task<size_t> _getn(_Out_writes_(size) _CharType* ptr, _In_ size_t size)
    {
        return pplx::task_from_result((size_t)m_buffer->sgetn(ptr, size));
    }

    virtual int_type _sbumpc() { return m_buffer->sbumpc(); }
    virtual int_type _sgetc() { return m_buffer->sgetc(); }

    virtual pplx::task<int_type> _bumpc() { return pplx::task_from_result<int_type>(m_buffer->sbumpc()); }
    virtual pplx::task<int_type> _getc() { return pplx::task_from_result<int_type>(m_buffer->sgetc()); }
    virtual pplx::task<int_type> _nextc() { return pplx::task_from_result<int_type>(m_buffer->snextc()); }
    virtual pplx::task<int_type> _ungetc() { return pplx::task_from_result<int_type>(m_buffer->sungetc()); }

    virtual pos_type getpos(std::ios_base::openmode mode) const
    {
        return m_buffer->pubseekoff(0, std::ios_base::cur, mode);
    }
    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode mode) { return m_buffer->pubseekpos(pos, mode); }
    virtual pos_type seekoff(off_type off, std::ios_base::seekdir dir, std::ios_base::openmode mode)
    {
        return m_buffer->pubseekoff(off, dir, mode);
    }

    virtual _CharType* _alloc(size_t) { return nullptr; }
    virtual void _commit(size_t) {}

    virtual bool acquire(_CharType*&, size_t&) { return false; }
    virtual void release(_CharType*, size_t) {}

    template<typename CharType>
    friend class concurrency::streams::stdio_ostream;
    template<typename CharType>
    friend class concurrency::streams::stdio_istream;

    std::basic_streambuf<_CharType>* m_buffer;
};

} // namespace details

/// <summary>
/// stdio_ostream represents an async ostream derived from a standard synchronous stream, as
/// defined by the "std" namespace. It is constructed from a reference to a standard stream, which
/// must be valid for the lifetime of the asynchronous stream.
/// </summary>
/// <typeparam name="CharType">
/// The data type of the basic element of the <c>stdio_ostream</c>.
/// </typeparam>
/// <remarks>
/// Since std streams are not reference-counted, great care must be taken by an application to make
/// sure that the std stream does not get destroyed until all uses of the asynchronous stream are
/// done and have been serviced.
/// </remarks>
template<typename CharType>
class stdio_ostream : public basic_ostream<CharType>
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <typeparam name="AlterCharType">
    /// The data type of the basic element of the source output stream.
    /// </typeparam>
    /// <param name="stream">The synchronous stream that this is using for its I/O</param>
    template<typename AlterCharType>
    stdio_ostream(std::basic_ostream<AlterCharType>& stream)
        : basic_ostream<CharType>(
              streams::streambuf<AlterCharType>(std::shared_ptr<details::basic_stdio_buffer<AlterCharType>>(
                  new details::basic_stdio_buffer<AlterCharType>(stream.rdbuf(), std::ios_base::out))))
    {
    }

    /// <summary>
    /// Copy constructor
    /// </summary>
    /// <param name="other">The source object</param>
    stdio_ostream(const stdio_ostream& other) : basic_ostream<CharType>(other) {}

    /// <summary>
    /// Assignment operator
    /// </summary>
    /// <param name="other">The source object</param>
    /// <returns>A reference to the output stream object that contains the result of the assignment.</returns>
    stdio_ostream& operator=(const stdio_ostream& other)
    {
        basic_ostream<CharType>::operator=(other);
        return *this;
    }
};

/// <summary>
/// stdio_istream represents an async istream derived from a standard synchronous stream, as
/// defined by the "std" namespace. It is constructed from a reference to a standard stream, which
/// must be valid for the lifetime of the asynchronous stream.
/// </summary>
/// <typeparam name="CharType">
/// The data type of the basic element of the <c>stdio_istream</c>.
/// </typeparam>
/// <remarks>
/// Since std streams are not reference-counted, great care must be taken by an application to make
/// sure that the std stream does not get destroyed until all uses of the asynchronous stream are
/// done and have been serviced.
/// </remarks>
template<typename CharType>
class stdio_istream : public basic_istream<CharType>
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <typeparam name="AlterCharType">
    /// The data type of the basic element of the source <c>istream</c>
    /// </typeparam>
    /// <param name="stream">The synchronous stream that this is using for its I/O</param>
    template<typename AlterCharType>
    stdio_istream(std::basic_istream<AlterCharType>& stream)
        : basic_istream<CharType>(
              streams::streambuf<AlterCharType>(std::shared_ptr<details::basic_stdio_buffer<AlterCharType>>(
                  new details::basic_stdio_buffer<AlterCharType>(stream.rdbuf(), std::ios_base::in))))
    {
    }

    /// <summary>
    /// Copy constructor
    /// </summary>
    /// <param name="other">The source object</param>
    stdio_istream(const stdio_istream& other) : basic_istream<CharType>(other) {}

    /// <summary>
    /// Assignment operator
    /// </summary>
    /// <param name="other">The source object</param>
    /// <returns>A reference to the input stream object that contains the result of the assignment.</returns>
    stdio_istream& operator=(const stdio_istream& other)
    {
        basic_istream<CharType>::operator=(other);
        return *this;
    }
};

namespace details
{
/// <summary>
/// IO streams stream buffer implementation used to interface with an async streambuffer underneath.
/// Used for implementing the standard synchronous streams that provide interop between std:: and concurrency::streams::
/// </summary>
template<typename CharType>
class basic_async_streambuf : public std::basic_streambuf<CharType>
{
public:
    typedef concurrency::streams::char_traits<CharType> traits;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;

    basic_async_streambuf(const streams::streambuf<CharType>& async_buf) : m_buffer(async_buf) {}

protected:
    //
    // The following are the functions in std::basic_streambuf that we need to override.
    //

    /// <summary>
    /// Writes one byte to the stream buffer.
    /// </summary>
    int_type overflow(int_type ch)
    {
        try
        {
            return m_buffer.putc(CharType(ch)).get();
        }
        catch (...)
        {
            return traits::eof();
        }
    }

    /// <summary>
    /// Gets one byte from the stream buffer without moving the read position.
    /// </summary>
    int_type underflow()
    {
        try
        {
            return m_buffer.getc().get();
        }
        catch (...)
        {
            return traits::eof();
        }
    }

    /// <summary>
    /// Gets one byte from the stream buffer and move the read position one character.
    /// </summary>
    int_type uflow()
    {
        try
        {
            return m_buffer.bumpc().get();
        }
        catch (...)
        {
            return traits::eof();
        }
    }

    /// <summary>
    /// Gets a number of characters from the buffer and place it into the provided memory block.
    /// </summary>
    std::streamsize xsgetn(_Out_writes_(count) CharType* ptr, _In_ std::streamsize count)
    {
        size_t cnt = size_t(count);
        size_t read_so_far = 0;

        try
        {
            while (read_so_far < cnt)
            {
                size_t rd = m_buffer.getn(ptr + read_so_far, cnt - read_so_far).get();
                read_so_far += rd;
                if (rd == 0) break;
            }
            return read_so_far;
        }
        catch (...)
        {
            return 0;
        }
    }

    /// <summary>
    /// Writes a given number of characters from the provided block into the stream buffer.
    /// </summary>
    std::streamsize xsputn(const CharType* ptr, std::streamsize count)
    {
        try
        {
            return m_buffer.putn_nocopy(ptr, static_cast<size_t>(count)).get();
        }
        catch (...)
        {
            return 0;
        }
    }

    /// <summary>
    /// Synchronizes with the underlying medium.
    /// </summary>
    int sync() // must be int as per std::basic_streambuf
    {
        try
        {
            m_buffer.sync().wait();
        }
        catch (...)
        {
        }
        return 0;
    }

    /// <summary>
    /// Seeks to the given offset relative to the beginning, end, or current position.
    /// </summary>
    pos_type seekoff(off_type offset,
                     std::ios_base::seekdir dir,
                     std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
    {
        try
        {
            if (dir == std::ios_base::cur && offset == 0) // Special case for getting the current position.
                return m_buffer.getpos(mode);
            return m_buffer.seekoff(offset, dir, mode);
        }
        catch (...)
        {
            return (pos_type(-1));
        }
    }

    /// <summary>
    /// Seeks to the given offset relative to the beginning of the stream.
    /// </summary>
    pos_type seekpos(pos_type pos, std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
    {
        try
        {
            return m_buffer.seekpos(pos, mode);
        }
        catch (...)
        {
            return (pos_type(-1));
        }
    }

private:
    concurrency::streams::streambuf<CharType> m_buffer;
};

} // namespace details

/// <summary>
/// A concrete STL ostream which relies on an asynchronous stream for its I/O.
/// </summary>
/// <typeparam name="CharType">
/// The data type of the basic element of the stream.
/// </typeparam>
template<typename CharType>
class async_ostream : public std::basic_ostream<CharType>
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <typeparam name="AlterCharType">
    /// The data type of the basic element of the source ostream.
    /// </typeparam>
    /// <param name="astream">The asynchronous stream whose stream buffer should be used for I/O</param>
    template<typename AlterCharType>
    async_ostream(const streams::basic_ostream<AlterCharType>& astream)
        : std::basic_ostream<CharType>(&m_strbuf), m_strbuf(astream.streambuf())
    {
    }

    /// <summary>
    /// Constructor
    /// </summary>
    /// <typeparam name="AlterCharType">
    /// The data type of the basic element of the source <c>streambuf</c>.
    /// </typeparam>
    /// <param name="strbuf">The asynchronous stream buffer to use for I/O</param>
    template<typename AlterCharType>
    async_ostream(const streams::streambuf<AlterCharType>& strbuf)
        : std::basic_ostream<CharType>(&m_strbuf), m_strbuf(strbuf)
    {
    }

private:
    details::basic_async_streambuf<CharType> m_strbuf;
};

/// <summary>
/// A concrete STL istream which relies on an asynchronous stream for its I/O.
/// </summary>
/// <typeparam name="CharType">
/// The data type of the basic element of the stream.
/// </typeparam>
template<typename CharType>
class async_istream : public std::basic_istream<CharType>
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <typeparam name="AlterCharType">
    /// The data type of the basic element of the source istream.
    /// </typeparam>
    /// <param name="astream">The asynchronous stream whose stream buffer should be used for I/O</param>
    template<typename AlterCharType>
    async_istream(const streams::basic_istream<AlterCharType>& astream)
        : std::basic_istream<CharType>(&m_strbuf), m_strbuf(astream.streambuf())
    {
    }

    /// <summary>
    /// Constructor
    /// </summary>
    /// <typeparam name="AlterCharType">
    /// The data type of the basic element of the source <c>streambuf</c>.
    /// </typeparam>
    /// <param name="strbuf">The asynchronous stream buffer to use for I/O</param>
    template<typename AlterCharType>
    async_istream(const streams::streambuf<AlterCharType>& strbuf)
        : std::basic_istream<CharType>(&m_strbuf), m_strbuf(strbuf)
    {
    }

private:
    details::basic_async_streambuf<CharType> m_strbuf;
};

/// <summary>
/// A concrete STL istream which relies on an asynchronous stream buffer for its I/O.
/// </summary>
/// <typeparam name="CharType">
/// The data type of the basic element of the stream.
/// </typeparam>
template<typename CharType>
class async_iostream : public std::basic_iostream<CharType>
{
public:
    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="strbuf">The asynchronous stream buffer to use for I/O</param>
    async_iostream(const streams::streambuf<CharType>& strbuf)
        : std::basic_iostream<CharType>(&m_strbuf), m_strbuf(strbuf)
    {
    }

private:
    details::basic_async_streambuf<CharType> m_strbuf;
};

#if defined(__cplusplus_winrt)

/// <summary>
/// Static class containing factory functions for WinRT streams implemented on top of Casablanca async streams.
/// </summary>
/// <remarks>WinRT streams are defined in terms of single-byte characters only.</remarks>
class winrt_stream
{
public:
    /// <summary>
    /// Creates a WinRT <c>IInputStream</c> reference from an asynchronous stream buffer.
    /// </summary>
    /// <param name="buffer">A stream buffer based on a single-byte character.</param>
    /// <returns>A reference to a WinRT <c>IInputStream</c>.</returns>
    /// <remarks>
    /// The stream buffer passed in must allow reading.
    /// The stream buffer is shared with the caller, allowing data to be passed between the two contexts. For
    /// example, using a <c>producer_consumer_buffer</c>, a Casablanca-based caller can pass data to a WinRT component.
    /// </remarks>
    _ASYNCRTIMP static Windows::Storage::Streams::IInputStream ^
        __cdecl create_input_stream(const concurrency::streams::streambuf<uint8_t>& buffer);

    /// <summary>
    /// Creates a WinRT <c>IOutputStream</c> reference from an asynchronous stream buffer.
    /// </summary>
    /// <param name="buffer">A stream buffer based on a single-byte character.</param>
    /// <returns>A reference to a WinRT <c>IOutputStream</c>.</returns>
    /// <remarks>
    /// The stream buffer passed in must allow writing.
    /// The stream buffer is shared with the caller, allowing data to be passed between the two contexts. For
    /// example, using a <c>producer_consumer_buffer</c>, a Casablanca-based caller can retrieve data from a WinRT
    /// component.
    /// </remarks>
    _ASYNCRTIMP static Windows::Storage::Streams::IOutputStream ^
        __cdecl create_output_stream(const concurrency::streams::streambuf<uint8_t>& buffer);

    /// <summary>
    /// Creates a WinRT <c>IRandomAccessStream reference from an asynchronous input stream.
    /// </summary>
    /// <param name="buffer">A stream based on a single-byte character.</param>
    /// <returns>A reference to a WinRT <c>IRandomAccessStream</c>.</returns>
    /// <remarks>
    /// The stream buffer is shared with the caller, allowing data to be passed between the two contexts. For
    /// example, using a <c>producer_consumer_buffer</c>, a Casablanca-based caller can pass data to and retrieve data
    /// from a WinRT component.
    /// </remarks>
    _ASYNCRTIMP static Windows::Storage::Streams::IRandomAccessStream ^
        __cdecl create_random_access_stream(const concurrency::streams::streambuf<uint8_t>& buffer);
};

#endif

} // namespace streams
} // namespace Concurrency

#if defined(_WIN32)
#pragma warning(pop)
#endif
