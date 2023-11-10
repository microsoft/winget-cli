/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Asynchronous I/O: streams API, used for formatted input and output, based on unformatted I/O using stream buffers
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#pragma once

#ifndef CASA_STREAMS_H
#define CASA_STREAMS_H

#include "cpprest/astreambuf.h"
#include <iosfwd>
#include <cstdio>

namespace Concurrency
{
namespace streams
{
template<typename CharType>
class basic_ostream;
template<typename CharType>
class basic_istream;

namespace details
{
template<typename CharType>
class basic_ostream_helper
{
public:
    basic_ostream_helper(streams::streambuf<CharType> buffer) : m_buffer(buffer) {}

    ~basic_ostream_helper() {}

private:
    template<typename CharType1>
    friend class streams::basic_ostream;

    concurrency::streams::streambuf<CharType> m_buffer;
};

template<typename CharType>
class basic_istream_helper
{
public:
    basic_istream_helper(streams::streambuf<CharType> buffer) : m_buffer(buffer) {}

    ~basic_istream_helper() {}

private:
    template<typename CharType1>
    friend class streams::basic_istream;

    concurrency::streams::streambuf<CharType> m_buffer;
};

template<typename CharType>
struct Value2StringFormatter
{
    template<typename T>
    static std::basic_string<CharType> format(const T& val)
    {
        std::basic_ostringstream<CharType> ss;
        ss << val;
        return ss.str();
    }
};

template<>
struct Value2StringFormatter<uint8_t>
{
    template<typename T>
    static std::basic_string<uint8_t> format(const T& val)
    {
        std::basic_ostringstream<char> ss;
        ss << val;
        return reinterpret_cast<const uint8_t*>(ss.str().c_str());
    }

    static std::basic_string<uint8_t> format(const utf16string& val)
    {
        return format(utility::conversions::utf16_to_utf8(val));
    }
};

static const char* _in_stream_msg = "stream not set up for input of data";
static const char* _in_streambuf_msg = "stream buffer not set up for input of data";
static const char* _out_stream_msg = "stream not set up for output of data";
static const char* _out_streambuf_msg = "stream buffer not set up for output of data";
} // namespace details

/// <summary>
/// Base interface for all asynchronous output streams.
/// </summary>
template<typename CharType>
class basic_ostream
{
public:
    typedef char_traits<CharType> traits;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;

    /// <summary>
    /// Default constructor
    /// </summary>
    basic_ostream() {}

    /// <summary>
    /// Copy constructor
    /// </summary>
    /// <param name="other">The source object</param>
    basic_ostream(const basic_ostream& other) : m_helper(other.m_helper) {}

    /// <summary>
    /// Assignment operator
    /// </summary>
    /// <param name="other">The source object</param>
    /// <returns>A reference to the stream object that contains the result of the assignment.</returns>
    basic_ostream& operator=(const basic_ostream& other)
    {
        m_helper = other.m_helper;
        return *this;
    }

    /// <summary>
    /// Constructor
    /// </summary>
    /// <param name="buffer">A stream buffer.</param>
    basic_ostream(streams::streambuf<CharType> buffer)
        : m_helper(std::make_shared<details::basic_ostream_helper<CharType>>(buffer))
    {
        _verify_and_throw(details::_out_streambuf_msg);
    }

    /// <summary>
    /// Close the stream, preventing further write operations.
    /// </summary>
    pplx::task<void> close() const
    {
        return is_valid() ? helper()->m_buffer.close(std::ios_base::out) : pplx::task_from_result();
    }

    /// <summary>
    /// Close the stream with exception, preventing further write operations.
    /// </summary>
    /// <param name="eptr">Pointer to the exception.</param>
    pplx::task<void> close(std::exception_ptr eptr) const
    {
        return is_valid() ? helper()->m_buffer.close(std::ios_base::out, eptr) : pplx::task_from_result();
    }

    /// <summary>
    /// Put a single character into the stream.
    /// </summary>
    /// <param name="ch">A character</param>
    pplx::task<int_type> write(CharType ch) const
    {
        pplx::task<int_type> result;
        if (!_verify_and_return_task(details::_out_stream_msg, result)) return result;
        return helper()->m_buffer.putc(ch);
    }

    /// <summary>
    /// Write a single value of "blittable" type T into the stream.
    /// </summary>
    /// <param name="value">A value of type T.</param>
    /// <remarks>
    /// This is not a replacement for a proper binary serialization solution, but it may
    /// form the foundation for one. Writing data bit-wise to a stream is a primitive
    /// operation of binary serialization.
    /// Currently, no attention is paid to byte order. All data is written in the platform's
    /// native byte order, which means little-endian on all platforms that have been tested.
    /// This function is only available for streams using a single-byte character size.
    /// </remarks>
    template<typename T>
    CASABLANCA_DEPRECATED(
        "Unsafe API that will be removed in future releases, use one of the other write overloads instead.")
    pplx::task<size_t> write(T value) const
    {
        static_assert(sizeof(CharType) == 1, "binary write is only supported for single-byte streams");
        static_assert(std::is_trivial<T>::value, "unsafe to use with non-trivial types");

        pplx::task<size_t> result;
        if (!_verify_and_return_task(details::_out_stream_msg, result)) return result;

        auto copy = std::make_shared<T>(std::move(value));
        return helper()
            ->m_buffer.putn_nocopy((CharType*)copy.get(), sizeof(T))
            .then([copy](pplx::task<size_t> op) -> size_t { return op.get(); });
    }

    /// <summary>
    /// Write a number of characters from a given stream buffer into the stream.
    /// </summary>
    /// <param name="source">A source stream buffer.</param>
    /// <param name="count">The number of characters to write.</param>
    pplx::task<size_t> write(streams::streambuf<CharType> source, size_t count) const
    {
        pplx::task<size_t> result;
        if (!_verify_and_return_task(details::_out_stream_msg, result)) return result;
        if (!source.can_read())
            return pplx::task_from_exception<size_t>(
                std::make_exception_ptr(std::runtime_error("source buffer not set up for input of data")));

        if (count == 0) return pplx::task_from_result((size_t)0);

        auto buffer = helper()->m_buffer;
        auto data = buffer.alloc(count);

        if (data != nullptr)
        {
            auto post_read = [buffer](pplx::task<size_t> op) -> pplx::task<size_t> {
                auto b = buffer;
                b.commit(op.get());
                return op;
            };
            return source.getn(data, count).then(post_read);
        }
        else
        {
            size_t available = 0;

            const bool acquired = source.acquire(data, available);
            if (available >= count)
            {
                auto post_write = [source, data](pplx::task<size_t> op) -> pplx::task<size_t> {
                    auto s = source;
                    s.release(data, op.get());
                    return op;
                };
                return buffer.putn_nocopy(data, count).then(post_write);
            }
            else
            {
                // Always have to release if acquire returned true.
                if (acquired)
                {
                    source.release(data, 0);
                }

                std::shared_ptr<CharType> buf(new CharType[count], [](CharType* buf) { delete[] buf; });

                auto post_write = [buf](pplx::task<size_t> op) -> pplx::task<size_t> { return op; };
                auto post_read = [buf, post_write, buffer](pplx::task<size_t> op) -> pplx::task<size_t> {
                    auto b = buffer;
                    return b.putn_nocopy(buf.get(), op.get()).then(post_write);
                };

                return source.getn(buf.get(), count).then(post_read);
            }
        }
    }

