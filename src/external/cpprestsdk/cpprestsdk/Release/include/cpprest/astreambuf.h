/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Asynchronous I/O: stream buffer. This is an extension to the PPL concurrency features and therefore
 * lives in the Concurrency namespace.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

#include "cpprest/asyncrt_utils.h"
#include "cpprest/details/basic_types.h"
#include "pplx/pplxtasks.h"
#include <atomic>
#include <cstring>
#include <ios>
#include <math.h>
#include <memory>

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) && !CPPREST_FORCE_PPLX
namespace Concurrency // since namespace pplx = Concurrency
#else
namespace pplx
#endif
{
namespace details
{
template<class F, class T = bool>
pplx::task<T> _do_while(F func)
{
    pplx::task<T> first = func();
    return first.then([=](bool guard) -> pplx::task<T> {
        if (guard)
            return pplx::details::_do_while<F, T>(func);
        else
            return first;
    });
}
} // namespace details
}

namespace Concurrency
{
/// Library for asynchronous streams.
namespace streams
{
/// <summary>
/// Extending the standard char_traits type with one that adds values and types
/// that are unique to "C++ REST SDK" streams.
/// </summary>
/// <typeparam name="_CharType">
/// The data type of the basic element of the stream.
/// </typeparam>
template<typename _CharType>
struct char_traits : std::char_traits<_CharType>
{
    /// <summary>
    /// Some synchronous functions will return this value if the operation
    /// requires an asynchronous call in a given situation.
    /// </summary>
    /// <returns>An <c>int_type</c> value which implies that an asynchronous call is required.</returns>
    static typename std::char_traits<_CharType>::int_type requires_async()
    {
        return std::char_traits<_CharType>::eof() - 1;
    }
};
#if !defined(_WIN32)
template<>
struct char_traits<unsigned char> : private std::char_traits<char>
{
public:
    typedef unsigned char char_type;

    using std::char_traits<char>::eof;
    using std::char_traits<char>::int_type;
    using std::char_traits<char>::off_type;
    using std::char_traits<char>::pos_type;

    static size_t length(const unsigned char* str)
    {
        return std::char_traits<char>::length(reinterpret_cast<const char*>(str));
    }

    static void assign(unsigned char& left, const unsigned char& right) { left = right; }
    static unsigned char* assign(unsigned char* left, size_t n, unsigned char value)
    {
        return reinterpret_cast<unsigned char*>(
            std::char_traits<char>::assign(reinterpret_cast<char*>(left), n, static_cast<char>(value)));
    }

    static unsigned char* copy(unsigned char* left, const unsigned char* right, size_t n)
    {
        return reinterpret_cast<unsigned char*>(
            std::char_traits<char>::copy(reinterpret_cast<char*>(left), reinterpret_cast<const char*>(right), n));
    }

    static unsigned char* move(unsigned char* left, const unsigned char* right, size_t n)
    {
        return reinterpret_cast<unsigned char*>(
            std::char_traits<char>::move(reinterpret_cast<char*>(left), reinterpret_cast<const char*>(right), n));
    }

    static int_type requires_async() { return eof() - 1; }
};
#endif

namespace details
{
/// <summary>
/// Stream buffer base class.
/// </summary>
template<typename _CharType>
class basic_streambuf
{
public:
    typedef _CharType char_type;
    typedef ::concurrency::streams::char_traits<_CharType> traits;

    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;

    /// <summary>
    /// Virtual constructor for stream buffers.
    /// </summary>
    virtual ~basic_streambuf() {}

    /// <summary>
    /// <c>can_read</c> is used to determine whether a stream buffer will support read operations (get).
    /// </summary>
    virtual bool can_read() const = 0;

    /// <summary>
    /// <c>can_write</c> is used to determine whether a stream buffer will support write operations (put).
    /// </summary>
    virtual bool can_write() const = 0;

    /// <summary>
    /// <c>can_seek<c/> is used to determine whether a stream buffer supports seeking.
    /// </summary>
    virtual bool can_seek() const = 0;

    /// <summary>
    /// <c>has_size<c/> is used to determine whether a stream buffer supports size().
    /// </summary>
    virtual bool has_size() const = 0;

    /// <summary>
    /// <c>is_eof</c> is used to determine whether a read head has reached the end of the buffer.
    /// </summary>
    virtual bool is_eof() const = 0;

    /// <summary>
    /// Gets the stream buffer size, if one has been set.
    /// </summary>
    /// <param name="direction">The direction of buffering (in or out)</param>
    /// <returns>The size of the internal buffer (for the given direction).</returns>
    /// <remarks>An implementation that does not support buffering will always return 0.</remarks>
    virtual size_t buffer_size(std::ios_base::openmode direction = std::ios_base::in) const = 0;

    /// <summary>
    /// Sets the stream buffer implementation to buffer or not buffer.
    /// </summary>
    /// <param name="size">The size to use for internal buffering, 0 if no buffering should be done.</param>
    /// <param name="direction">The direction of buffering (in or out)</param>
    /// <remarks>An implementation that does not support buffering will silently ignore calls to this function and it
    /// will not have any effect on what is returned by subsequent calls to <see cref="::buffer_size method"
    /// />.</remarks>
    virtual void set_buffer_size(size_t size, std::ios_base::openmode direction = std::ios_base::in) = 0;

    /// <summary>
    /// For any input stream, <c>in_avail</c> returns the number of characters that are immediately available
    /// to be consumed without blocking. May be used in conjunction with <cref="::sbumpc method"/> to read data without
    /// incurring the overhead of using tasks.
    /// </summary>
    virtual size_t in_avail() const = 0;

    /// <summary>
    /// Checks if the stream buffer is open.
    /// </summary>
    /// <remarks>No separation is made between open for reading and open for writing.</remarks>
    virtual bool is_open() const = 0;

