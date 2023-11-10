/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * This file defines a basic STL-container-based stream buffer. Reading from the buffer will not remove any data
 * from it and seeking is thus supported.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

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

template<typename _CollectionType>
class container_buffer;

namespace details
{
/// <summary>
/// The basic_container_buffer class serves as a memory-based steam buffer that supports writing or reading
/// sequences of characters.
/// The class itself should not be used in application code, it is used by the stream definitions farther down in the
/// header file.
/// </summary>
/// <remarks> When closed, neither writing nor reading is supported any longer. <c>basic_container_buffer</c> does not
/// support simultaneous use of the buffer for reading and writing.</remarks>
template<typename _CollectionType>
class basic_container_buffer : public streams::details::streambuf_state_manager<typename _CollectionType::value_type>
{
public:
    typedef typename _CollectionType::value_type _CharType;
    typedef typename basic_streambuf<_CharType>::traits traits;
    typedef typename basic_streambuf<_CharType>::int_type int_type;
    typedef typename basic_streambuf<_CharType>::pos_type pos_type;
    typedef typename basic_streambuf<_CharType>::off_type off_type;

    /// <summary>
    /// Returns the underlying data container
    /// </summary>
    _CollectionType& collection() { return m_data; }

    /// <summary>
    /// Destructor
    /// </summary>
    virtual ~basic_container_buffer()
    {
        // Invoke the synchronous versions since we need to
        // purge the request queue before deleting the buffer
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
    virtual utility::size64_t size() const { return utility::size64_t(m_data.size()); }

    /// <summary>
    /// Get the stream buffer size, if one has been set.
    /// </summary>
    /// <param name="direction">The direction of buffering (in or out)</param>
    /// <remarks>An implementation that does not support buffering will always return '0'.</remarks>
    virtual size_t buffer_size(std::ios_base::openmode = std::ios_base::in) const { return 0; }

    /// <summary>
    /// Sets the stream buffer implementation to buffer or not buffer.
    /// </summary>
    /// <param name="size">The size to use for internal buffering, 0 if no buffering should be done.</param>
    /// <param name="direction">The direction of buffering (in or out)</param>
    /// <remarks>An implementation that does not support buffering will silently ignore calls to this function and it
    /// will not have any effect on what is returned by subsequent calls to <see cref="::buffer_size method"
    /// />.</remarks>
    virtual void set_buffer_size(size_t, std::ios_base::openmode = std::ios_base::in) { return; }

    /// <summary>
    /// For any input stream, <c>in_avail</c> returns the number of characters that are immediately available
    /// to be consumed without blocking. May be used in conjunction with <cref="::sbumpc method"/> to read data without
    /// incurring the overhead of using tasks.
    /// </summary>
    virtual size_t in_avail() const
    {
        // See the comment in seek around the restriction that we do not allow read head to
        // seek beyond the current write_end.
        _ASSERTE(m_current_position <= m_data.size());

        msl::safeint3::SafeInt<size_t> readhead(m_current_position);
        msl::safeint3::SafeInt<size_t> writeend(m_data.size());
        return (size_t)(writeend - readhead);
    }

    virtual pplx::task<bool> _sync() { return pplx::task_from_result(true); }

    virtual pplx::task<int_type> _putc(_CharType ch)
    {
        int_type retVal = (this->write(&ch, 1) == 1) ? static_cast<int_type>(ch) : traits::eof();
        return pplx::task_from_result<int_type>(retVal);
    }

    virtual pplx::task<size_t> _putn(const _CharType* ptr, size_t count)
    {
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

        // Allocate space
        resize_for_write(m_current_position + count);

        // Let the caller copy the data
        return (_CharType*)&m_data[m_current_position];
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
        ptr = nullptr;
        count = 0;

        if (!this->can_read()) return false;

        count = in_avail();

        if (count > 0)
        {
            ptr = (_CharType*)&m_data[m_current_position];
            return true;
        }
        else
        {
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

        return static_cast<pos_type>(m_current_position);
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

        // In order to support relative seeking from the end position we need to fix an end position.
        // Technically, there is no end for the stream buffer as new writes would just expand the buffer.
        // For now, we assume that the current write_end is the end of the buffer. We use this artificial
        // end to restrict the read head from seeking beyond what is available.

        pos_type end(m_data.size());

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
                // Allocate space
                resize_for_write(pos);

                // Nothing to really copy

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
        pos_type end = static_cast<pos_type>(m_data.size());

        switch (way)
        {
            case std::ios_base::beg: return seekpos(beg + offset, mode);

            case std::ios_base::cur: return seekpos(cur + offset, mode);

            case std::ios_base::end: return seekpos(end + offset, mode);

            default: return static_cast<pos_type>(traits::eof());
        }
    }

private:
    template<typename _CollectionType1>
    friend class streams::container_buffer;

    /// <summary>
    /// Constructor
    /// </summary>
    basic_container_buffer(std::ios_base::openmode mode)
        : streambuf_state_manager<typename _CollectionType::value_type>(mode), m_current_position(0)
    {
        validate_mode(mode);
    }

    /// <summary>
    /// Constructor
    /// </summary>
    basic_container_buffer(_CollectionType data, std::ios_base::openmode mode)
        : streambuf_state_manager<typename _CollectionType::value_type>(mode)
        , m_data(std::move(data))
        , m_current_position((mode & std::ios_base::in) ? 0 : m_data.size())
    {
        validate_mode(mode);
    }

    static void validate_mode(std::ios_base::openmode mode)
    {
        // Disallow simultaneous use of the stream buffer for writing and reading.
        if ((mode & std::ios_base::in) && (mode & std::ios_base::out))
            throw std::invalid_argument("this combination of modes on container stream not supported");
    }

    /// <summary>
    /// Determine if the request can be satisfied.
    /// </summary>
    bool can_satisfy(size_t)
    {
        // We can always satisfy a read, at least partially, unless the
        // read position is at the very end of the buffer.
        return (in_avail() > 0);
    }

    /// <summary>
    /// Reads a byte from the stream and returns it as int_type.
    /// Note: This routine shall only be called if can_satisfy() returned true.
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
    /// Note: This routine shall only be called if can_satisfy() returned true.
    /// </summary>
    size_t read(_Out_writes_(count) _CharType* ptr, _In_ size_t count, bool advance = true)
    {
        if (!can_satisfy(count)) return 0;

        msl::safeint3::SafeInt<size_t> request_size(count);
        msl::safeint3::SafeInt<size_t> read_size = request_size.Min(in_avail());

        size_t newPos = m_current_position + read_size;

        auto readBegin = std::begin(m_data) + m_current_position;
        auto readEnd = std::begin(m_data) + newPos;

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

        auto newSize = m_current_position + count;

        // Allocate space
        resize_for_write(newSize);

        // Copy the data
        std::copy(ptr, ptr + count, std::begin(m_data) + m_current_position);

        // Update write head and satisfy pending reads if any
        update_current_position(newSize);

        return count;
    }

    /// <summary>
    /// Resize the underlying container to match the new write head
    /// </summary>
    void resize_for_write(size_t newPos)
    {
        // Resize the container if required
        if (newPos > m_data.size())
        {
            m_data.resize(newPos);
        }
    }

    /// <summary>
    /// Updates the write head to the new position
    /// </summary>
    void update_current_position(size_t newPos)
    {
        // The new write head
        m_current_position = newPos;
        _ASSERTE(m_current_position <= m_data.size());
    }

    // The actual data store
    _CollectionType m_data;

    // Read/write head
    size_t m_current_position;
};

} // namespace details

/// <summary>
/// The basic_container_buffer class serves as a memory-based steam buffer that supports writing or reading
/// sequences of characters. Note that it cannot be used as a consumer producer buffer.
/// </summary>
/// <typeparam name="_CollectionType">
/// The type of the container.
/// </typeparam>
/// <remarks>
/// This is a reference-counted version of <c>basic_container_buffer</c>.
/// </remarks>
template<typename _CollectionType>
class container_buffer : public streambuf<typename _CollectionType::value_type>
{
public:
    typedef typename _CollectionType::value_type char_type;