    /// <summary>
    /// Write the specified string to the output stream.
    /// </summary>
    /// <param name="str">Input string.</param>
    pplx::task<size_t> print(const std::basic_string<CharType>& str) const
    {
        pplx::task<size_t> result;
        if (!_verify_and_return_task(details::_out_stream_msg, result)) return result;

        if (str.empty())
        {
            return pplx::task_from_result<size_t>(0);
        }
        else
        {
            auto sharedStr = std::make_shared<std::basic_string<CharType>>(str);
            return helper()->m_buffer.putn_nocopy(sharedStr->c_str(), sharedStr->size()).then([sharedStr](size_t size) {
                return size;
            });
        }
    }

    /// <summary>
    /// Write a value of type <c>T</c> to the output stream.
    /// </summary>
    /// <typeparam name="T">
    /// The data type of the object to be written to the stream
    /// </typeparam>
    /// <param name="val">Input object.</param>
    template<typename T>
    pplx::task<size_t> print(const T& val) const
    {
        pplx::task<size_t> result;
        if (!_verify_and_return_task(details::_out_stream_msg, result)) return result;
        // TODO in the future this could be improved to have Value2StringFormatter avoid another unnecessary copy
        // by putting the string on the heap before calling the print string overload.
        return print(details::Value2StringFormatter<CharType>::format(val));
    }

    /// <summary>
    /// Write a value of type <c>T</c> to the output stream and append a newline character.
    /// </summary>
    /// <typeparam name="T">
    /// The data type of the object to be written to the stream
    /// </typeparam>
    /// <param name="val">Input object.</param>
    template<typename T>
    pplx::task<size_t> print_line(const T& val) const
    {
        pplx::task<size_t> result;
        if (!_verify_and_return_task(details::_out_stream_msg, result)) return result;
        auto str = details::Value2StringFormatter<CharType>::format(val);
        str.push_back(CharType('\n'));
        return print(str);
    }

    /// <summary>
    /// Flush any buffered output data.
    /// </summary>
    pplx::task<void> flush() const
    {
        pplx::task<void> result;
        if (!_verify_and_return_task(details::_out_stream_msg, result)) return result;
        return helper()->m_buffer.sync();
    }

    /// <summary>
    /// Seeks to the specified write position.
    /// </summary>
    /// <param name="pos">An offset relative to the beginning of the stream.</param>
    /// <returns>The new position in the stream.</returns>
    pos_type seek(pos_type pos) const
    {
        _verify_and_throw(details::_out_stream_msg);
        return helper()->m_buffer.seekpos(pos, std::ios_base::out);
    }

    /// <summary>
    /// Seeks to the specified write position.
    /// </summary>
    /// <param name="off">An offset relative to the beginning, current write position, or the end of the stream.</param>
    /// <param name="way">The starting point (beginning, current, end) for the seek.</param>
    /// <returns>The new position in the stream.</returns>
    pos_type seek(off_type off, std::ios_base::seekdir way) const
    {
        _verify_and_throw(details::_out_stream_msg);
        return helper()->m_buffer.seekoff(off, way, std::ios_base::out);
    }

    /// <summary>
    /// Get the current write position, i.e. the offset from the beginning of the stream.
    /// </summary>
    /// <returns>The current write position.</returns>
    pos_type tell() const
    {
        _verify_and_throw(details::_out_stream_msg);
        return helper()->m_buffer.getpos(std::ios_base::out);
    }

    /// <summary>
    /// <c>can_seek<c/> is used to determine whether the stream supports seeking.
    /// </summary>
    /// <returns><c>true</c> if the stream supports seeking, <c>false</c> otherwise.</returns>
    bool can_seek() const { return is_valid() && m_helper->m_buffer.can_seek(); }

    /// <summary>
    /// Test whether the stream has been initialized with a valid stream buffer.
    /// </summary>
    /// <returns><c>true</c> if the stream has been initialized with a valid stream buffer, <c>false</c>
    /// otherwise.</returns>
    bool is_valid() const { return (m_helper != nullptr) && ((bool)m_helper->m_buffer); }

    /// <summary>
    /// Test whether the stream has been initialized or not.
    /// </summary>
    operator bool() const { return is_valid(); }

    /// <summary>
    /// Test whether the stream is open for writing.
    /// </summary>
    /// <returns><c>true</c> if the stream is open for writing, <c>false</c> otherwise.</returns>
    bool is_open() const { return is_valid() && m_helper->m_buffer.can_write(); }

    /// <summary>
    /// Get the underlying stream buffer.
    /// </summary>
    /// <returns>The underlying stream buffer.</returns>
    concurrency::streams::streambuf<CharType> streambuf() const { return helper()->m_buffer; }

protected:
    void set_helper(std::shared_ptr<details::basic_ostream_helper<CharType>> helper) { m_helper = helper; }

private:
    template<typename T>
    bool _verify_and_return_task(const char* msg, pplx::task<T>& tsk) const
    {
        auto buffer = helper()->m_buffer;
        if (!(buffer.exception() == nullptr))
        {
            tsk = pplx::task_from_exception<T>(buffer.exception());
            return false;
        }
        if (!buffer.can_write())
        {
            tsk = pplx::task_from_exception<T>(std::make_exception_ptr(std::runtime_error(msg)));
            return false;
        }
        return true;
    }

    void _verify_and_throw(const char* msg) const
    {
        auto buffer = helper()->m_buffer;
        if (!(buffer.exception() == nullptr)) std::rethrow_exception(buffer.exception());
        if (!buffer.can_write()) throw std::runtime_error(msg);
    }

    std::shared_ptr<details::basic_ostream_helper<CharType>> helper() const
    {
        if (!m_helper) throw std::logic_error("uninitialized stream object");
        return m_helper;
    }