    /// <summary>
    /// Closes the stream buffer, preventing further read or write operations.
    /// </summary>
    /// <param name="mode">The I/O mode (in or out) to close for.</param>
    virtual pplx::task<void> close(std::ios_base::openmode mode = (std::ios_base::in | std::ios_base::out)) = 0;

    /// <summary>
    /// Closes the stream buffer with an exception.
    /// </summary>
    /// <param name="mode">The I/O mode (in or out) to close for.</param>
    /// <param name="eptr">Pointer to the exception.</param>
    virtual pplx::task<void> close(std::ios_base::openmode mode, std::exception_ptr eptr) = 0;

    /// <summary>
    /// Writes a single character to the stream.
    /// </summary>
    /// <param name="ch">The character to write</param>
    /// <returns>A <c>task</c> that holds the value of the character. This value is EOF if the write operation
    /// fails.</returns>
    virtual pplx::task<int_type> putc(_CharType ch) = 0;

    /// <summary>
    /// Writes a number of characters to the stream.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be written.</param>
    /// <param name="count">The number of characters to write.</param>
    /// <returns>A <c>task</c> that holds the number of characters actually written, either 'count' or 0.</returns>
    virtual pplx::task<size_t> putn(const _CharType* ptr, size_t count) = 0;

    /// <summary>
    /// Writes a number of characters to the stream. Note: callers must make sure the data to be written is valid until
    /// the returned task completes.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be written.</param>
    /// <param name="count">The number of characters to write.</param>
    /// <returns>A <c>task</c> that holds the number of characters actually written, either 'count' or 0.</returns>
    virtual pplx::task<size_t> putn_nocopy(const _CharType* ptr, size_t count) = 0;

    /// <summary>
    /// Reads a single character from the stream and advances the read position.
    /// </summary>
    /// <returns>A <c>task</c> that holds the value of the character. This value is EOF if the read fails.</returns>
    virtual pplx::task<int_type> bumpc() = 0;

    /// <summary>
    /// Reads a single character from the stream and advances the read position.
    /// </summary>
    /// <returns>The value of the character. <c>-1</c> if the read fails. <c>-2</c> if an asynchronous read is
    /// required</returns> <remarks>This is a synchronous operation, but is guaranteed to never block.</remarks>
    virtual int_type sbumpc() = 0;

    /// <summary>
    /// Reads a single character from the stream without advancing the read position.
    /// </summary>
    /// <returns>A <c>task</c> that holds the value of the byte. This value is EOF if the read fails.</returns>
    virtual pplx::task<int_type> getc() = 0;

    /// <summary>
    /// Reads a single character from the stream without advancing the read position.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails. <see cref="::requires_async method" /> if an
    /// asynchronous read is required</returns> <remarks>This is a synchronous operation, but is guaranteed to never
    /// block.</remarks>
    virtual int_type sgetc() = 0;

    /// <summary>
    /// Advances the read position, then returns the next character without advancing again.
    /// </summary>
    /// <returns>A <c>task</c> that holds the value of the character. This value is EOF if the read fails.</returns>
    virtual pplx::task<int_type> nextc() = 0;

    /// <summary>
    /// Retreats the read position, then returns the current character without advancing.
    /// </summary>
    /// <returns>A <c>task</c> that holds the value of the character. This value is EOF if the read fails,
    /// <c>requires_async</c> if an asynchronous read is required</returns>
    virtual pplx::task<int_type> ungetc() = 0;

    /// <summary>
    /// Reads up to a given number of characters from the stream.
    /// </summary>
    /// <param name="ptr">The address of the target memory area.</param>
    /// <param name="count">The maximum number of characters to read.</param>
    /// <returns>A <c>task</c> that holds the number of characters read. This value is O if the end of the stream is
    /// reached.</returns>
    virtual pplx::task<size_t> getn(_Out_writes_(count) _CharType* ptr, _In_ size_t count) = 0;

    /// <summary>
    /// Copies up to a given number of characters from the stream, synchronously.
    /// </summary>
    /// <param name="ptr">The address of the target memory area.</param>
    /// <param name="count">The maximum number of characters to read.</param>
    /// <returns>The number of characters copied. O if the end of the stream is reached or an asynchronous read is
    /// required.</returns> <remarks>This is a synchronous operation, but is guaranteed to never block.</remarks>
    virtual size_t scopy(_Out_writes_(count) _CharType* ptr, _In_ size_t count) = 0;

    /// <summary>
    /// Gets the current read or write position in the stream.
    /// </summary>
    /// <param name="direction">The I/O direction to seek (see remarks)</param>
    /// <returns>The current position. EOF if the operation fails.</returns>
    /// <remarks>Some streams may have separate write and read cursors.
    ///          For such streams, the direction parameter defines whether to move the read or the write
    ///          cursor.</remarks>
    virtual pos_type getpos(std::ios_base::openmode direction) const = 0;

    /// <summary>
    /// Gets the size of the stream, if known. Calls to <c>has_size</c> will determine whether
    /// the result of <c>size</c> can be relied on.
    /// </summary>
    virtual utility::size64_t size() const = 0;

    /// <summary>
    /// Seeks to the given position.
    /// </summary>
    /// <param name="pos">The offset from the beginning of the stream.</param>
    /// <param name="direction">The I/O direction to seek (see remarks).</param>
    /// <returns>The position. EOF if the operation fails.</returns>
    /// <remarks>Some streams may have separate write and read cursors. For such streams, the direction parameter
    /// defines whether to move the read or the write cursor.</remarks>
    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode direction) = 0;

    /// <summary>
    /// Seeks to a position given by a relative offset.
    /// </summary>
    /// <param name="offset">The relative position to seek to</param>
    /// <param name="way">The starting point (beginning, end, current) for the seek.</param>
    /// <param name="mode">The I/O direction to seek (see remarks)</param>
    /// <returns>The position. EOF if the operation fails.</returns>
    /// <remarks>Some streams may have separate write and read cursors.
    ///          For such streams, the mode parameter defines whether to move the read or the write cursor.</remarks>
    virtual pos_type seekoff(off_type offset, std::ios_base::seekdir way, std::ios_base::openmode mode) = 0;