    /// <summary>
    /// Creates a container_buffer given a collection, copying its data into the buffer.
    /// </summary>
    /// <param name="data">The collection that is the starting point for the buffer</param>
    /// <param name="mode">The I/O mode that the buffer should use (in / out)</param>
    container_buffer(_CollectionType data, std::ios_base::openmode mode = std::ios_base::in)
        : streambuf<typename _CollectionType::value_type>(
              std::shared_ptr<details::basic_container_buffer<_CollectionType>>(
                  new streams::details::basic_container_buffer<_CollectionType>(std::move(data), mode)))
    {
    }

    /// <summary>
    /// Creates a container_buffer starting from an empty collection.
    /// </summary>
    /// <param name="mode">The I/O mode that the buffer should use (in / out)</param>
    container_buffer(std::ios_base::openmode mode = std::ios_base::out)
        : streambuf<typename _CollectionType::value_type>(
              std::shared_ptr<details::basic_container_buffer<_CollectionType>>(
                  new details::basic_container_buffer<_CollectionType>(mode)))
    {
    }

    _CollectionType& collection() const
    {
        auto listBuf = static_cast<details::basic_container_buffer<_CollectionType>*>(this->get_base().get());
        return listBuf->collection();
    }
};

/// <summary>
/// A static class to allow users to create input and out streams based off STL
/// collections. The sole purpose of this class to avoid users from having to know
/// anything about stream buffers.
/// </summary>
/// <typeparam name="_CollectionType">The type of the STL collection.</typeparam>
template<typename _CollectionType>
class container_stream
{
public:
    typedef typename _CollectionType::value_type char_type;
    typedef container_buffer<_CollectionType> buffer_type;