    std::shared_ptr<details::basic_ostream_helper<CharType>> m_helper;
};

template<typename int_type>
struct _type_parser_integral_traits
{
    typedef std::false_type _is_integral;
    typedef std::false_type _is_unsigned;
};

#ifdef _WIN32
#define _INT_TRAIT(_t, _low, _high)                                                                                    \
    template<>                                                                                                         \
    struct _type_parser_integral_traits<_t>                                                                            \
    {                                                                                                                  \
        typedef std::true_type _is_integral;                                                                           \
        typedef std::false_type _is_unsigned;                                                                          \
        static const int64_t _min = _low;                                                                              \
        static const int64_t _max = _high;                                                                             \
    };
#define _UINT_TRAIT(_t, _low, _high)                                                                                   \
    template<>                                                                                                         \
    struct _type_parser_integral_traits<_t>                                                                            \
    {                                                                                                                  \
        typedef std::true_type _is_integral;                                                                           \
        typedef std::true_type _is_unsigned;                                                                           \
        static const uint64_t _max = _high;                                                                            \
    };

_INT_TRAIT(char, INT8_MIN, INT8_MAX)
_INT_TRAIT(signed char, INT8_MIN, INT8_MAX)
_INT_TRAIT(short, INT16_MIN, INT16_MAX)
#if defined(_NATIVE_WCHAR_T_DEFINED)
_INT_TRAIT(wchar_t, WCHAR_MIN, WCHAR_MAX)
#endif
_INT_TRAIT(int, INT32_MIN, INT32_MAX)
_INT_TRAIT(long, LONG_MIN, LONG_MAX)
_INT_TRAIT(long long, LLONG_MIN, LLONG_MAX)
_UINT_TRAIT(unsigned char, UINT8_MIN, UINT8_MAX)
_UINT_TRAIT(unsigned short, UINT16_MIN, UINT16_MAX)
_UINT_TRAIT(unsigned int, UINT32_MIN, UINT32_MAX)
_UINT_TRAIT(unsigned long, ULONG_MIN, ULONG_MAX)
_UINT_TRAIT(unsigned long long, ULLONG_MIN, ULLONG_MAX)
#else
#define _INT_TRAIT(_t)                                                                                                 \
    template<>                                                                                                         \
    struct _type_parser_integral_traits<_t>                                                                            \
    {                                                                                                                  \
        typedef std::true_type _is_integral;                                                                           \
        typedef std::false_type _is_unsigned;                                                                          \
        static const int64_t _min = (std::numeric_limits<_t>::min)();                                                  \
        static const int64_t _max = (std::numeric_limits<_t>::max)();                                                  \
    };
#define _UINT_TRAIT(_t)                                                                                                \
    template<>                                                                                                         \
    struct _type_parser_integral_traits<_t>                                                                            \
    {                                                                                                                  \
        typedef std::true_type _is_integral;                                                                           \
        typedef std::true_type _is_unsigned;                                                                           \
        static const uint64_t _max = (std::numeric_limits<_t>::max)();                                                 \
    };

_INT_TRAIT(char)
_INT_TRAIT(signed char)
_INT_TRAIT(short)
_INT_TRAIT(utf16char)
_INT_TRAIT(int)
_INT_TRAIT(long)
_INT_TRAIT(long long)
_UINT_TRAIT(unsigned char)
_UINT_TRAIT(unsigned short)
_UINT_TRAIT(unsigned int)
_UINT_TRAIT(unsigned long)
_UINT_TRAIT(unsigned long long)
#endif

template<typename CharType>
class _type_parser_base
{
public:
    typedef char_traits<CharType> traits;
    typedef typename traits::int_type int_type;

    _type_parser_base() {}

protected:
    // Aid in parsing input: skipping whitespace characters.
    static pplx::task<void> _skip_whitespace(streams::streambuf<CharType> buffer);

    // Aid in parsing input: peek at a character at a time, call type-specific code to examine, extract value when done.
    // <remark>AcceptFunctor should model std::function<bool(std::shared_ptr<X>, int_type)></remark>
    // <remark>ExtractFunctor should model std::function<pplx::task<ReturnType>(std::shared_ptr<X>)></remark>
    template<typename StateType, typename ReturnType, typename AcceptFunctor, typename ExtractFunctor>
    static pplx::task<ReturnType> _parse_input(streams::streambuf<CharType> buffer,
                                               AcceptFunctor accept_character,
                                               ExtractFunctor extract);
};

/// <summary>
/// Class used to handle asynchronous parsing for basic_istream::extract. To support new
/// types create a new template specialization and implement the parse function.
/// </summary>
template<typename CharType, typename T>
class type_parser
{
public:
    static pplx::task<T> parse(streams::streambuf<CharType> buffer)
    {
        typedef typename _type_parser_integral_traits<T>::_is_integral ii;
        typedef typename _type_parser_integral_traits<T>::_is_unsigned ui;

        static_assert(ii::value || !ui::value, "type is not supported for extraction from a stream");

        return _parse(buffer, ii {}, ui {});
    }

private:
    static pplx::task<T> _parse(streams::streambuf<CharType> buffer, std::false_type, std::false_type)
    {
        _parse_floating_point(buffer);
    }

    static pplx::task<T> _parse(streams::streambuf<CharType> buffer, std::true_type, std::false_type)
    {
        return type_parser<CharType, int64_t>::parse(buffer).then([](pplx::task<int64_t> op) -> T {
            int64_t val = op.get();
            if (val <= _type_parser_integral_traits<T>::_max && val >= _type_parser_integral_traits<T>::_min)
                return (T)val;
            else
                throw std::range_error("input out of range for target type");
        });
    }

    static pplx::task<T> _parse(streams::streambuf<CharType> buffer, std::true_type, std::true_type)
    {
        return type_parser<CharType, uint64_t>::parse(buffer).then([](pplx::task<uint64_t> op) -> T {
            uint64_t val = op.get();
            if (val <= _type_parser_integral_traits<T>::_max)
                return (T)val;
            else
                throw std::range_error("input out of range for target type");
        });
    }
};

/// <summary>
/// Base interface for all asynchronous input streams.
/// </summary>
template<typename CharType>
class basic_istream
{
public:
    typedef char_traits<CharType> traits;
    typedef typename traits::int_type int_type;
    typedef typename traits::pos_type pos_type;
    typedef typename traits::off_type off_type;

    /// <summary>
    /// Default constructor
    /// </summary>
    basic_istream() {}

    /// <summary>
    /// Constructor
    /// </summary>
    /// <typeparam name="CharType">
    /// The data type of the basic element of the stream.
    /// </typeparam>
    /// <param name="buffer">A stream buffer.</param>
    template<class AlterCharType>
    basic_istream(streams::streambuf<AlterCharType> buffer)
        : m_helper(std::make_shared<details::basic_istream_helper<CharType>>(std::move(buffer)))
    {
        _verify_and_throw(details::_in_streambuf_msg);
    }

    /// <summary>
    /// Copy constructor
    /// </summary>
    /// <param name="other">The source object</param>
    basic_istream(const basic_istream& other) : m_helper(other.m_helper) {}