    /// <summary>
    /// For output streams, flush any internally buffered data to the underlying medium.
    /// </summary>
    /// <returns>A <c>task</c> that returns <c>true</c> if the sync succeeds, <c>false</c> if not.</returns>
    virtual pplx::task<void> sync() = 0;

    //
    // Efficient read and write.
    //
    // The following routines are intended to be used for more efficient, copy-free, reading and
    // writing of data from/to the stream. Rather than having the caller provide a buffer into which
    // data is written or from which it is read, the stream buffer provides a pointer directly to the
    // internal data blocks that it is using. Since not all stream buffers use internal data structures
    // to copy data, the functions may not be supported by all. An application that wishes to use this
    // functionality should therefore first try them and check for failure to support. If there is
    // such failure, the application should fall back on the copying interfaces (putn / getn)
    //

    /// <summary>
    /// Allocates a contiguous memory block and returns it.
    /// </summary>
    /// <param name="count">The number of characters to allocate.</param>
    /// <returns>A pointer to a block to write to, null if the stream buffer implementation does not support
    /// alloc/commit.</returns>
    virtual _CharType* alloc(_In_ size_t count) = 0;

    /// <summary>
    /// Submits a block already allocated by the stream buffer.
    /// </summary>
    /// <param name="count">The number of characters to be committed.</param>
    virtual void commit(_In_ size_t count) = 0;

    /// <summary>
    /// Gets a pointer to the next already allocated contiguous block of data.
    /// </summary>
    /// <param name="ptr">A reference to a pointer variable that will hold the address of the block on success.</param>
    /// <param name="count">The number of contiguous characters available at the address in 'ptr'.</param>
    /// <returns><c>true</c> if the operation succeeded, <c>false</c> otherwise.</returns>
    /// <remarks>
    /// A return of false does not necessarily indicate that a subsequent read operation would fail, only that
    /// there is no block to return immediately or that the stream buffer does not support the operation.
    /// The stream buffer may not de-allocate the block until <see cref="::release method" /> is called.
    /// If the end of the stream is reached, the function will return <c>true</c>, a null pointer, and a count of zero;
    /// a subsequent read will not succeed.
    /// </remarks>
    virtual bool acquire(_Out_ _CharType*& ptr, _Out_ size_t& count) = 0;

    /// <summary>
    /// Releases a block of data acquired using <see cref="::acquire method"/>. This frees the stream buffer to
    /// de-allocate the memory, if it so desires. Move the read position ahead by the count.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be released.</param>
    /// <param name="count">The number of characters that were read.</param>
    virtual void release(_Out_writes_(count) _CharType* ptr, _In_ size_t count) = 0;

