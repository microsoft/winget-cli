/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * This file defines a stream buffer that is based on a raw pointer and block size. Unlike a vector-based
 * stream buffer, the buffer cannot be expanded or contracted, it has a fixed capacity.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

#ifndef CASA_RAWPTR_STREAMS_H
#define CASA_RAWPTR_STREAMS_H

#include "cpprest/astreambuf.h"
#include "cpprest/streams.h"
#include "pplx/pplxtasks.h"
#include <algorithm>
#include <iterator>
#include <queue>
#include <vector>

namespace Concurrency
{
namespace streams
{
// Forward declarations
template<typename _CharType>
class rawptr_buffer;

namespace details
{
/// <summary>
/// The basic_rawptr_buffer class serves as a memory-based steam buffer that supports both writing and reading
/// sequences of characters to and from a fixed-size block.
/// </summary>
template<typename _CharType>
class basic_rawptr_buffer : public streams::details::streambuf_state_manager<_CharType>
{
public:
    typedef _CharType char_type;

    typedef typename basic_streambuf<_CharType>::traits traits;
    typedef typename basic_streambuf<_CharType>::int_type int_type;
    typedef typename basic_streambuf<_CharType>::pos_type pos_type;
    typedef typename basic_streambuf<_CharType>::off_type off_type;

    /// <summary>
    /// Constructor
    /// </summary>
    basic_rawptr_buffer()
        : streambuf_state_manager<_CharType>(std::ios_base::in | std::ios_base::out)
        , m_data(nullptr)
        , m_current_position(0)
        , m_size(0)
    {
    }

    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~basic_rawptr_buffer()
    {
        this->_close_read();
        this->_close_write();
    }

protected:
    /// <summary>
    /// can_seek is used to determine whether a stream buffer supports seeking.
    /// </summary>
    virtual bool can_seek() const { return this->is_open(); }

    /// <summary>
    /// <c>has_size<c/> is used to determine whether a stream buffer supports size().
    /// </summary>
    virtual bool has_size() const { return this->is_open(); }

    /// <summary>
    /// Gets the size of the stream, if known. Calls to <c>has_size</c> will determine whether
    /// the result of <c>size</c> can be relied on.
    /// </summary>
    virtual utility::size64_t size() const { return utility::size64_t(m_size); }

    /// <summary>
    /// Get the stream buffer size, if one has been set.
    /// </summary>
    /// <param name="direction">The direction of buffering (in or out)</param>
    /// <remarks>An implementation that does not support buffering will always return '0'.</remarks>
    virtual size_t buffer_size(std::ios_base::openmode = std::ios_base::in) const { return 0; }

    /// <summary>
    /// Set the stream buffer implementation to buffer or not buffer.
    /// </summary>
    /// <param name="size">The size to use for internal buffering, 0 if no buffering should be done.</param>
    /// <param name="direction">The direction of buffering (in or out)</param>
    /// <remarks>An implementation that does not support buffering will silently ignore calls to this function and it
    /// will not have
    ///          any effect on what is returned by subsequent calls to buffer_size().</remarks>
    virtual void set_buffer_size(size_t, std::ios_base::openmode = std::ios_base::in) { return; }

    /// <summary>
    /// For any input stream, in_avail returns the number of characters that are immediately available
    /// to be consumed without blocking. May be used in conjunction with <cref="::sbumpc method"/> and sgetn() to
    /// read data without incurring the overhead of using tasks.
    /// </summary>
    virtual size_t in_avail() const
    {
        // See the comment in seek around the restiction that we do not allow read head to
        // seek beyond the current size.
        _ASSERTE(m_current_position <= m_size);

        msl::safeint3::SafeInt<size_t> readhead(m_current_position);
        msl::safeint3::SafeInt<size_t> writeend(m_size);
        return (size_t)(writeend - readhead);
    }

    /// <summary>
    /// Closes the stream buffer, preventing further read or write operations.
    /// </summary>
    /// <param name="mode">The I/O mode (in or out) to close for.</param>
    virtual pplx::task<void> close(std::ios_base::openmode mode)
    {
        if (mode & std::ios_base::in)
        {
            this->_close_read().get(); // Safe to call get() here.
        }

        if (mode & std::ios_base::out)
        {
            this->_close_write().get(); // Safe to call get() here.
        }

        if (!this->can_read() && !this->can_write())
        {
            m_data = nullptr;
        }

        // Exceptions will be propagated out of _close_read or _close_write
        return pplx::task_from_result();
    }

    virtual pplx::task<bool> _sync() { return pplx::task_from_result(true); }

    virtual pplx::task<int_type> _putc(_CharType ch)
    {
        if (m_current_position >= m_size) return pplx::task_from_result<int_type>(traits::eof());
        int_type retVal = (this->write(&ch, 1) == 1) ? static_cast<int_type>(ch) : traits::eof();
        return pplx::task_from_result<int_type>(retVal);
    }