    /// <summary>
    /// Assignment operator
    /// </summary>
    /// <param name="other">The source object</param>
    /// <returns>A reference to the stream object that contains the result of the assignment.</returns>
    basic_istream& operator=(const basic_istream& other)
    {
        m_helper = other.m_helper;
        return *this;
    }

    /// <summary>
    /// Close the stream, preventing further read operations.
    /// </summary>
    pplx::task<void> close() const
    {
        return is_valid() ? helper()->m_buffer.close(std::ios_base::in) : pplx::task_from_result();
    }

    /// <summary>
    /// Close the stream with exception, preventing further read operations.
    /// </summary>
    /// <param name="eptr">Pointer to the exception.</param>
    pplx::task<void> close(std::exception_ptr eptr) const
    {
        return is_valid() ? m_helper->m_buffer.close(std::ios_base::in, eptr) : pplx::task_from_result();
    }

    /// <summary>
    /// Tests whether last read cause the stream reach EOF.
    /// </summary>
    /// <returns>True if the read head has reached the end of the stream, false otherwise.</returns>
    bool is_eof() const { return is_valid() ? m_helper->m_buffer.is_eof() : false; }

    /// <summary>
    /// Get the next character and return it as an int_type. Advance the read position.
    /// </summary>
    /// <returns>A <c>task</c> that holds the next character as an <c>int_type</c> on successful completion.</returns>
    pplx::task<int_type> read() const
    {
        pplx::task<int_type> result;
        if (!_verify_and_return_task(details::_in_stream_msg, result)) return result;
        return helper()->m_buffer.bumpc();
    }

    /// <summary>
    /// Read a single value of "blittable" type T from the stream.
    /// </summary>
    /// <returns>A value of type T.</returns>
    /// <remarks>
    /// This is not a replacement for a proper binary serialization solution, but it may
    /// form the foundation for one. Reading data bit-wise to a stream is a primitive
    /// operation of binary serialization.
    /// Currently, no attention is paid to byte order. All data is read in the platform's
    /// native byte order, which means little-endian on all platforms that have been tested.
    /// This function is only available for streams using a single-byte character size.
    /// </remarks>
    template<typename T>
    CASABLANCA_DEPRECATED(
        "Unsafe API that will be removed in future releases, use one of the other read overloads instead.")
    pplx::task<T> read() const
    {
        static_assert(sizeof(CharType) == 1, "binary read is only supported for single-byte streams");
        static_assert(std::is_trivial<T>::value, "unsafe to use with non-trivial types");

        pplx::task<T> result;
        if (!_verify_and_return_task(details::_in_stream_msg, result)) return result;

        auto copy = std::make_shared<T>();
        return helper()->m_buffer.getn((CharType*)copy.get(), sizeof(T)).then([copy](pplx::task<size_t>) -> T {
            return std::move(*copy);
        });
    }

    /// <summary>
    /// Reads up to <c>count</c> characters and place into the provided buffer.
    /// </summary>
    /// <param name="target">An async stream buffer supporting write operations.</param>
    /// <param name="count">The maximum number of characters to read</param>
    /// <returns>A <c>task</c> that holds the number of characters read. This number is 0 if the end of the stream is
    /// reached.</returns>
    pplx::task<size_t> read(streams::streambuf<CharType> target, size_t count) const
    {
        pplx::task<size_t> result;
        if (!_verify_and_return_task(details::_in_stream_msg, result)) return result;
        if (!target.can_write())
            return pplx::task_from_exception<size_t>(
                std::make_exception_ptr(std::runtime_error("target not set up for output of data")));

        // Capture 'buffer' rather than 'helper' here due to VC++ 2010 limitations.
        auto buffer = helper()->m_buffer;

        auto data = target.alloc(count);

        if (data != nullptr)
        {
            auto post_read = [target](pplx::task<size_t> op) -> pplx::task<size_t> {
                auto t = target;
                t.commit(op.get());
                return op;
            };
            return buffer.getn(data, count).then(post_read);
        }
        else
        {
            size_t available = 0;

            const bool acquired = buffer.acquire(data, available);
            if (available >= count)
            {
                auto post_write = [buffer, data](pplx::task<size_t> op) -> pplx::task<size_t> {
                    auto b = buffer;
                    b.release(data, op.get());
                    return op;
                };
                return target.putn_nocopy(data, count).then(post_write);
            }
            else
            {
                // Always have to release if acquire returned true.
                if (acquired)
                {
                    buffer.release(data, 0);
                }

                std::shared_ptr<CharType> buf(new CharType[count], [](CharType* buf) { delete[] buf; });

                auto post_write = [buf](pplx::task<size_t> op) -> pplx::task<size_t> { return op; };
                auto post_read = [buf, target, post_write](pplx::task<size_t> op) -> pplx::task<size_t> {
                    auto trg = target;
                    return trg.putn_nocopy(buf.get(), op.get()).then(post_write);
                };

                return helper()->m_buffer.getn(buf.get(), count).then(post_read);
            }
        }
    }

    /// <summary>
    /// Get the next character and return it as an int_type. Do not advance the read position.
    /// </summary>
    /// <returns>A <c>task</c> that holds the character, widened to an integer. This character is EOF when the peek
    /// operation fails.</returns>
    pplx::task<int_type> peek() const
    {
        pplx::task<int_type> result;
        if (!_verify_and_return_task(details::_in_stream_msg, result)) return result;
        return helper()->m_buffer.getc();
    }

    /// <summary>
    /// Read characters until a delimiter or EOF is found, and place them into the target.
    /// Proceed past the delimiter, but don't include it in the target buffer.
    /// </summary>
    /// <param name="target">An async stream buffer supporting write operations.</param>
    /// <param name="delim">The delimiting character to stop the read at.</param>
    /// <returns>A <c>task</c> that holds the number of characters read.</returns>
    pplx::task<size_t> read_to_delim(streams::streambuf<CharType> target, int_type delim) const
    {
        pplx::task<size_t> result;
        if (!_verify_and_return_task(details::_in_stream_msg, result)) return result;
        if (!target.can_write())
            return pplx::task_from_exception<size_t>(
                std::make_exception_ptr(std::runtime_error("target not set up for output of data")));

        // Capture 'buffer' rather than 'helper' here due to VC++ 2010 limitations.
        auto buffer = helper()->m_buffer;

        int_type req_async = traits::requires_async();

        std::shared_ptr<_read_helper> _locals = std::make_shared<_read_helper>();

        auto flush = [=]() mutable {
            return target.putn_nocopy(_locals->outbuf, _locals->write_pos).then([=](size_t wrote) mutable {
                _locals->total += wrote;
                _locals->write_pos = 0;
                return target.sync();
            });
        };

        auto update = [=](int_type ch) mutable {
            if (ch == traits::eof()) return false;
            if (ch == delim) return false;

            _locals->outbuf[_locals->write_pos] = static_cast<CharType>(ch);
            _locals->write_pos += 1;

            if (_locals->is_full())
            {
                // Flushing synchronously because performance is terrible if we
                // schedule an empty task. This isn't on a user's thread.
                flush().get();
            }

            return true;
        };

        auto loop = pplx::details::_do_while([=]() mutable -> pplx::task<bool> {
            while (buffer.in_avail() > 0)
            {
                int_type ch = buffer.sbumpc();

                if (ch == req_async)
                {
                    break;
                }

                if (!update(ch))
                {
                    return pplx::task_from_result(false);
                }
            }
            return buffer.bumpc().then(update);
        });

        return loop.then([=](bool) mutable { return flush().then([=] { return _locals->total; }); });
    }