    /// <summary>
    /// Creates an input stream given an STL container.
    /// </summary>
    /// </param name="data">STL container to back the input stream.</param>
    /// <returns>An input stream.</returns>
    static concurrency::streams::basic_istream<char_type> open_istream(_CollectionType data)
    {
        return concurrency::streams::basic_istream<char_type>(buffer_type(std::move(data), std::ios_base::in));
    }

    /// <summary>
    /// Creates an output stream using an STL container as the storage.
    /// </summary>
    /// <returns>An output stream.</returns>
    static concurrency::streams::basic_ostream<char_type> open_ostream()
    {
        return concurrency::streams::basic_ostream<char_type>(buffer_type(std::ios_base::out));
    }
};

/// <summary>
/// The stringstream allows an input stream to be constructed from std::string or std::wstring
/// For output streams the underlying string container could be retrieved using <c>buf-&gt;collection().</c>
/// </summary>
typedef container_stream<std::basic_string<char>> stringstream;
typedef stringstream::buffer_type stringstreambuf;

typedef container_stream<utility::string_t> wstringstream;
typedef wstringstream::buffer_type wstringstreambuf;

/// <summary>
/// The <c>bytestream</c> is a static class that allows an input stream to be constructed from any STL container.
/// </summary>
class bytestream
{
public:
    /// <summary>
    /// Creates a single byte character input stream given an STL container.
    /// </summary>
    /// <typeparam name="_CollectionType">The type of the STL collection.</typeparam>
    /// <param name="data">STL container to back the input stream.</param>
    /// <returns>An single byte character input stream.</returns>
    template<typename _CollectionType>
    static concurrency::streams::istream open_istream(_CollectionType data)
    {
        return concurrency::streams::istream(
            streams::container_buffer<_CollectionType>(std::move(data), std::ios_base::in));
    }

    /// <summary>
    /// Creates a single byte character output stream using an STL container as storage.
    /// </summary>
    /// <typeparam name="_CollectionType">The type of the STL collection.</typeparam>
    /// <returns>A single byte character output stream.</returns>
    template<typename _CollectionType>
    static concurrency::streams::ostream open_ostream()
    {
        return concurrency::streams::ostream(streams::container_buffer<_CollectionType>());
    }
};

} // namespace streams
} // namespace Concurrency