    virtual pplx::task<size_t> _putn(const _CharType* ptr, size_t count)
    {
        msl::safeint3::SafeInt<size_t> newSize = msl::safeint3::SafeInt<size_t>(count) + m_current_position;
        if (newSize > m_size)
            return pplx::task_from_exception<size_t>(
                std::make_exception_ptr(std::runtime_error("Writing past the end of the buffer")));
        return pplx::task_from_result<size_t>(this->write(ptr, count));
    }

    /// <summary>
    /// Allocates a contiguous memory block and returns it.
    /// </summary>
    /// <param name="count">The number of characters to allocate.</param>
    /// <returns>A pointer to a block to write to, null if the stream buffer implementation does not support
    /// alloc/commit.</returns>
    _CharType* _alloc(size_t count)
    {
        if (!this->can_write()) return nullptr;

        msl::safeint3::SafeInt<size_t> readhead(m_current_position);
        msl::safeint3::SafeInt<size_t> writeend(m_size);
        size_t space_left = (size_t)(writeend - readhead);

        if (space_left < count) return nullptr;

        // Let the caller copy the data
        return (_CharType*)(m_data + m_current_position);
    }

    /// <summary>
    /// Submits a block already allocated by the stream buffer.
    /// </summary>
    /// <param name="count">The number of characters to be committed.</param>
    void _commit(size_t actual)
    {
        // Update the write position and satisfy any pending reads
        update_current_position(m_current_position + actual);
    }

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
        count = 0;
        ptr = nullptr;

        if (!this->can_read()) return false;

        count = in_avail();

        if (count > 0)
        {
            ptr = (_CharType*)(m_data + m_current_position);
            return true;
        }
        else
        {
            ptr = nullptr;

            // Can only be open for read OR write, not both. If there is no data then
            // we have reached the end of the stream so indicate such with true.
            return true;
        }
    }

    /// <summary>
    /// Releases a block of data acquired using <see cref="::acquire method"/>. This frees the stream buffer to
    /// de-allocate the memory, if it so desires. Move the read position ahead by the count.
    /// </summary>
    /// <param name="ptr">A pointer to the block of data to be released.</param>
    /// <param name="count">The number of characters that were read.</param>
    virtual void release(_Out_writes_opt_(count) _CharType* ptr, _In_ size_t count)
    {
        if (ptr != nullptr) update_current_position(m_current_position + count);
    }

    virtual pplx::task<size_t> _getn(_Out_writes_(count) _CharType* ptr, _In_ size_t count)
    {
        return pplx::task_from_result(this->read(ptr, count));
    }

    size_t _sgetn(_Out_writes_(count) _CharType* ptr, _In_ size_t count) { return this->read(ptr, count); }

    virtual size_t _scopy(_Out_writes_(count) _CharType* ptr, _In_ size_t count)
    {
        return this->read(ptr, count, false);
    }

    virtual pplx::task<int_type> _bumpc() { return pplx::task_from_result(this->read_byte(true)); }

    virtual int_type _sbumpc() { return this->read_byte(true); }

    virtual pplx::task<int_type> _getc() { return pplx::task_from_result(this->read_byte(false)); }

    int_type _sgetc() { return this->read_byte(false); }

    virtual pplx::task<int_type> _nextc()
    {
        if (m_current_position >= m_size - 1) return pplx::task_from_result(basic_streambuf<_CharType>::traits::eof());

        this->read_byte(true);
        return pplx::task_from_result(this->read_byte(false));
    }

    virtual pplx::task<int_type> _ungetc()
    {
        auto pos = seekoff(-1, std::ios_base::cur, std::ios_base::in);
        if (pos == (pos_type)traits::eof()) return pplx::task_from_result(traits::eof());
        return this->getc();
    }

    /// <summary>
    /// Gets the current read or write position in the stream.
    /// </summary>
    /// <param name="direction">The I/O direction to seek (see remarks)</param>
    /// <returns>The current position. EOF if the operation fails.</returns>
    /// <remarks>Some streams may have separate write and read cursors.
    ///          For such streams, the direction parameter defines whether to move the read or the write
    ///          cursor.</remarks>
    virtual pos_type getpos(std::ios_base::openmode mode) const
    {
        if (((mode & std::ios_base::in) && !this->can_read()) || ((mode & std::ios_base::out) && !this->can_write()))
            return static_cast<pos_type>(traits::eof());

        if (mode == std::ios_base::in)
            return (pos_type)m_current_position;
        else if (mode == std::ios_base::out)
            return (pos_type)m_current_position;
        else
            return (pos_type)traits::eof();
    }