    /// <summary>
    /// Retrieves the stream buffer exception_ptr if it has been set.
    /// </summary>
    /// <returns>Pointer to the exception, if it has been set; otherwise, <c>nullptr</c> will be returned</returns>
    virtual std::exception_ptr exception() const = 0;
};

template<typename _CharType>
class streambuf_state_manager : public basic_streambuf<_CharType>,
                                public std::enable_shared_from_this<streambuf_state_manager<_CharType>>
{
public:
    typedef typename details::basic_streambuf<_CharType>::traits traits;
    typedef typename details::basic_streambuf<_CharType>::int_type int_type;
    typedef typename details::basic_streambuf<_CharType>::pos_type pos_type;
    typedef typename details::basic_streambuf<_CharType>::off_type off_type;

    /// <summary>
    /// <c>can_read</c> is used to determine whether a stream buffer will support read operations (get).
    /// </summary>
    virtual bool can_read() const { return m_stream_can_read; }

    /// <summary>
    /// <c>can_write</c> is used to determine whether a stream buffer will support write operations (put).
    /// </summary>
    virtual bool can_write() const { return m_stream_can_write; }

    /// <summary>
    /// Checks if the stream buffer is open.
    /// </summary>
    /// <remarks>No separation is made between open for reading and open for writing.</remarks>
    virtual bool is_open() const { return can_read() || can_write(); }

    /// <summary>
    /// Closes the stream buffer, preventing further read or write operations.
    /// </summary>
    /// <param name="mode">The I/O mode (in or out) to close for.</param>
    virtual pplx::task<void> close(std::ios_base::openmode mode = std::ios_base::in | std::ios_base::out)
    {
        pplx::task<void> closeOp = pplx::task_from_result();

        if (mode & std::ios_base::in && can_read())
        {
            closeOp = _close_read();
        }

        // After the flush_internal task completed, "this" object may have been destroyed,
        // accessing the members is invalid, use shared_from_this to avoid access violation exception.
        auto this_ptr = std::static_pointer_cast<streambuf_state_manager>(this->shared_from_this());

        if (mode & std::ios_base::out && can_write())
        {
            if (closeOp.is_done())
                closeOp = closeOp && _close_write().then([this_ptr] {}); // passing down exceptions from closeOp
            else
                closeOp = closeOp.then([this_ptr] { return this_ptr->_close_write().then([this_ptr] {}); });
        }

        return closeOp;
    }

    /// <summary>
    /// Closes the stream buffer with an exception.
    /// </summary>
    /// <param name="mode">The I/O mode (in or out) to close for.</param>
    /// <param name="eptr">Pointer to the exception.</param>
    virtual pplx::task<void> close(std::ios_base::openmode mode, std::exception_ptr eptr)
    {
        if (m_currentException == nullptr) m_currentException = eptr;
        return close(mode);
    }

    /// <summary>
    /// <c>is_eof</c> is used to determine whether a read head has reached the end of the buffer.
    /// </summary>
    virtual bool is_eof() const { return m_stream_read_eof; }

    /// <summary>
    /// Writes a single character to the stream.
    /// </summary>
    /// <param name="ch">The character to write</param>
    /// <returns>The value of the character. EOF if the write operation fails</returns>
    virtual pplx::task<int_type> putc(_CharType ch)
    {
        if (!can_write()) return create_exception_checked_value_task<int_type>(traits::eof());

        return create_exception_checked_task<int_type>(_putc(ch), [](int_type) {
            return false; // no EOF for write
        });
    }

    /// <summary>
    /// Writes a number of characters to the stream.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be written.</param>
    /// <param name="count">The number of characters to write.</param>
    /// <returns>The number of characters actually written, either 'count' or 0.</returns>
    CASABLANCA_DEPRECATED("This API in some cases performs a copy. It is deprecated and will be removed in a future "
                          "release. Use putn_nocopy instead.")
    virtual pplx::task<size_t> putn(const _CharType* ptr, size_t count)
    {
        if (!can_write()) return create_exception_checked_value_task<size_t>(0);
        if (count == 0) return pplx::task_from_result<size_t>(0);

        return create_exception_checked_task<size_t>(_putn(ptr, count, true), [](size_t) {
            return false; // no EOF for write
        });
    }

    /// <summary>
    /// Writes a number of characters to the stream. Note: callers must make sure the data to be written is valid until
    /// the returned task completes.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be written.</param>
    /// <param name="count">The number of characters to write.</param>
    /// <returns>A <c>task</c> that holds the number of characters actually written, either 'count' or 0.</returns>
    virtual pplx::task<size_t> putn_nocopy(const _CharType* ptr, size_t count)
    {
        if (!can_write()) return create_exception_checked_value_task<size_t>(0);
        if (count == 0) return pplx::task_from_result<size_t>(0);

        return create_exception_checked_task<size_t>(_putn(ptr, count), [](size_t) {
            return false; // no EOF for write
        });
    }

    /// <summary>
    /// Reads a single character from the stream and advances the read position.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails.</returns>
    virtual pplx::task<int_type> bumpc()
    {
        if (!can_read())
            return create_exception_checked_value_task<int_type>(streambuf_state_manager<_CharType>::traits::eof());

        return create_exception_checked_task<int_type>(
            _bumpc(), [](int_type val) { return val == streambuf_state_manager<_CharType>::traits::eof(); });
    }

    /// <summary>
    /// Reads a single character from the stream and advances the read position.
    /// </summary>
    /// <returns>The value of the character. <c>-1</c> if the read fails. <c>-2</c> if an asynchronous read is
    /// required</returns> <remarks>This is a synchronous operation, but is guaranteed to never block.</remarks>
    virtual int_type sbumpc()
    {
        if (!(m_currentException == nullptr)) std::rethrow_exception(m_currentException);
        if (!can_read()) return traits::eof();
        return check_sync_read_eof(_sbumpc());
    }

    /// <summary>
    /// Reads a single character from the stream without advancing the read position.
    /// </summary>
    /// <returns>The value of the byte. EOF if the read fails.</returns>
    virtual pplx::task<int_type> getc()
    {
        if (!can_read()) return create_exception_checked_value_task<int_type>(traits::eof());

        return create_exception_checked_task<int_type>(
            _getc(), [](int_type val) { return val == streambuf_state_manager<_CharType>::traits::eof(); });
    }

    /// <summary>
    /// Reads a single character from the stream without advancing the read position.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails. <see cref="::requires_async method" /> if an
    /// asynchronous read is required</returns> <remarks>This is a synchronous operation, but is guaranteed to never
    /// block.</remarks>
    virtual int_type sgetc()
    {
        if (!(m_currentException == nullptr)) std::rethrow_exception(m_currentException);
        if (!can_read()) return traits::eof();
        return check_sync_read_eof(_sgetc());
    }

    /// <summary>
    /// Advances the read position, then returns the next character without advancing again.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails.</returns>
    virtual pplx::task<int_type> nextc()
    {
        if (!can_read()) return create_exception_checked_value_task<int_type>(traits::eof());

        return create_exception_checked_task<int_type>(
            _nextc(), [](int_type val) { return val == streambuf_state_manager<_CharType>::traits::eof(); });
    }

    /// <summary>
    /// Retreats the read position, then returns the current character without advancing.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails. <see cref="::requires_async method" /> if an
    /// asynchronous read is required</returns>
    virtual pplx::task<int_type> ungetc()
    {
        if (!can_read()) return create_exception_checked_value_task<int_type>(traits::eof());

        return create_exception_checked_task<int_type>(_ungetc(), [](int_type) { return false; });
    }

    /// <summary>
    /// Reads up to a given number of characters from the stream.
    /// </summary>
    /// <param name="ptr">The address of the target memory area.</param>
    /// <param name="count">The maximum number of characters to read.</param>
    /// <returns>The number of characters read. O if the end of the stream is reached.</returns>
    virtual pplx::task<size_t> getn(_Out_writes_(count) _CharType* ptr, _In_ size_t count)
    {
        if (!can_read()) return create_exception_checked_value_task<size_t>(0);
        if (count == 0) return pplx::task_from_result<size_t>(0);

        return create_exception_checked_task<size_t>(_getn(ptr, count), [](size_t val) { return val == 0; });
    }

    /// <summary>
    /// Copies up to a given number of characters from the stream, synchronously.
    /// </summary>
    /// <param name="ptr">The address of the target memory area.</param>
    /// <param name="count">The maximum number of characters to read.</param>
    /// <returns>The number of characters copied. O if the end of the stream is reached or an asynchronous read is
    /// required.</returns> <remarks>This is a synchronous operation, but is guaranteed to never block.</remarks>
    virtual size_t scopy(_Out_writes_(count) _CharType* ptr, _In_ size_t count)
    {
        if (!(m_currentException == nullptr)) std::rethrow_exception(m_currentException);
        if (!can_read()) return 0;

        return _scopy(ptr, count);
    }

    /// <summary>
    /// For output streams, flush any internally buffered data to the underlying medium.
    /// </summary>
    /// <returns><c>true</c> if the flush succeeds, <c>false</c> if not</returns>
    virtual pplx::task<void> sync()
    {
        if (!can_write())
        {
            if (m_currentException == nullptr)
                return pplx::task_from_result();
            else
                return pplx::task_from_exception<void>(m_currentException);
        }
        return create_exception_checked_task<bool>(_sync(), [](bool) { return false; }).then([](bool) {});
    }

    /// <summary>
    /// Retrieves the stream buffer exception_ptr if it has been set.
    /// </summary>
    /// <returns>Pointer to the exception, if it has been set; otherwise, <c>nullptr</c> will be returned.</returns>
    virtual std::exception_ptr exception() const { return m_currentException; }

    /// <summary>
    /// Allocates a contiguous memory block and returns it.
    /// </summary>
    /// <param name="count">The number of characters to allocate.</param>
    /// <returns>A pointer to a block to write to, null if the stream buffer implementation does not support
    /// alloc/commit.</returns> <remarks>This is intended as an advanced API to be used only when it is important to
    /// avoid extra copies.</remarks>
    _CharType* alloc(size_t count)
    {
        if (m_alloced)
            throw std::logic_error(
                "The buffer is already allocated, this maybe caused by overlap of stream read or write");

        _CharType* alloc_result = _alloc(count);

        if (alloc_result) m_alloced = true;

        return alloc_result;
    }

    /// <summary>
    /// Submits a block already allocated by the stream buffer.
    /// </summary>
    /// <param name="count">The number of characters to be committed.</param>
    /// <remarks>This is intended as an advanced API to be used only when it is important to avoid extra
    /// copies.</remarks>
    void commit(size_t count)
    {
        if (!m_alloced) throw std::logic_error("The buffer needs to allocate first");

        _commit(count);
        m_alloced = false;
    }

public:
    virtual bool can_seek() const = 0;
    virtual bool has_size() const = 0;
    virtual utility::size64_t size() const { return 0; }
    virtual size_t buffer_size(std::ios_base::openmode direction = std::ios_base::in) const = 0;
    virtual void set_buffer_size(size_t size, std::ios_base::openmode direction = std::ios_base::in) = 0;
    virtual size_t in_avail() const = 0;
    virtual pos_type getpos(std::ios_base::openmode direction) const = 0;
    virtual pos_type seekpos(pos_type pos, std::ios_base::openmode direction) = 0;
    virtual pos_type seekoff(off_type offset, std::ios_base::seekdir way, std::ios_base::openmode mode) = 0;
    virtual bool acquire(_Out_writes_(count) _CharType*& ptr, _In_ size_t& count) = 0;
    virtual void release(_Out_writes_(count) _CharType* ptr, _In_ size_t count) = 0;

protected:
    virtual pplx::task<int_type> _putc(_CharType ch) = 0;

    // This API is only needed for file streams and until we remove the deprecated stream buffer putn overload.
    virtual pplx::task<size_t> _putn(const _CharType* ptr, size_t count, bool)
    {
        // Default to no copy, only the file streams API overloads and performs a copy.
        return _putn(ptr, count);
    }
    virtual pplx::task<size_t> _putn(const _CharType* ptr, size_t count) = 0;

    virtual pplx::task<int_type> _bumpc() = 0;
    virtual int_type _sbumpc() = 0;
    virtual pplx::task<int_type> _getc() = 0;
    virtual int_type _sgetc() = 0;
    virtual pplx::task<int_type> _nextc() = 0;
    virtual pplx::task<int_type> _ungetc() = 0;
    virtual pplx::task<size_t> _getn(_Out_writes_(count) _CharType* ptr, _In_ size_t count) = 0;
    virtual size_t _scopy(_Out_writes_(count) _CharType* ptr, _In_ size_t count) = 0;
    virtual pplx::task<bool> _sync() = 0;
    virtual _CharType* _alloc(size_t count) = 0;
    virtual void _commit(size_t count) = 0;

    /// <summary>
    /// The real read head close operation, implementation should override it if there is any resource to be released.
    /// </summary>
    virtual pplx::task<void> _close_read()
    {
        m_stream_can_read = false;
        return pplx::task_from_result();
    }

    /// <summary>
    /// The real write head close operation, implementation should override it if there is any resource to be released.
    /// </summary>
    virtual pplx::task<void> _close_write()
    {
        m_stream_can_write = false;
        return pplx::task_from_result();
    }

protected:
    streambuf_state_manager(std::ios_base::openmode mode)
    {
        m_stream_can_read = (mode & std::ios_base::in) != 0;
        m_stream_can_write = (mode & std::ios_base::out) != 0;
        m_stream_read_eof = false;
        m_alloced = false;
    }

    std::exception_ptr m_currentException;
    // The in/out mode for the buffer
    std::atomic<bool> m_stream_can_read;
    std::atomic<bool> m_stream_can_write;
    std::atomic<bool> m_stream_read_eof;
    std::atomic<bool> m_alloced;

private:
    template<typename _CharType1>
    pplx::task<_CharType1> create_exception_checked_value_task(const _CharType1& val) const
    {
        if (this->exception() == nullptr)
            return pplx::task_from_result<_CharType1>(static_cast<_CharType1>(val));
        else
            return pplx::task_from_exception<_CharType1>(this->exception());
    }

    // Set exception and eof states for async read
    template<typename _CharType1>
    pplx::task<_CharType1> create_exception_checked_task(pplx::task<_CharType1> result,
                                                         std::function<bool(_CharType1)> eof_test,
                                                         std::ios_base::openmode mode = std::ios_base::in |
                                                                                        std::ios_base::out)
    {
        auto thisPointer = this->shared_from_this();

        auto func1 = [=](pplx::task<_CharType1> t1) -> pplx::task<_CharType1> {
            try
            {
                thisPointer->m_stream_read_eof = eof_test(t1.get());
            }
            catch (...)
            {
                thisPointer->close(mode, std::current_exception()).get();
                return pplx::task_from_exception<_CharType1>(thisPointer->exception(), pplx::task_options());
            }
            if (thisPointer->m_stream_read_eof && !(thisPointer->exception() == nullptr))
                return pplx::task_from_exception<_CharType1>(thisPointer->exception(), pplx::task_options());
            return t1;
        };

        if (result.is_done())
        {
            // If the data is already available, we should avoid scheduling a continuation, so we do it inline.
            return func1(result);
        }
        else
        {
            return result.then(func1);
        }
    }

    // Set eof states for sync read
    int_type check_sync_read_eof(int_type ch)
    {
        m_stream_read_eof = ch == traits::eof();
        return ch;
    }
};

} // namespace details

// Forward declarations
template<typename _CharType>
class basic_istream;
template<typename _CharType>
class basic_ostream;

/// <summary>
/// Reference-counted stream buffer.
/// </summary>
/// <typeparam name="_CharType">
/// The data type of the basic element of the <c>streambuf.</c>
/// </typeparam>
/// <typeparam name="_CharType2">
/// The data type of the basic element of the <c>streambuf.</c>
/// </typeparam>
template<typename _CharType>
class streambuf : public details::basic_streambuf<_CharType>
{
public:
    typedef typename details::basic_streambuf<_CharType>::traits traits;
    typedef typename details::basic_streambuf<_CharType>::int_type int_type;
    typedef typename details::basic_streambuf<_CharType>::pos_type pos_type;
    typedef typename details::basic_streambuf<_CharType>::off_type off_type;
    typedef typename details::basic_streambuf<_CharType>::char_type char_type;