    /// <summary>
    /// Read until reaching a newline character. The newline is not included in the target.
    /// </summary>
    /// <param name="target">An asynchronous stream buffer supporting write operations.</param>
    /// <returns>A <c>task</c> that holds the number of characters read. This number is 0 if the end of the stream is
    /// reached.</returns>
    pplx::task<size_t> read_line(streams::streambuf<CharType> target) const
    {
        pplx::task<size_t> result;
        if (!_verify_and_return_task(details::_in_stream_msg, result)) return result;
        if (!target.can_write())
            return pplx::task_from_exception<size_t>(
                std::make_exception_ptr(std::runtime_error("target not set up for receiving data")));

        // Capture 'buffer' rather than 'helper' here due to VC++ 2010 limitations.
        concurrency::streams::streambuf<CharType> buffer = helper()->m_buffer;

        int_type req_async = traits::requires_async();

        std::shared_ptr<_read_helper> _locals = std::make_shared<_read_helper>();

        auto flush = [=]() mutable {
            return target.putn_nocopy(_locals->outbuf, _locals->write_pos).then([=](size_t wrote) mutable {
                _locals->total += wrote;
                _locals->write_pos = 0;
                return target.sync();
            });
        };

        auto update = [=](int_type ch) mutable {
            if (ch == traits::eof()) return false;
            if (ch == '\n') return false;
            if (ch == '\r')
            {
                _locals->saw_CR = true;
                return true;
            }

            _locals->outbuf[_locals->write_pos] = static_cast<CharType>(ch);
            _locals->write_pos += 1;

            if (_locals->is_full())
            {
                // Flushing synchronously because performance is terrible if we
                // schedule an empty task. This isn't on a user's thread.
                flush().wait();
            }

            return true;
        };

        auto update_after_cr = [=](int_type ch) mutable -> pplx::task<bool> {
            if (ch == traits::eof()) return pplx::task_from_result(false);
            if (ch == '\n')
            {
                return buffer.bumpc().then([](int_type) { return false; });
            }
            return pplx::task_from_result(false);
        };

        auto loop = pplx::details::_do_while([=]() mutable -> pplx::task<bool> {
            while (buffer.in_avail() > 0)
            {
                int_type ch;

                if (_locals->saw_CR)
                {
                    ch = buffer.sgetc();
                    if (ch == '\n') buffer.sbumpc();
                    return pplx::task_from_result(false);
                }

                ch = buffer.sbumpc();

                if (ch == req_async) break;

                if (!update(ch))
                {
                    return pplx::task_from_result(false);
                }
            }

            if (_locals->saw_CR)
            {
                return buffer.getc().then(update_after_cr);
            }
            return buffer.bumpc().then(update);
        });

        return loop.then([=](bool) mutable { return flush().then([=] { return _locals->total; }); });
    }

    /// <summary>
    /// Read until reaching the end of the stream.
    /// </summary>
    /// <param name="target">An asynchronous stream buffer supporting write operations.</param>
    /// <returns>The number of characters read.</returns>
    pplx::task<size_t> read_to_end(streams::streambuf<CharType> target) const
    {
        pplx::task<size_t> result;
        if (!_verify_and_return_task("stream not set up for output of data", result)) return result;
        if (!target.can_write())
            return pplx::task_from_exception<size_t>(
                std::make_exception_ptr(std::runtime_error("source buffer not set up for input of data")));

        auto l_buffer = helper()->m_buffer;
        auto l_buf_size = this->buf_size;
        std::shared_ptr<_read_helper> l_locals = std::make_shared<_read_helper>();

        auto copy_to_target = [l_locals, target, l_buffer, l_buf_size]() mutable -> pplx::task<bool> {
            // We need to capture these, because the object itself may go away
            // before we're done processing the data.
            // auto locs = _locals;
            // auto trg = target;

            return l_buffer.getn(l_locals->outbuf, l_buf_size).then([=](size_t rd) mutable -> pplx::task<bool> {
                if (rd == 0) return pplx::task_from_result(false);

                // Must be nested to capture rd
                return target.putn_nocopy(l_locals->outbuf, rd)
                    .then([target, l_locals, rd](size_t wr) mutable -> pplx::task<bool> {
                        l_locals->total += wr;

                        if (rd != wr)
                            // Number of bytes written is less than number of bytes received.
                            throw std::runtime_error("failed to write all bytes");

                        return target.sync().then([]() { return true; });
                    });
            });
        };

        auto loop = pplx::details::_do_while(copy_to_target);

        return loop.then([=](bool) mutable -> size_t { return l_locals->total; });
    }

    /// <summary>
    /// Seeks to the specified write position.
    /// </summary>
    /// <param name="pos">An offset relative to the beginning of the stream.</param>
    /// <returns>The new position in the stream.</returns>
    pos_type seek(pos_type pos) const
    {
        _verify_and_throw(details::_in_stream_msg);
        return helper()->m_buffer.seekpos(pos, std::ios_base::in);
    }

    /// <summary>
    /// Seeks to the specified write position.
    /// </summary>
    /// <param name="off">An offset relative to the beginning, current write position, or the end of the stream.</param>
    /// <param name="way">The starting point (beginning, current, end) for the seek.</param>
    /// <returns>The new position in the stream.</returns>
    pos_type seek(off_type off, std::ios_base::seekdir way) const
    {
        _verify_and_throw(details::_in_stream_msg);
        return helper()->m_buffer.seekoff(off, way, std::ios_base::in);
    }

    /// <summary>
    /// Get the current write position, i.e. the offset from the beginning of the stream.
    /// </summary>
    /// <returns>The current write position.</returns>
    pos_type tell() const
    {
        _verify_and_throw(details::_in_stream_msg);
        return helper()->m_buffer.getpos(std::ios_base::in);
    }