    /// <summary>
    /// Seeks to the given position.
    /// </summary>
    /// <param name="pos">The offset from the beginning of the stream.</param>
    /// <param name="direction">The I/O direction to seek (see remarks).</param>
    /// <returns>The position. EOF if the operation fails.</returns>
    /// <remarks>Some streams may have separate write and read cursors. For such streams, the direction parameter
    /// defines whether to move the read or the write cursor.</remarks>
    virtual pos_type seekpos(pos_type position, std::ios_base::openmode mode)
    {
        pos_type beg(0);
        pos_type end(m_size);

        if (position >= beg)
        {
            auto pos = static_cast<size_t>(position);

            // Read head
            if ((mode & std::ios_base::in) && this->can_read())
            {
                if (position <= end)
                {
                    // We do not allow reads to seek beyond the end or before the start position.
                    update_current_position(pos);
                    return static_cast<pos_type>(m_current_position);
                }
            }

            // Write head
            if ((mode & std::ios_base::out) && this->can_write())
            {
                // Update write head and satisfy read requests if any
                update_current_position(pos);

                return static_cast<pos_type>(m_current_position);
            }
        }

        return static_cast<pos_type>(traits::eof());
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
    virtual pos_type seekoff(off_type offset, std::ios_base::seekdir way, std::ios_base::openmode mode)
    {
        pos_type beg = 0;
        pos_type cur = static_cast<pos_type>(m_current_position);
        pos_type end = static_cast<pos_type>(m_size);

        switch (way)
        {
            case std::ios_base::beg: return seekpos(beg + offset, mode);

            case std::ios_base::cur: return seekpos(cur + offset, mode);

            case std::ios_base::end: return seekpos(end + offset, mode);

            default: return static_cast<pos_type>(traits::eof());
        }
    }

private:
    template<typename _CharType1>
    friend class ::concurrency::streams::rawptr_buffer;

    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="data">The address (pointer to) the memory block.</param>
    /// <param name="size">The memory block size, measured in number of characters.</param>
    basic_rawptr_buffer(const _CharType* data, size_t size)
        : streambuf_state_manager<_CharType>(std::ios_base::in)
        , m_data(const_cast<_CharType*>(data))
        , m_size(size)
        , m_current_position(0)
    {
        validate_mode(std::ios_base::in);
    }

    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="data">The address (pointer to) the memory block.</param>
    /// <param name="size">The memory block size, measured in number of characters.</param>
    /// <param name="mode">The stream mode (in, out, etc.).</param>
    basic_rawptr_buffer(_CharType* data, size_t size, std::ios_base::openmode mode)
        : streambuf_state_manager<_CharType>(mode), m_data(data), m_size(size), m_current_position(0)
    {
        validate_mode(mode);
    }

    static void validate_mode(std::ios_base::openmode mode)
    {
        // Disallow simultaneous use of the stream buffer for writing and reading.
        if ((mode & std::ios_base::in) && (mode & std::ios_base::out))
            throw std::invalid_argument("this combination of modes on raw pointer stream not supported");
    }

    /// <summary>
    /// Determines if the request can be satisfied.
    /// </summary>
    bool can_satisfy(size_t) const
    {
        // We can always satisfy a read, at least partially, unless the
        // read position is at the very end of the buffer.
        return (in_avail() > 0);
    }

    /// <summary>
    /// Reads a byte from the stream and returns it as int_type.
    /// Note: This routine must only be called if can_satisfy() returns true.
    /// </summary>
    int_type read_byte(bool advance = true)
    {
        _CharType value;
        auto read_size = this->read(&value, 1, advance);
        return read_size == 1 ? static_cast<int_type>(value) : traits::eof();
    }

    /// <summary>
    /// Reads up to count characters into ptr and returns the count of characters copied.
    /// The return value (actual characters copied) could be <= count.
    /// Note: This routine must only be called if can_satisfy() returns true.
    /// </summary>
    size_t read(_Out_writes_(count) _CharType* ptr, _In_ size_t count, bool advance = true)
    {
        if (!can_satisfy(count)) return 0;

        msl::safeint3::SafeInt<size_t> request_size(count);
        msl::safeint3::SafeInt<size_t> read_size = request_size.Min(in_avail());

        size_t newPos = m_current_position + read_size;

        auto readBegin = m_data + m_current_position;
        auto readEnd = m_data + newPos;

#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
        // Avoid warning C4996: Use checked iterators under SECURE_SCL
        std::copy(readBegin, readEnd, stdext::checked_array_iterator<_CharType*>(ptr, count));
#else
        std::copy(readBegin, readEnd, ptr);
#endif // _WIN32

        if (advance)
        {
            update_current_position(newPos);
        }

        return (size_t)read_size;
    }

    /// <summary>
    /// Write count characters from the ptr into the stream buffer
    /// </summary>
    size_t write(const _CharType* ptr, size_t count)
    {
        if (!this->can_write() || (count == 0)) return 0;

        msl::safeint3::SafeInt<size_t> newSize = msl::safeint3::SafeInt<size_t>(count) + m_current_position;

        if (newSize > m_size) throw std::runtime_error("Writing past the end of the buffer");

            // Copy the data
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
        // Avoid warning C4996: Use checked iterators under SECURE_SCL
        std::copy(ptr, ptr + count, stdext::checked_array_iterator<_CharType*>(m_data, m_size, m_current_position));
#else
        std::copy(ptr, ptr + count, m_data + m_current_position);
#endif // _WIN32

        // Update write head and satisfy pending reads if any
        update_current_position(newSize);

        return count;
    }

    /// <summary>
    /// Updates the current read or write position
    /// </summary>
    void update_current_position(size_t newPos)
    {
        // The new write head
        m_current_position = newPos;

        _ASSERTE(m_current_position <= m_size);
    }

    // The actual memory block
    _CharType* m_data;

    // The size of the memory block
    size_t m_size;

    // Read/write head
    size_t m_current_position;
};

} // namespace details

/// <summary>
/// The <c>rawptr_buffer</c> class serves as a memory-based stream buffer that supports reading
/// sequences of characters to or from a fixed-size block. Note that it cannot be used simultaneously for reading as
/// well as writing.
/// </summary>
/// <typeparam name="_CharType">
/// The data type of the basic element of the <c>rawptr_buffer</c>.
/// </typeparam>
template<typename _CharType>
class rawptr_buffer : public streambuf<_CharType>
{
public:
    typedef _CharType char_type;