    template<typename _CharType2>
    friend class streambuf;

    /// <summary>
    /// Constructor.
    /// </summary>
    /// <param name="ptr">A pointer to the concrete stream buffer implementation.</param>
    streambuf(_In_ const std::shared_ptr<details::basic_streambuf<_CharType>>& ptr) : m_buffer(ptr) {}

    /// <summary>
    /// Default constructor.
    /// </summary>
    streambuf() {}

    /// <summary>
    /// Converter Constructor.
    /// </summary>
    /// <typeparam name="AlterCharType">
    /// The data type of the basic element of the source <c>streambuf</c>.
    /// </typeparam>
    /// <param name="other">The source buffer to be converted.</param>
    template<typename AlterCharType>
    streambuf(const streambuf<AlterCharType>& other)
        : m_buffer(std::static_pointer_cast<details::basic_streambuf<_CharType>>(
              std::static_pointer_cast<void>(other.m_buffer)))
    {
        static_assert(std::is_same<pos_type, typename details::basic_streambuf<AlterCharType>::pos_type>::value &&
                          std::is_same<off_type, typename details::basic_streambuf<AlterCharType>::off_type>::value &&
                          std::is_integral<_CharType>::value && std::is_integral<AlterCharType>::value &&
                          std::is_integral<int_type>::value &&
                          std::is_integral<typename details::basic_streambuf<AlterCharType>::int_type>::value &&
                          sizeof(_CharType) == sizeof(AlterCharType) &&
                          sizeof(int_type) == sizeof(typename details::basic_streambuf<AlterCharType>::int_type),
                      "incompatible stream character types");
    }