    /// <summary>
    /// <c>can_seek<c/> is used to determine whether the stream supports seeking.
    /// </summary>
    /// <returns><c>true</c> if the stream supports seeking, <c>false</c> otherwise.</returns>
    bool can_seek() const { return is_valid() && m_helper->m_buffer.can_seek(); }

    /// <summary>
    /// Test whether the stream has been initialized with a valid stream buffer.
    /// </summary>
    bool is_valid() const { return (m_helper != nullptr) && ((bool)m_helper->m_buffer); }

    /// <summary>
    /// Test whether the stream has been initialized or not.
    /// </summary>
    operator bool() const { return is_valid(); }

    /// <summary>
    /// Test whether the stream is open for writing.
    /// </summary>
    /// <returns><c>true</c> if the stream is open for writing, <c>false</c> otherwise.</returns>
    bool is_open() const { return is_valid() && m_helper->m_buffer.can_read(); }

    /// <summary>
    /// Get the underlying stream buffer.
    /// </summary>
    concurrency::streams::streambuf<CharType> streambuf() const { return helper()->m_buffer; }

    /// <summary>
    /// Read a value of type <c>T</c> from the stream.
    /// </summary>
    /// <remarks>
    /// Supports the C++ primitive types. Can be expanded to additional types
    /// by adding template specializations for <c>type_parser</c>.
    /// </remarks>
    /// <typeparam name="T">
    /// The data type of the element to be read from the stream.
    /// </typeparam>
    /// <returns>A <c>task</c> that holds the element read from the stream.</returns>
    template<typename T>
    pplx::task<T> extract() const
    {
        pplx::task<T> result;
        if (!_verify_and_return_task(details::_in_stream_msg, result)) return result;
        return type_parser<CharType, T>::parse(helper()->m_buffer);
    }

private:
    template<typename T>
    bool _verify_and_return_task(const char* msg, pplx::task<T>& tsk) const
    {
        auto buffer = helper()->m_buffer;
        if (!(buffer.exception() == nullptr))
        {
            tsk = pplx::task_from_exception<T>(buffer.exception());
            return false;
        }
        if (!buffer.can_read())
        {
            tsk = pplx::task_from_exception<T>(std::make_exception_ptr(std::runtime_error(msg)));
            return false;
        }
        return true;
    }

    void _verify_and_throw(const char* msg) const
    {
        auto buffer = helper()->m_buffer;
        if (!(buffer.exception() == nullptr)) std::rethrow_exception(buffer.exception());
        if (!buffer.can_read()) throw std::runtime_error(msg);
    }

    std::shared_ptr<details::basic_istream_helper<CharType>> helper() const
    {
        if (!m_helper) throw std::logic_error("uninitialized stream object");
        return m_helper;
    }

    static const size_t buf_size = 16 * 1024;

    struct _read_helper
    {
        size_t total;
        CharType outbuf[buf_size];
        size_t write_pos;
        bool saw_CR;

        bool is_full() const { return write_pos == buf_size; }

        _read_helper() : total(0), write_pos(0), saw_CR(false) {}
    };

    std::shared_ptr<details::basic_istream_helper<CharType>> m_helper;
};

typedef basic_ostream<uint8_t> ostream;
typedef basic_istream<uint8_t> istream;

typedef basic_ostream<utf16char> wostream;
typedef basic_istream<utf16char> wistream;

template<typename CharType>
pplx::task<void> _type_parser_base<CharType>::_skip_whitespace(streams::streambuf<CharType> buffer)
{
    int_type req_async = traits::requires_async();

    auto update = [=](int_type ch) mutable {
        if (isspace(ch))
        {
            if (buffer.sbumpc() == req_async)
            {
                // Synchronously because performance is terrible if we
                // schedule an empty task. This isn't on a user's thread.
                buffer.nextc().wait();
            }
            return true;
        }

        return false;
    };

    auto loop = pplx::details::_do_while([=]() mutable -> pplx::task<bool> {
        while (buffer.in_avail() > 0)
        {
            int_type ch = buffer.sgetc();

            if (ch == req_async) break;

            if (!update(ch))
            {
                return pplx::task_from_result(false);
            }
        }
        return buffer.getc().then(update);
    });

    return loop.then([=](pplx::task<bool> op) { op.wait(); });
}

template<typename CharType>
template<typename StateType, typename ReturnType, typename AcceptFunctor, typename ExtractFunctor>
pplx::task<ReturnType> _type_parser_base<CharType>::_parse_input(concurrency::streams::streambuf<CharType> buffer,
                                                                 AcceptFunctor accept_character,
                                                                 ExtractFunctor extract)
{
    std::shared_ptr<StateType> state = std::make_shared<StateType>();

    auto update = [=](pplx::task<int_type> op) -> pplx::task<bool> {
        int_type ch = op.get();
        if (ch == traits::eof()) return pplx::task_from_result(false);
        bool accepted = accept_character(state, ch);
        if (!accepted) return pplx::task_from_result(false);
        // We peeked earlier, so now we must advance the position.
        concurrency::streams::streambuf<CharType> buf = buffer;
        return buf.bumpc().then([](int_type) { return true; });
    };

    auto peek_char = [=]() -> pplx::task<bool> {
        concurrency::streams::streambuf<CharType> buf = buffer;

        // If task results are immediately available, there's little need to use ".then(),"
        // so optimize for prompt values.

        auto get_op = buf.getc();
        while (get_op.is_done())
        {
            auto condition = update(get_op);
            if (!condition.is_done() || !condition.get()) return condition;

            get_op = buf.getc();
        }

        return get_op.then(update);
    };

    auto finish = [=](pplx::task<bool> op) -> pplx::task<ReturnType> {
        op.wait();
        pplx::task<ReturnType> result = extract(state);
        return result;
    };

    return _skip_whitespace(buffer).then([=](pplx::task<void> op) -> pplx::task<ReturnType> {
        op.wait();
        return pplx::details::_do_while(peek_char).then(finish);
    });
}

template<typename CharType>
class type_parser<CharType, std::basic_string<CharType>> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<std::string> parse(streams::streambuf<CharType> buffer)
    {
        return base::template _parse_input<std::basic_string<CharType>, std::string>(
            buffer, _accept_char, _extract_result);
    }

private:
    static bool _accept_char(std::shared_ptr<std::basic_string<CharType>> state, int_type ch)
    {
        if (ch == traits::eof() || isspace(ch)) return false;
        state->push_back(CharType(ch));
        return true;
    }
    static pplx::task<std::basic_string<CharType>> _extract_result(std::shared_ptr<std::basic_string<CharType>> state)
    {
        return pplx::task_from_result(*state);
    }
};

template<typename CharType>
class type_parser<CharType, int64_t> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<int64_t> parse(streams::streambuf<CharType> buffer)
    {
        return base::template _parse_input<_int64_state, int64_t>(buffer, _accept_char, _extract_result);
    }

private:
    struct _int64_state
    {
        _int64_state() : result(0), correct(false), minus(0) {}