    /// <summary>
    /// Create a rawptr_buffer given a pointer to a memory block and the size of the block.
    /// </summary>
    /// <param name="data">The address (pointer to) the memory block.</param>
    /// <param name="size">The memory block size, measured in number of characters.</param>
    rawptr_buffer(const char_type* data, size_t size)
        : streambuf<char_type>(std::shared_ptr<details::basic_rawptr_buffer<char_type>>(
              new details::basic_rawptr_buffer<char_type>(data, size)))
    {
    }

    /// <summary>
    /// Create a rawptr_buffer given a pointer to a memory block and the size of the block.
    /// </summary>
    /// <param name="data">The address (pointer to) the memory block.</param>
    /// <param name="size">The memory block size, measured in number of characters.</param>
    rawptr_buffer(char_type* data, size_t size, std::ios_base::openmode mode = std::ios::out)
        : streambuf<char_type>(std::shared_ptr<details::basic_rawptr_buffer<char_type>>(
              new details::basic_rawptr_buffer<char_type>(data, size, mode)))
    {
    }

    /// <summary>
    /// Default constructor.
    /// </summary>
    rawptr_buffer() {}
};

/// <summary>
/// The rawptr_stream class is used to create memory-backed streams that support writing or reading
/// sequences of characters to / from a fixed-size block.
/// </summary>
/// <typeparam name="_CharType">
/// The data type of the basic element of the <c>rawptr_stream</c>.
/// </typeparam>
template<typename _CharType>
class rawptr_stream
{
public:
    typedef _CharType char_type;
    typedef rawptr_buffer<_CharType> buffer_type;

    /// <summary>
    /// Create a rawptr-stream given a pointer to a read-only memory block and the size of the block.
    /// </summary>
    /// <param name="data">The address (pointer to) the memory block.</param>
    /// <param name="size">The memory block size, measured in number of characters.</param>
    /// <returns>An opened input stream.</returns>
    static concurrency::streams::basic_istream<char_type> open_istream(const char_type* data, size_t size)
    {
        return concurrency::streams::basic_istream<char_type>(buffer_type(data, size));
    }

    /// <summary>
    /// Create a rawptr-stream given a pointer to a writable memory block and the size of the block.
    /// </summary>
    /// <param name="data">The address (pointer to) the memory block.</param>
    /// <param name="size">The memory block size, measured in number of characters.</param>
    /// <returns>An opened input stream.</returns>
    static concurrency::streams::basic_istream<char_type> open_istream(char_type* data, size_t size)
    {
        return concurrency::streams::basic_istream<char_type>(buffer_type(data, size, std::ios::in));
    }

    /// <summary>
    /// Create a rawptr-stream given a pointer to a writable memory block and the size of the block.
    /// </summary>
    /// <param name="data">The address (pointer to) the memory block.</param>
    /// <param name="size">The memory block size, measured in number of characters.</param>
    /// <returns>An opened output stream.</returns>
    static concurrency::streams::basic_ostream<char_type> open_ostream(char_type* data, size_t size)
    {
        return concurrency::streams::basic_ostream<char_type>(buffer_type(data, size, std::ios::out));
    }
};

} // namespace streams
} // namespace Concurrency

#endif