    /// <summary>
    /// Constructs an input stream head for this stream buffer.
    /// </summary>
    /// <returns><c>basic_istream</c>.</returns>
    concurrency::streams::basic_istream<_CharType> create_istream() const
    {
        if (!can_read()) throw std::runtime_error("stream buffer not set up for input of data");
        return concurrency::streams::basic_istream<_CharType>(*this);
    }

    /// <summary>
    /// Constructs an output stream for this stream buffer.
    /// </summary>
    /// <returns>basic_ostream</returns>
    concurrency::streams::basic_ostream<_CharType> create_ostream() const
    {
        if (!can_write()) throw std::runtime_error("stream buffer not set up for output of data");
        return concurrency::streams::basic_ostream<_CharType>(*this);
    }

    /// <summary>
    /// Checks if the stream buffer has been initialized or not.
    /// </summary>
    operator bool() const { return (bool)m_buffer; }

    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~streambuf() {}

    const std::shared_ptr<details::basic_streambuf<_CharType>>& get_base() const
    {
        if (!m_buffer)
        {
            throw std::invalid_argument("Invalid streambuf object");
        }

        return m_buffer;
    }

    /// <summary>
    /// <c>can_read</c> is used to determine whether a stream buffer will support read operations (get).
    /// </summary>
    virtual bool can_read() const { return get_base()->can_read(); }

    /// <summary>
    /// <c>can_write</c> is used to determine whether a stream buffer will support write operations (put).
    /// </summary>
    virtual bool can_write() const { return get_base()->can_write(); }

    /// <summary>
    /// <c>can_seek</c> is used to determine whether a stream buffer supports seeking.
    /// </summary>
    /// <returns>True if seeking is supported, false otherwise.</returns>
    virtual bool can_seek() const { return get_base()->can_seek(); }

    /// <summary>
    /// <c>has_size</c> is used to determine whether a stream buffer supports size().
    /// </summary>
    /// <returns>True if the <c>size</c> API is supported, false otherwise.</returns>
    virtual bool has_size() const { return get_base()->has_size(); }

    /// <summary>
    /// Gets the total number of characters in the stream buffer, if known. Calls to <c>has_size</c> will determine
    /// whether the result of <c>size</c> can be relied on.
    /// </summary>
    /// <returns>The total number of characters in the stream buffer.</returns>
    virtual utility::size64_t size() const { return get_base()->size(); }