        int64_t result;
        bool correct;
        char minus; // 0 -- no sign, 1 -- plus, 2 -- minus
    };

    static bool _accept_char(std::shared_ptr<_int64_state> state, int_type ch)
    {
        if (ch == traits::eof()) return false;
        if (state->minus == 0)
        {
            // OK to find a sign.
            if (!::isdigit(ch) && ch != int_type('+') && ch != int_type('-')) return false;
        }
        else
        {
            if (!::isdigit(ch)) return false;
        }

        // At least one digit was found.
        state->correct = true;

        if (ch == int_type('+'))
        {
            state->minus = 1;
        }
        else if (ch == int_type('-'))
        {
            state->minus = 2;
        }
        else
        {
            if (state->minus == 0) state->minus = 1;

            // Shift the existing value by 10, then add the new value.
            bool positive = state->result >= 0;

            state->result *= 10;
            state->result += int64_t(ch - int_type('0'));

            if ((state->result >= 0) != positive)
            {
                state->correct = false;
                return false;
            }
        }
        return true;
    }

    static pplx::task<int64_t> _extract_result(std::shared_ptr<_int64_state> state)
    {
        if (!state->correct) throw std::range_error("integer value is too large to fit in 64 bits");

        int64_t result = (state->minus == 2) ? -state->result : state->result;
        return pplx::task_from_result<int64_t>(result);
    }
};

template<typename FloatingPoint>
struct _double_state
{
    _double_state()
        : result(0)
        , minus(0)
        , after_comma(0)
        , exponent(false)
        , exponent_number(0)
        , exponent_minus(0)
        , complete(false)
        , p_exception_string()
    {
    }

    FloatingPoint result;
    char minus; // 0 -- no sign, 1 -- plus, 2 -- minus
    int after_comma;
    bool exponent;
    int exponent_number;
    char exponent_minus; // 0 -- no sign, 1 -- plus, 2 -- minus
    bool complete;
    std::string p_exception_string;
};

template<typename FloatingPoint, typename int_type>
static std::string create_exception_message(int_type ch, bool exponent)
{
    std::string result;
    if (exponent)
    {
        result.assign("Invalid character 'X' in exponent");
    }
    else
    {
        result.assign("Invalid character 'X'");
    }

    result[19] = static_cast<char>(ch);
    return result;
}

template<typename FloatingPoint, typename int_type>
static bool _accept_char(std::shared_ptr<_double_state<FloatingPoint>> state, int_type ch)
{
    if (state->minus == 0)
    {
        if (!::isdigit(ch) && ch != int_type('.') && ch != int_type('+') && ch != int_type('-'))
        {
            if (!state->complete)
                state->p_exception_string = create_exception_message<FloatingPoint, int_type>(ch, false);
            return false;
        }
    }
    else
    {
        if (!state->exponent && !::isdigit(ch) && ch != int_type('.') && ch != int_type('E') && ch != int_type('e'))
        {
            if (!state->complete)
                state->p_exception_string = create_exception_message<FloatingPoint, int_type>(ch, false);
            return false;
        }

        if (state->exponent && !::isdigit(ch) && ch != int_type('+') && ch != int_type('-'))
        {
            if (!state->complete)
                state->p_exception_string = create_exception_message<FloatingPoint, int_type>(ch, true);
            return false;
        }
    }

    switch (ch)
    {
        case int_type('+'):
            state->complete = false;
            if (state->exponent)
            {
                if (state->exponent_minus != 0)
                {
                    state->p_exception_string = "The exponent sign already set";
                    return false;
                }
                state->exponent_minus = 1;
            }
            else
            {
                state->minus = 1;
            }
            break;
        case int_type('-'):
            state->complete = false;
            if (state->exponent)
            {
                if (state->exponent_minus != 0)
                {
                    state->p_exception_string = "The exponent sign already set";
                    return false;
                }

                state->exponent_minus = 2;
            }
            else
            {
                state->minus = 2;
            }
            break;
        case int_type('.'):
            state->complete = false;
            if (state->after_comma > 0) return false;

            state->after_comma = 1;
            break;
        case int_type('E'):
        case int_type('e'):
            state->complete = false;
            if (state->exponent) return false;
            state->exponent_number = 0;
            state->exponent = true;
            break;
        default:
            state->complete = true;
            if (!state->exponent)
            {
                if (state->minus == 0) state->minus = 1;

                state->result *= 10;
                state->result += int64_t(ch - int_type('0'));

                if (state->after_comma > 0) state->after_comma++;
            }
            else
            {
                if (state->exponent_minus == 0) state->exponent_minus = 1;
                state->exponent_number *= 10;
                state->exponent_number += int64_t(ch - int_type('0'));
            }
    }
    return true;
}

template<typename FloatingPoint>
static pplx::task<FloatingPoint> _extract_result(std::shared_ptr<_double_state<FloatingPoint>> state)
{
    if (state->p_exception_string.length() > 0) throw std::runtime_error(state->p_exception_string.c_str());

    if (!state->complete && state->exponent) throw std::runtime_error("Incomplete exponent");

    FloatingPoint result = static_cast<FloatingPoint>((state->minus == 2) ? -state->result : state->result);
    if (state->exponent_minus == 2) state->exponent_number = 0 - state->exponent_number;

    if (state->after_comma > 0) state->exponent_number -= state->after_comma - 1;

    if (state->exponent_number >= 0)
    {
        result *= static_cast<FloatingPoint>(
            std::pow(static_cast<FloatingPoint>(10.0), static_cast<FloatingPoint>(state->exponent_number)));

#pragma push_macro("max")
#undef max

        if (result > std::numeric_limits<FloatingPoint>::max() || result < -std::numeric_limits<FloatingPoint>::max())
            throw std::overflow_error("The value is too big");
#pragma pop_macro("max")
    }
    else
    {
        bool is_zero = (result == 0);

        result /= static_cast<FloatingPoint>(
            std::pow(static_cast<FloatingPoint>(10.0), static_cast<FloatingPoint>(-state->exponent_number)));

        if (!is_zero && result > -std::numeric_limits<FloatingPoint>::denorm_min() &&
            result < std::numeric_limits<FloatingPoint>::denorm_min())
            throw std::underflow_error("The value is too small");
    }

    return pplx::task_from_result<FloatingPoint>(result);
}

template<typename CharType>
class type_parser<CharType, double> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<double> parse(streams::streambuf<CharType> buffer)
    {
        return base::template _parse_input<_double_state<double>, double>(
            buffer, _accept_char<double, int_type>, _extract_result<double>);
    }

protected:
};