    /// <summary>
    /// Gets the stream buffer size, if one has been set.
    /// </summary>
    /// <param name="direction">The direction of buffering (in or out)</param>
    /// <returns>The size of the internal buffer (for the given direction).</returns>
    /// <remarks>An implementation that does not support buffering will always return 0.</remarks>
    virtual size_t buffer_size(std::ios_base::openmode direction = std::ios_base::in) const
    {
        return get_base()->buffer_size(direction);
    }

    /// <summary>
    /// Sets the stream buffer implementation to buffer or not buffer.
    /// </summary>
    /// <param name="size">The size to use for internal buffering, 0 if no buffering should be done.</param>
    /// <param name="direction">The direction of buffering (in or out)</param>
    /// <remarks>An implementation that does not support buffering will silently ignore calls to this function and it
    /// will not have any effect on what is returned by subsequent calls to <see cref="::buffer_size method"
    /// />.</remarks>
    virtual void set_buffer_size(size_t size, std::ios_base::openmode direction = std::ios_base::in)
    {
        get_base()->set_buffer_size(size, direction);
    }

    /// <summary>
    /// For any input stream, <c>in_avail</c> returns the number of characters that are immediately available
    /// to be consumed without blocking. May be used in conjunction with <cref="::sbumpc method"/> to read data without
    /// incurring the overhead of using tasks.
    /// </summary>
    /// <returns>Number of characters that are ready to read.</returns>
    virtual size_t in_avail() const { return get_base()->in_avail(); }

    /// <summary>
    /// Checks if the stream buffer is open.
    /// </summary>
    /// <remarks>No separation is made between open for reading and open for writing.</remarks>
    /// <returns>True if the stream buffer is open for reading or writing, false otherwise.</returns>
    virtual bool is_open() const { return get_base()->is_open(); }

    /// <summary>
    /// <c>is_eof</c> is used to determine whether a read head has reached the end of the buffer.
    /// </summary>
    /// <returns>True if at the end of the buffer, false otherwise.</returns>
    virtual bool is_eof() const { return get_base()->is_eof(); }

    /// <summary>
    /// Closes the stream buffer, preventing further read or write operations.
    /// </summary>
    /// <param name="mode">The I/O mode (in or out) to close for.</param>
    virtual pplx::task<void> close(std::ios_base::openmode mode = (std::ios_base::in | std::ios_base::out))
    {
        // We preserve the check here to workaround a Dev10 compiler crash
        auto buffer = get_base();
        return buffer ? buffer->close(mode) : pplx::task_from_result();
    }

    /// <summary>
    /// Closes the stream buffer with an exception.
    /// </summary>
    /// <param name="mode">The I/O mode (in or out) to close for.</param>
    /// <param name="eptr">Pointer to the exception.</param>
    virtual pplx::task<void> close(std::ios_base::openmode mode, std::exception_ptr eptr)
    {
        // We preserve the check here to workaround a Dev10 compiler crash
        auto buffer = get_base();
        return buffer ? buffer->close(mode, eptr) : pplx::task_from_result();
    }

    /// <summary>
    /// Writes a single character to the stream.
    /// </summary>
    /// <param name="ch">The character to write</param>
    /// <returns>The value of the character. EOF if the write operation fails</returns>
    virtual pplx::task<int_type> putc(_CharType ch) { return get_base()->putc(ch); }

    /// <summary>
    /// Allocates a contiguous memory block and returns it.
    /// </summary>
    /// <param name="count">The number of characters to allocate.</param>
    /// <returns>A pointer to a block to write to, null if the stream buffer implementation does not support
    /// alloc/commit.</returns>
    virtual _CharType* alloc(size_t count) { return get_base()->alloc(count); }

    /// <summary>
    /// Submits a block already allocated by the stream buffer.
    /// </summary>
    /// <param name="count">The number of characters to be committed.</param>
    virtual void commit(size_t count) { get_base()->commit(count); }

    /// <summary>
    /// Gets a pointer to the next already allocated contiguous block of data.
    /// </summary>
    /// <param name="ptr">A reference to a pointer variable that will hold the address of the block on success.</param>
    /// <param name="count">The number of contiguous characters available at the address in 'ptr'.</param>
    /// <returns><c>true</c> if the operation succeeded, <c>false</c> otherwise.</returns>
    /// <remarks>
    /// A return of false does not necessarily indicate that a subsequent read operation would fail, only that
    /// there is no block to return immediately or that the stream buffer does not support the operation.
    /// The stream buffer may not de-allocate the block until <see cref="::release method" /> is called.
    /// If the end of the stream is reached, the function will return <c>true</c>, a null pointer, and a count of zero;
    /// a subsequent read will not succeed.
    /// </remarks>
    virtual bool acquire(_Out_ _CharType*& ptr, _Out_ size_t& count)
    {
        ptr = nullptr;
        count = 0;
        return get_base()->acquire(ptr, count);
    }

    /// <summary>
    /// Releases a block of data acquired using <see cref="::acquire method"/>. This frees the stream buffer to
    /// de-allocate the memory, if it so desires. Move the read position ahead by the count.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be released.</param>
    /// <param name="count">The number of characters that were read.</param>
    virtual void release(_Out_writes_(count) _CharType* ptr, _In_ size_t count) { get_base()->release(ptr, count); }

    /// <summary>
    /// Writes a number of characters to the stream.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be written.</param>
    /// <param name="count">The number of characters to write.</param>
    /// <returns>The number of characters actually written, either 'count' or 0.</returns>
    CASABLANCA_DEPRECATED("This API in some cases performs a copy. It is deprecated and will be removed in a future "
                          "release. Use putn_nocopy instead.")
    virtual pplx::task<size_t> putn(const _CharType* ptr, size_t count) { return get_base()->putn(ptr, count); }

    /// <summary>
    /// Writes a number of characters to the stream. Note: callers must make sure the data to be written is valid until
    /// the returned task completes.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be written.</param>
    /// <param name="count">The number of characters to write.</param>
    /// <returns>The number of characters actually written, either 'count' or 0.</returns>
    virtual pplx::task<size_t> putn_nocopy(const _CharType* ptr, size_t count)
    {
        return get_base()->putn_nocopy(ptr, count);
    }

    /// <summary>
    /// Reads a single character from the stream and advances the read position.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails.</returns>
    virtual pplx::task<int_type> bumpc() { return get_base()->bumpc(); }

    /// <summary>
    /// Reads a single character from the stream and advances the read position.
    /// </summary>
    /// <returns>The value of the character. <c>-1</c> if the read fails. <c>-2</c> if an asynchronous read is
    /// required</returns> <remarks>This is a synchronous operation, but is guaranteed to never block.</remarks>
    virtual typename details::basic_streambuf<_CharType>::int_type sbumpc() { return get_base()->sbumpc(); }

    /// <summary>
    /// Reads a single character from the stream without advancing the read position.
    /// </summary>
    /// <returns>The value of the byte. EOF if the read fails.</returns>
    virtual pplx::task<int_type> getc() { return get_base()->getc(); }

    /// <summary>
    /// Reads a single character from the stream without advancing the read position.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails. <see cref="::requires_async method" /> if an
    /// asynchronous read is required</returns> <remarks>This is a synchronous operation, but is guaranteed to never
    /// block.</remarks>
    virtual typename details::basic_streambuf<_CharType>::int_type sgetc() { return get_base()->sgetc(); }

    /// <summary>
    /// Advances the read position, then returns the next character without advancing again.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails.</returns>
    pplx::task<int_type> nextc() { return get_base()->nextc(); }

    /// <summary>
    /// Retreats the read position, then returns the current character without advancing.
    /// </summary>
    /// <returns>The value of the character. EOF if the read fails. <see cref="::requires_async method" /> if an
    /// asynchronous read is required</returns>
    pplx::task<int_type> ungetc() { return get_base()->ungetc(); }

    /// <summary>
    /// Reads up to a given number of characters from the stream.
    /// </summary>
    /// <param name="ptr">The address of the target memory area.</param>
    /// <param name="count">The maximum number of characters to read.</param>
    /// <returns>The number of characters read. O if the end of the stream is reached.</returns>
    virtual pplx::task<size_t> getn(_Out_writes_(count) _CharType* ptr, _In_ size_t count)
    {
        return get_base()->getn(ptr, count);
    }

    /// <summary>
    /// Copies up to a given number of characters from the stream, synchronously.
    /// </summary>
    /// <param name="ptr">The address of the target memory area.</param>
    /// <param name="count">The maximum number of characters to read.</param>
    /// <returns>The number of characters copied. O if the end of the stream is reached or an asynchronous read is
    /// required.</returns> <remarks>This is a synchronous operation, but is guaranteed to never block.</remarks>
    virtual size_t scopy(_Out_writes_(count) _CharType* ptr, _In_ size_t count)
    {
        return get_base()->scopy(ptr, count);
    }

    /// <summary>
    /// Gets the current read or write position in the stream.
    /// </summary>
    /// <param name="direction">The I/O direction to seek (see remarks)</param>
    /// <returns>The current position. EOF if the operation fails.</returns>
    /// <remarks>Some streams may have separate write and read cursors.
    ///          For such streams, the direction parameter defines whether to move the read or the write
    ///          cursor.</remarks>
    virtual typename details::basic_streambuf<_CharType>::pos_type getpos(std::ios_base::openmode direction) const
    {
        return get_base()->getpos(direction);
    }

    /// <summary>
    /// Seeks to the given position.
    /// </summary>
    /// <param name="pos">The offset from the beginning of the stream.</param>
    /// <param name="direction">The I/O direction to seek (see remarks).</param>
    /// <returns>The position. EOF if the operation fails.</returns>
    /// <remarks>Some streams may have separate write and read cursors. For such streams, the direction parameter
    /// defines whether to move the read or the write cursor.</remarks>
    virtual typename details::basic_streambuf<_CharType>::pos_type seekpos(
        typename details::basic_streambuf<_CharType>::pos_type pos, std::ios_base::openmode direction)
    {
        return get_base()->seekpos(pos, direction);
    }

    /// <summary>
    /// Seeks to a position given by a relative offset.
    /// </summary>
    /// <param name="offset">The relative position to seek to</param>
    /// <param name="way">The starting point (beginning, end, current) for the seek.</param>
    /// <param name="mode">The I/O direction to seek (see remarks)</param>
    /// <returns>The position. EOF if the operation fails.</returns>
    /// <remarks>Some streams may have separate write and read cursors.
    ///          For such streams, the mode parameter defines whether to move the read or the write cursor.</remarks>
    virtual typename details::basic_streambuf<_CharType>::pos_type seekoff(
        typename details::basic_streambuf<_CharType>::off_type offset,
        std::ios_base::seekdir way,
        std::ios_base::openmode mode)
    {
        return get_base()->seekoff(offset, way, mode);
    }

    /// <summary>
    /// For output streams, flush any internally buffered data to the underlying medium.
    /// </summary>
    /// <returns><c>true</c> if the flush succeeds, <c>false</c> if not</returns>
    virtual pplx::task<void> sync() { return get_base()->sync(); }

    /// <summary>
    /// Retrieves the stream buffer exception_ptr if it has been set.
    /// </summary>
    /// <returns>Pointer to the exception, if it has been set; otherwise, <c>nullptr</c> will be returned</returns>
    virtual std::exception_ptr exception() const { return get_base()->exception(); }

private:
    std::shared_ptr<details::basic_streambuf<_CharType>> m_buffer;
};

} // namespace streams
} // namespace Concurrency