template<typename CharType>
class type_parser<CharType, float> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<float> parse(streams::streambuf<CharType> buffer)
    {
        return base::template _parse_input<_double_state<float>, float>(
            buffer, _accept_char<float, int_type>, _extract_result<float>);
    }

protected:
};

template<typename CharType>
class type_parser<CharType, uint64_t> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<uint64_t> parse(streams::streambuf<CharType> buffer)
    {
        return base::template _parse_input<_uint64_state, uint64_t>(buffer, _accept_char, _extract_result);
    }

private:
    struct _uint64_state
    {
        _uint64_state() : result(0), correct(false) {}
        uint64_t result;
        bool correct;
    };

    static bool _accept_char(std::shared_ptr<_uint64_state> state, int_type ch)
    {
        if (!::isdigit(ch)) return false;

        // At least one digit was found.
        state->correct = true;

        // Shift the existing value by 10, then add the new value.
        state->result *= 10;
        state->result += uint64_t(ch - int_type('0'));

        return true;
    }

    static pplx::task<uint64_t> _extract_result(std::shared_ptr<_uint64_state> state)
    {
        if (!state->correct) throw std::range_error("integer value is too large to fit in 64 bits");
        return pplx::task_from_result(state->result);
    }
};

template<typename CharType>
class type_parser<CharType, bool> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<bool> parse(streams::streambuf<CharType> buffer)
    {
        return base::template _parse_input<_bool_state, bool>(buffer, _accept_char, _extract_result);
    }

private:
    struct _bool_state
    {
        _bool_state() : state(0) {}
        // { 0 -- not started, 1 -- 't', 2 -- 'tr', 3 -- 'tru', 4 -- 'f', 5 -- 'fa', 6 -- 'fal', 7 -- 'fals', 8 --
        // 'true', 9 -- 'false' }
        short state;
    };

    static bool _accept_char(std::shared_ptr<_bool_state> state, int_type ch)
    {
        switch (state->state)
        {
            case 0:
                if (ch == int_type('t'))
                    state->state = 1;
                else if (ch == int_type('f'))
                    state->state = 4;
                else if (ch == int_type('1'))
                    state->state = 8;
                else if (ch == int_type('0'))
                    state->state = 9;
                else
                    return false;
                break;
            case 1:
                if (ch == int_type('r'))
                    state->state = 2;
                else
                    return false;
                break;
            case 2:
                if (ch == int_type('u'))
                    state->state = 3;
                else
                    return false;
                break;
            case 3:
                if (ch == int_type('e'))
                    state->state = 8;
                else
                    return false;
                break;
            case 4:
                if (ch == int_type('a'))
                    state->state = 5;
                else
                    return false;
                break;
            case 5:
                if (ch == int_type('l'))
                    state->state = 6;
                else
                    return false;
                break;
            case 6:
                if (ch == int_type('s'))
                    state->state = 7;
                else
                    return false;
                break;
            case 7:
                if (ch == int_type('e'))
                    state->state = 9;
                else
                    return false;
                break;
            case 8:
            case 9: return false;
        }
        return true;
    }
    static pplx::task<bool> _extract_result(std::shared_ptr<_bool_state> state)
    {
        bool correct = (state->state == 8 || state->state == 9);
        if (!correct)
        {
            std::runtime_error exc("cannot parse as Boolean value");
            throw exc;
        }
        return pplx::task_from_result(state->state == 8);
    }
};

template<typename CharType>
class type_parser<CharType, signed char> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<signed char> parse(streams::streambuf<CharType> buffer)
    {
        return base::_skip_whitespace(buffer).then([=](pplx::task<void> op) -> pplx::task<signed char> {
            op.wait();
            return type_parser<CharType, signed char>::_get_char(buffer);
        });
    }

private:
    static pplx::task<signed char> _get_char(streams::streambuf<CharType> buffer)
    {
        concurrency::streams::streambuf<CharType> buf = buffer;
        return buf.bumpc().then([=](pplx::task<int_type> op) -> signed char {
            int_type val = op.get();
            if (val == traits::eof()) throw std::runtime_error("reached end-of-stream while constructing a value");
            return static_cast<signed char>(val);
        });
    }
};

template<typename CharType>
class type_parser<CharType, unsigned char> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<unsigned char> parse(streams::streambuf<CharType> buffer)
    {
        return base::_skip_whitespace(buffer).then([=](pplx::task<void> op) -> pplx::task<unsigned char> {
            op.wait();
            return type_parser<CharType, unsigned char>::_get_char(buffer);
        });
    }

private:
    static pplx::task<unsigned char> _get_char(streams::streambuf<CharType> buffer)
    {
        concurrency::streams::streambuf<CharType> buf = buffer;
        return buf.bumpc().then([=](pplx::task<int_type> op) -> unsigned char {
            int_type val = op.get();
            if (val == traits::eof()) throw std::runtime_error("reached end-of-stream while constructing a value");
            return static_cast<unsigned char>(val);
        });
    }
};

template<typename CharType>
class type_parser<CharType, char> : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<char> parse(streams::streambuf<CharType> buffer)
    {
        return base::_skip_whitespace(buffer).then([=](pplx::task<void> op) -> pplx::task<char> {
            op.wait();
            return _get_char(buffer);
        });
    }

private:
    static pplx::task<char> _get_char(streams::streambuf<CharType> buffer)
    {
        concurrency::streams::streambuf<CharType> buf = buffer;
        return buf.bumpc().then([=](pplx::task<int_type> op) -> char {
            int_type val = op.get();
            if (val == traits::eof()) throw std::runtime_error("reached end-of-stream while constructing a value");
            return char(val);
        });
    }
};

#ifdef _WIN32
template<class CharType>
class type_parser<CharType, std::enable_if_t<sizeof(CharType) == 1, std::basic_string<wchar_t>>>
    : public _type_parser_base<CharType>
{
    typedef _type_parser_base<CharType> base;

public:
    typedef typename base::traits traits;
    typedef typename base::int_type int_type;

    static pplx::task<std::wstring> parse(streams::streambuf<CharType> buffer)
    {
        return base::template _parse_input<std::basic_string<char>, std::basic_string<wchar_t>>(
            buffer, _accept_char, _extract_result);
    }

private:
    static bool _accept_char(const std::shared_ptr<std::basic_string<char>>& state, int_type ch)
    {
        if (ch == concurrency::streams::char_traits<char>::eof() || isspace(ch)) return false;
        state->push_back(char(ch));
        return true;
    }
    static pplx::task<std::basic_string<wchar_t>> _extract_result(std::shared_ptr<std::basic_string<char>> state)
    {
        return pplx::task_from_result(utility::conversions::utf8_to_utf16(*state));
    }
};
#endif //_WIN32

} // namespace streams
} // namespace Concurrency

#endif
