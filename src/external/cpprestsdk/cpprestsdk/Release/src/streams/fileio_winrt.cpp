/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Asynchronous I/O: stream buffer implementation details
 *
 * We're going to some lengths to avoid exporting C++ class member functions and implementation details across
 * module boundaries, and the factoring requires that we keep the implementation details away from the main header
 * files. The supporting functions, which are in this file, use C-like signatures to avoid as many issues as
 * possible.
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

#include "cpprest/details/fileio.h"
#include "cpprest/interopstream.h"
#include "robuffer.h"

using namespace ::Windows::Foundation;
using namespace ::Windows::Storage;
using namespace ::Windows::Storage::Streams;
using namespace ::Windows::Networking;
using namespace ::Windows::Networking::Sockets;

namespace Concurrency
{
namespace streams
{
namespace details
{
/// <summary>
/// The public parts of the file information record contain only what is implementation-
/// independent. The actual allocated record is larger and has details that the implementation
/// require in order to function.
/// </summary>
struct _file_info_impl : _file_info
{
    _file_info_impl(Streams::IRandomAccessStream ^ stream, std::ios_base::openmode mode)
        : m_stream(stream), m_writer(nullptr), _file_info(mode, 0)
    {
        m_pendingWrites = pplx::task_from_result();
    }

    _file_info_impl(Streams::IRandomAccessStream ^ stream, std::ios_base::openmode mode, size_t buffer_size)
        : m_stream(stream), m_writer(nullptr), _file_info(mode, buffer_size)
    {
        m_pendingWrites = pplx::task_from_result();
    }

    Streams::IRandomAccessStream ^ m_stream;
    Streams::IDataWriter ^ m_writer;
    pplx::task<void> m_pendingWrites;
};

} // namespace details
} // namespace streams
} // namespace Concurrency

using namespace Concurrency::streams::details;

#pragma warning(push)
#pragma warning(disable : 4100)

/// <summary>
/// Translate from C++ STL file open modes to Win32 flags.
/// </summary>
/// <param name="mode">The C++ file open mode</param>
/// <param name="prot">The C++ file open protection</param>
/// <param name="dwDesiredAccess">A pointer to a DWORD that will hold the desired access flags</param>
/// <param name="dwCreationDisposition">A pointer to a DWORD that will hold the creation disposition</param>
/// <param name="dwShareMode">A pointer to a DWORD that will hold the share mode</param>
void _get_create_flags(std::ios_base::openmode mode,
                       int prot,
                       FileAccessMode& acc_mode,
                       CreationCollisionOption& options)
{
    options = CreationCollisionOption::OpenIfExists;
    acc_mode = FileAccessMode::ReadWrite;

    if ((mode & std::ios_base::in) && !(mode & std::ios_base::out)) acc_mode = FileAccessMode::Read;
    if (mode & std::ios_base::trunc) options = CreationCollisionOption::ReplaceExisting;
}

/// <summary>
/// Perform post-CreateFile processing.
/// </summary>
/// <param name="fh">The Win32 file handle</param>
/// <param name="callback">The callback interface pointer</param>
/// <param name="mode">The C++ file open mode</param>
/// <returns>The error code if there was an error in file creation.</returns>
void _finish_create(Streams::IRandomAccessStream ^ stream,
                    _In_ _filestream_callback* callback,
                    std::ios_base::openmode mode,
                    int prot)
{
    _file_info_impl* info = nullptr;

    info = new _file_info_impl(stream, mode, 512);

    // Seek to end if it's in appending write mode
    if ((mode & std::ios_base::out) && (mode & std::ios_base::app || mode & std::ios_base::ate))
    {
        _seekwrpos_fsb(info, static_cast<size_t>(stream->Size), 1);
    }

    callback->on_opened(info);
}

/// <summary>
/// Create a streambuf instance to represent a WinRT file.
/// </summary>
/// <param name="callback">A pointer to the callback interface to invoke when the file has been opened.</param>
/// <param name="file">The file object</param>
/// <param name="mode">A creation mode for the stream buffer</param>
/// <returns>True if the opening operation could be initiated, false otherwise.</returns>
/// <remarks>
/// True does not signal that the file will eventually be successfully opened, just that the process was started.
/// This is only available for WinRT.
/// </remarks>
bool __cdecl _open_fsb_stf_str(_In_ Concurrency::streams::details::_filestream_callback* callback,
                               ::Windows::Storage::StorageFile ^ file,
                               std::ios_base::openmode mode,
                               int prot)
{
    _ASSERTE(callback != nullptr);
    _ASSERTE(file != nullptr);

    CreationCollisionOption options;
    FileAccessMode acc_mode;

    _get_create_flags(mode, prot, acc_mode, options);

    pplx::create_task(file->OpenAsync(acc_mode)).then([=](pplx::task<Streams::IRandomAccessStream ^> sop) {
        try
        {
            _finish_create(sop.get(), callback, mode, prot);
        }
        catch (Platform::Exception ^ exc)
        {
            callback->on_error(std::make_exception_ptr(utility::details::create_system_error(exc->HResult)));
        }
    });

    return true;
}

bool __cdecl _sync_fsb_winrt(_In_ Concurrency::streams::details::_file_info* info,
                             _In_opt_ Concurrency::streams::details::_filestream_callback* callback)
{
    _ASSERTE(info != nullptr);

    _file_info_impl* fInfo = static_cast<_file_info_impl*>(info);

    pplx::extensibility::scoped_recursive_lock_t lck(fInfo->m_lock);

    if (fInfo->m_stream == nullptr || fInfo->m_writer == nullptr || !fInfo->m_stream->CanWrite) return false;

    // take a snapshot of current writer, since writer can be replaced during flush
    auto writer = fInfo->m_writer;
    // Flush operation will not begin until all previous writes (StoreAsync) finished, thus it could avoid race.
    fInfo->m_pendingWrites = fInfo->m_pendingWrites.then([=] { return writer->StoreAsync(); })
                                 .then([=](unsigned int) {
                                     fInfo->m_buffill = 0;
                                     return writer->FlushAsync();
                                 })
                                 .then([=](pplx::task<bool> result) {
                                     // Rethrow exception if no callback attached.
                                     if (callback == nullptr)
                                         result.wait();
                                     else
                                     {
                                         try
                                         {
                                             result.wait();
                                             callback->on_completed(0);
                                         }
                                         catch (Platform::Exception ^ exc)
                                         {
                                             callback->on_error(std::make_exception_ptr(
                                                 utility::details::create_system_error(exc->HResult)));
                                         }
                                     }
                                 });
    return true;
}

/// <summary>
/// Close a file stream buffer.
/// </summary>
/// <param name="info">The file info record of the file</param>
/// <param name="callback">A pointer to the callback interface to invoke when the file has been opened.</param>
/// <returns>True if the closing operation could be initiated, false otherwise.</returns>
/// <remarks>
/// True does not signal that the file will eventually be successfully closed, just that the process was started.
/// </remarks>
bool __cdecl _close_fsb_nolock(_In_ _file_info** info,
                               _In_ Concurrency::streams::details::_filestream_callback* callback)
{
    _ASSERTE(callback != nullptr);
    _ASSERTE(info != nullptr);
    _ASSERTE(*info != nullptr);

    _file_info_impl* fInfo = static_cast<_file_info_impl*>(*info);

    *info = nullptr;

    auto stream = fInfo->m_stream;

    if (fInfo->m_stream->CanWrite)
    {
        _sync_fsb_winrt(fInfo, nullptr);
        fInfo->m_pendingWrites.then([=](pplx::task<void> t) {
            try
            {
                // The lock fInfo->m_lock must not be held at this point
                delete fInfo;
                t.wait();
                callback->on_closed();
            }
            catch (Platform::Exception ^ exc)
            {
                callback->on_error(std::make_exception_ptr(utility::details::create_system_error(exc->HResult)));
            }
        });
    }
    else
    {
        // The lock fInfo->m_lock must not be held at this point
        delete fInfo;
        callback->on_closed();
    }
    return true;
}

bool __cdecl _close_fsb(_In_ _file_info** info, _In_ Concurrency::streams::details::_filestream_callback* callback)
{
    return _close_fsb_nolock(info, callback);
}

/// <summary>
/// Initiate an asynchronous (overlapped) read from the file stream.
/// </summary>
/// <param name="info">The file info record of the file</param>
/// <param name="callback">A pointer to the callback interface to invoke when the write request is completed.</param>
/// <param name="ptr">A pointer to a buffer where the data should be placed</param>
/// <param name="count">The size (in bytes) of the buffer</param>
/// <param name="offset">The offset in the file to read from</param>
/// <returns>0 if the read request is still outstanding, -1 if the request failed, otherwise the size of the data read
/// into the buffer</returns>
size_t __cdecl _read_file_async(_In_ Concurrency::streams::details::_file_info_impl* fInfo,
                                _In_ Concurrency::streams::details::_filestream_callback* callback,
                                _Out_writes_(count) void* ptr,
                                _In_ size_t count,
                                size_t offset)
{
    if (fInfo->m_stream == nullptr)
    {
        if (callback != nullptr)
        {
            // I don't know of a better error code, so this will have to do.
            callback->on_error(std::make_exception_ptr(utility::details::create_system_error(ERROR_INVALID_ADDRESS)));
        }
        return 0;
    }

    auto reader = ref new Streams::DataReader(fInfo->m_stream->GetInputStreamAt(offset));

    pplx::create_task(reader->LoadAsync(static_cast<unsigned int>(count))).then([=](pplx::task<unsigned int> result) {
        try
        {
            auto read = result.get();

            if (read > 0)
            {
                reader->ReadBytes(Platform::ArrayReference<unsigned char>(static_cast<unsigned char*>(ptr), read));
            }

            callback->on_completed(read);
        }
        catch (Platform::Exception ^ exc)
        {
            callback->on_error(std::make_exception_ptr(utility::details::create_system_error(exc->HResult)));
        }
    });

    return 0;
}

template<typename Func>
class _filestream_callback_fill_buffer : public _filestream_callback
{
public:
    _filestream_callback_fill_buffer(_In_ _file_info* info, const Func& func) : m_func(func), m_info(info) {}

    virtual void on_completed(size_t result)
    {
        m_func(result);
        delete this;
    }

private:
    _file_info* m_info;
    Func m_func;
};

template<typename Func>
_filestream_callback_fill_buffer<Func>* create_callback(_In_ _file_info* info, const Func& func)
{
    return new _filestream_callback_fill_buffer<Func>(info, func);
}

size_t _fill_buffer_fsb(_In_ _file_info_impl* fInfo,
                        _In_ _filestream_callback* callback,
                        size_t count,
                        size_t char_size)
{
    msl::safeint3::SafeInt<size_t> safeCount = count;

    if (fInfo->m_buffer == nullptr || safeCount > fInfo->m_bufsize)
    {
        if (fInfo->m_buffer != nullptr) delete fInfo->m_buffer;

        fInfo->m_bufsize = safeCount.Max(fInfo->m_buffer_size);
        fInfo->m_buffer = new char[fInfo->m_bufsize * char_size];
        fInfo->m_bufoff = fInfo->m_rdpos;

        auto cb = create_callback(fInfo, [=](size_t result) {
            {
                pplx::extensibility::scoped_recursive_lock_t lck(fInfo->m_lock);
                fInfo->m_buffill = result / char_size;
            }
            callback->on_completed(result);
        });

        auto read = _read_file_async(
            fInfo, cb, (uint8_t*)fInfo->m_buffer, fInfo->m_bufsize * char_size, fInfo->m_rdpos * char_size);

        switch (read)
        {
            case 0:
                // pending
                return read;

            case (-1):
                // error
                delete cb;
                return read;

            default:
                // operation is complete. The pattern of returning synchronously
                // has the expectation that we duplicate the callback code here...
                // Do the expedient thing for now.
                cb->on_completed(read);

                // return pending
                return 0;
        };
    }

    // First, we need to understand how far into the buffer we have already read
    // and how much remains.

    size_t bufpos = fInfo->m_rdpos - fInfo->m_bufoff;
    size_t bufrem = fInfo->m_buffill - bufpos;

    // We have four different scenarios:
    //  1. The read position is before the start of the buffer, in which case we will just reuse the buffer.
    //  2. The read position is in the middle of the buffer, and we need to read some more.
    //  3. The read position is beyond the end of the buffer. Do as in #1.
    //  4. We have everything we need.

    if ((fInfo->m_rdpos < fInfo->m_bufoff) || (fInfo->m_rdpos >= (fInfo->m_bufoff + fInfo->m_buffill)))
    {
        // Reuse the existing buffer.

        fInfo->m_bufoff = fInfo->m_rdpos;

        auto cb = create_callback(fInfo, [=](size_t result) {
            {
                pplx::extensibility::scoped_recursive_lock_t lck(fInfo->m_lock);
                fInfo->m_buffill = result / char_size;
            }
            callback->on_completed(bufrem * char_size + result);
        });

        auto read = _read_file_async(
            fInfo, cb, (uint8_t*)fInfo->m_buffer, fInfo->m_bufsize * char_size, fInfo->m_rdpos * char_size);

        switch (read)
        {
            case 0:
                // pending
                return read;

            case (-1):
                // error
                delete cb;
                return read;

            default:
                // operation is complete. The pattern of returning synchronously
                // has the expectation that we duplicate the callback code here...
                // Do the expedient thing for now.
                cb->on_completed(read);

                // return pending
                return 0;
        };
    }
    else if (bufrem < count)
    {
        fInfo->m_bufsize = safeCount.Max(fInfo->m_buffer_size);

        // Then, we allocate a new buffer.

        char* newbuf = new char[fInfo->m_bufsize * char_size];

        // Then, we copy the unread part to the new buffer and delete the old buffer

        if (bufrem > 0) memcpy(newbuf, fInfo->m_buffer + bufpos * char_size, bufrem * char_size);

        delete fInfo->m_buffer;
        fInfo->m_buffer = newbuf;

        // Then, we read the remainder of the count into the new buffer
        fInfo->m_bufoff = fInfo->m_rdpos;

        auto cb = create_callback(fInfo, [=](size_t result) {
            {
                pplx::extensibility::scoped_recursive_lock_t lck(fInfo->m_lock);
                fInfo->m_buffill = result / char_size;
            }
            callback->on_completed(bufrem * char_size + result);
        });

        auto read = _read_file_async(fInfo,
                                     cb,
                                     (uint8_t*)fInfo->m_buffer + bufrem * char_size,
                                     (fInfo->m_bufsize - bufrem) * char_size,
                                     (fInfo->m_rdpos + bufrem) * char_size);

        switch (read)
        {
            case 0:
                // pending
                return read;

            case (-1):
                // error
                delete cb;
                return read;

            default:
                // operation is complete. The pattern of returning synchronously
                // has the expectation that we duplicate the callback code here...
                // Do the expedient thing for now.
                cb->on_completed(read);

                // return pending
                return 0;
        };
    }
    else
    {
        // If we are here, it means that we didn't need to read, we already have enough data in the buffer
        return count * char_size;
    }
}

/// <summary>
/// Read data from a file stream into a buffer
/// </summary>
/// <param name="info">The file info record of the file</param>
/// <param name="callback">A pointer to the callback interface to invoke when the write request is completed.</param>
/// <param name="ptr">A pointer to a buffer where the data should be placed</param>
/// <param name="count">The size (in characters) of the buffer</param>
/// <returns>0 if the read request is still outstanding, -1 if the request failed, otherwise the size of the data read
/// into the buffer</returns>
size_t __cdecl _getn_fsb(_In_ Concurrency::streams::details::_file_info* info,
                         _In_ Concurrency::streams::details::_filestream_callback* callback,
                         _Out_writes_(count) void* ptr,
                         _In_ size_t count,
                         size_t char_size)
{
    _ASSERTE(callback != nullptr);
    _ASSERTE(info != nullptr);
    _ASSERTE(count > 0);

    _file_info_impl* fInfo = static_cast<_file_info_impl*>(info);

    pplx::extensibility::scoped_recursive_lock_t lck(info->m_lock);

    if (fInfo->m_buffer_size > 0)
    {
        auto cb = create_callback(fInfo, [=](size_t read) {
            auto sz = count * char_size;
            auto copy = (read < sz) ? read : sz;
            auto bufoff = fInfo->m_rdpos - fInfo->m_bufoff;
            memcpy(ptr, fInfo->m_buffer + bufoff * char_size, copy);
            fInfo->m_atend = copy < sz;
            callback->on_completed(copy);
        });

        size_t read = _fill_buffer_fsb(fInfo, cb, count, char_size);

        if (read > 0)
        {
            auto sz = count * char_size;
            auto copy = (read < sz) ? read : sz;
            auto bufoff = fInfo->m_rdpos - fInfo->m_bufoff;
            memcpy(ptr, fInfo->m_buffer + bufoff * char_size, copy);
            fInfo->m_atend = copy < sz;
            return copy;
        }

        return (size_t)read;
    }
    else
    {
        return _read_file_async(fInfo, callback, ptr, count * char_size, fInfo->m_rdpos * char_size);
    }
}

/// <summary>
/// Write data from a buffer into the file stream.
/// </summary>
/// <param name="info">The file info record of the file</param>
/// <param name="callback">A pointer to the callback interface to invoke when the write request is completed.</param>
/// <param name="ptr">A pointer to a buffer where the data should be placed</param>
/// <param name="count">The size (in characters) of the buffer</param>
/// <returns>0 if the write request is still outstanding, -1 if the request failed, otherwise the size of the data read
/// into the buffer</returns>
size_t __cdecl _putn_fsb(_In_ Concurrency::streams::details::_file_info* info,
                         _In_ Concurrency::streams::details::_filestream_callback* callback,
                         const void* ptr,
                         size_t count,
                         size_t char_size)
{
    _ASSERTE(callback != nullptr);
    _ASSERTE(info != nullptr);
    _ASSERTE(count > 0);

    _file_info_impl* fInfo = static_cast<_file_info_impl*>(info);

    pplx::extensibility::scoped_recursive_lock_t lck(fInfo->m_lock);

    if (fInfo->m_stream == nullptr) return static_cast<size_t>(-1);

    // To preserve the async write order, we have to move the write head before read.
    if (fInfo->m_wrpos != static_cast<size_t>(-1)) fInfo->m_wrpos += count;

    msl::safeint3::SafeInt<unsigned int> safeWriteSize = count;
    safeWriteSize *= char_size;

    // In most of the time, we preserve the writer so that it would have better performance.
    // However, after user call seek, we will dispose old writer. By doing so, users could
    // write to new writer in new position while the old writer is still flushing data into stream.
    if (fInfo->m_writer == nullptr)
    {
        fInfo->m_writer = ref new Streams::DataWriter(fInfo->m_stream);
        fInfo->m_buffill = 0;
    }

    // It keeps tracking the number of bytes written into m_writer buffer.
    fInfo->m_buffill += count;

    // ArrayReference here is for avoiding data copy.
    fInfo->m_writer->WriteBytes(Platform::ArrayReference<unsigned char>(
        const_cast<unsigned char*>(static_cast<const unsigned char*>(ptr)), safeWriteSize));

    // Flush data from m_writer buffer into stream , if the buffer is full
    if (fInfo->m_buffill >= fInfo->m_buffer_size)
    {
        fInfo->m_buffill = 0;
        fInfo->m_pendingWrites = fInfo->m_pendingWrites.then([=] { return fInfo->m_writer->StoreAsync(); })
                                     .then([=](pplx::task<unsigned int> result) {
                                         try
                                         {
                                             result.wait();
                                             callback->on_completed(safeWriteSize);
                                         }
                                         catch (Platform::Exception ^ exc)
                                         {
                                             callback->on_error(std::make_exception_ptr(
                                                 utility::details::create_system_error(exc->HResult)));
                                         }
                                     });
        return 0;
    }
    else
        return safeWriteSize;
}

/// <summary>
/// Flush all buffered data to the underlying file.
/// </summary>
/// <param name="info">The file info record of the file</param>
/// <param name="callback">A pointer to the callback interface to invoke when the write request is completed.</param>
/// <returns>True if the request was initiated</returns>
bool __cdecl _sync_fsb(_In_ Concurrency::streams::details::_file_info* info,
                       _In_ Concurrency::streams::details::_filestream_callback* callback)
{
    return _sync_fsb_winrt(info, callback);
}

/// <summary>
/// Adjust the internal buffers and pointers when the application seeks to a new read location in the stream.
/// </summary>
/// <param name="info">The file info record of the file</param>
/// <param name="pos">The new position (offset from the start) in the file stream</param>
/// <returns>New file position or -1 if error</returns>
size_t __cdecl _seekrdpos_fsb(_In_ Concurrency::streams::details::_file_info* info, size_t pos, size_t char_size)
{
    _ASSERTE(info != nullptr);

    _file_info_impl* fInfo = static_cast<_file_info_impl*>(info);

    pplx::extensibility::scoped_recursive_lock_t lck(info->m_lock);

    if (fInfo->m_stream == nullptr) return static_cast<size_t>(-1);
    ;

    if (pos < fInfo->m_bufoff || pos > (fInfo->m_bufoff + fInfo->m_buffill))
    {
        delete fInfo->m_buffer;
        fInfo->m_buffer = nullptr;
        fInfo->m_bufoff = fInfo->m_buffill = fInfo->m_bufsize = 0;
    }

    fInfo->m_rdpos = pos;

    return fInfo->m_rdpos;
}

/// <summary>
/// Adjust the internal buffers and pointers when the application seeks to a new read location in the stream.
/// </summary>
/// <param name="info">The file info record of the file</param>
/// <param name="offset">The new position (offset from the end of the stream) in the file stream</param>
/// <param name="char_size">The size of the character type used for this stream</param>
/// <returns>New file position or -1 if error</returns>
_ASYNCRTIMP size_t __cdecl _seekrdtoend_fsb(_In_ Concurrency::streams::details::_file_info* info,
                                            int64_t offset,
                                            size_t char_size)
{
    _ASSERTE(info != nullptr);
    _file_info_impl* fInfo = static_cast<_file_info_impl*>(info);

    return _seekrdpos_fsb(info, static_cast<size_t>(fInfo->m_stream->Size / char_size + offset), char_size);
}

utility::size64_t __cdecl _get_size(_In_ concurrency::streams::details::_file_info* info, size_t char_size)
{
    _ASSERTE(info != nullptr);

    _file_info_impl* fInfo = static_cast<_file_info_impl*>(info);

    pplx::extensibility::scoped_recursive_lock_t lck(info->m_lock);

    if (fInfo->m_stream == nullptr) return 0;

    return utility::size64_t(fInfo->m_stream->Size / char_size);
}

/// <summary>
/// Adjust the internal buffers and pointers when the application seeks to a new write location in the stream.
/// </summary>
/// <param name="info">The file info record of the file</param>
/// <param name="pos">The new position (offset from the start) in the file stream</param>
/// <returns>New file position or -1 if error</returns>
size_t __cdecl _seekwrpos_fsb(_In_ Concurrency::streams::details::_file_info* info, size_t pos, size_t char_size)
{
    _ASSERTE(info != nullptr);

    _file_info_impl* fInfo = static_cast<_file_info_impl*>(info);

    pplx::extensibility::scoped_recursive_lock_t lck(info->m_lock);

    if (fInfo->m_stream == nullptr) return static_cast<size_t>(-1);

    fInfo->m_wrpos = pos;

    // m_buffill keeps number of chars written into the m_writer buffer.
    // We need to flush it into stream before seek the write head of the stream
    if (fInfo->m_buffill > 0) _sync_fsb_winrt(fInfo, nullptr);

    // Moving write head should follow the flush operation. is_done test is for perf optimization.
    if (fInfo->m_pendingWrites.is_done())
        fInfo->m_stream->Seek(static_cast<long long>(pos) * char_size);
    else
    {
        auto lastWriter = fInfo->m_writer;
        fInfo->m_writer = nullptr;

        fInfo->m_pendingWrites = fInfo->m_pendingWrites.then([=] {
            // Detach stream could avoid stream destruction after writer get destructed.
            lastWriter->DetachStream();
            fInfo->m_stream->Seek(static_cast<long long>(pos) * char_size);
        });
    }

    return fInfo->m_wrpos;
}

namespace Concurrency
{
namespace streams
{
namespace details
{
/// <summary>
/// This class acts as a bridge between WinRT input streams and Casablanca asynchronous streams.
/// </summary>
ref class IRandomAccessStream_bridge sealed : public Windows::Storage::Streams::IRandomAccessStream
{
public:
    virtual property bool CanRead
    {
        bool get() { return m_buffer.can_read(); }
    }

    virtual property bool CanWrite
    {
        bool get() { return m_buffer.can_write(); }
    }

    virtual property uint64_t Position
    {
        uint64_t get() { return m_position; }
    }

    virtual property uint64_t Size
    {
        uint64_t get()
        {
            if (!m_buffer.has_size()) return m_remembered_size;
            return m_buffer.size();
        }

        void set(uint64_t sz)
        {
            if (!m_buffer.has_size() || !m_buffer.can_write())
                m_remembered_size = sz;
            else
                m_buffer.seekoff(basic_streambuf<uint8_t>::pos_type(sz), std::ios_base::beg, std::ios_base::out);
        }
    }

    virtual Windows::Storage::Streams::IRandomAccessStream ^
        CloneStream() { return ref new IRandomAccessStream_bridge(m_buffer); }

        virtual Windows::Storage::Streams::IInputStream
        ^
        GetInputStreamAt(uint64_t position) {
            if (!m_buffer.can_read()) return nullptr;

            concurrency::streams::streambuf<uint8_t>::pos_type pos = position;

            if (m_buffer.can_seek() || pos == m_buffer.getpos(std::ios_base::in))
            {
                return ref new IRandomAccessStream_bridge(m_buffer, position);
            }
            return nullptr;
        }

        virtual Windows::Storage::Streams::IOutputStream
        ^ GetOutputStreamAt(uint64_t position) {
              if (!m_buffer.can_write()) return nullptr;

              concurrency::streams::streambuf<uint8_t>::pos_type pos = position;

              if (m_buffer.can_seek() || pos == m_buffer.getpos(std::ios_base::out))
              {
                  return ref new IRandomAccessStream_bridge(m_buffer, position);
              }
              return nullptr;
          };

    virtual void Seek(uint64_t position)
    {
        if (!m_buffer.can_seek()) throw ref new Platform::InvalidArgumentException(L"underlying buffer cannot seek");

        m_position = position;

        m_buffer.seekpos(concurrency::streams::streambuf<uint8_t>::pos_type(m_position), std::ios_base::in);
        m_buffer.seekpos(concurrency::streams::streambuf<uint8_t>::pos_type(m_position), std::ios_base::out);
    }

    virtual Windows::Foundation::IAsyncOperationWithProgress<unsigned int, unsigned int> ^
        WriteAsync(Windows::Storage::Streams::IBuffer ^ buffer);
    virtual Windows::Foundation::IAsyncOperationWithProgress<::Windows::Storage::Streams::IBuffer ^, unsigned int> ^
        ReadAsync(::Windows::Storage::Streams::IBuffer ^ buffer,
                  unsigned int count,
                  Windows::Storage::Streams::InputStreamOptions options);
    virtual Windows::Foundation::IAsyncOperation<bool> ^ FlushAsync();

    virtual ~IRandomAccessStream_bridge() {}

    internal :

        IRandomAccessStream_bridge(const concurrency::streams::streambuf<uint8_t>& buffer)
        : m_buffer(buffer), m_remembered_size(0), m_position(0)
    {
    }

    IRandomAccessStream_bridge(const concurrency::streams::streambuf<uint8_t>& buffer,
                               concurrency::streams::streambuf<uint8_t>::pos_type position)
        : m_buffer(buffer), m_remembered_size(0), m_position(position)
    {
    }

private:
    uint64_t m_remembered_size;
    concurrency::streams::streambuf<uint8_t>::pos_type m_position;
    concurrency::streams::streambuf<uint8_t> m_buffer;
};

struct _alloc_protector
{
    _alloc_protector(concurrency::streams::streambuf<uint8_t>& buffer) : m_buffer(buffer), m_size(0) {}

    ~_alloc_protector() { m_buffer.commit(m_size); }

    size_t m_size;

private:
    _alloc_protector& operator=(const _alloc_protector&);

    concurrency::streams::streambuf<uint8_t>& m_buffer;
};

struct _acquire_protector
{
    _acquire_protector(concurrency::streams::streambuf<uint8_t>& buffer, uint8_t* ptr)
        : m_buffer(buffer), m_ptr(ptr), m_size(0)
    {
    }

    ~_acquire_protector() { m_buffer.release(m_ptr, m_size); }

    size_t m_size;

private:
    _acquire_protector& operator=(const _acquire_protector&);

    uint8_t* m_ptr;
    concurrency::streams::streambuf<uint8_t>& m_buffer;
};

// Rather than using ComPtr, which is somewhat complex, a simple RAII class
// to make sure that Release() is called is useful here.
struct _IUnknown_protector
{
    _IUnknown_protector(IUnknown* unk_ptr) : m_unknown(unk_ptr) {}
    ~_IUnknown_protector()
    {
        if (m_unknown != nullptr) m_unknown->Release();
    }

private:
    IUnknown* m_unknown;
};

Windows::Foundation::IAsyncOperationWithProgress<::Windows::Storage::Streams::IBuffer ^, unsigned int> ^
    IRandomAccessStream_bridge::ReadAsync(::Windows::Storage::Streams::IBuffer ^ buffer,
                                          unsigned int count,
                                          Windows::Storage::Streams::InputStreamOptions options)
{
    if (!m_buffer.can_read())
    {
        return pplx::create_async([buffer](pplx::progress_reporter<uint32> reporter) { return buffer; });
    }

    if (buffer->Capacity < count)
        return pplx::create_async([buffer](pplx::progress_reporter<uint32> reporter) { return buffer; });

    m_buffer.seekpos(concurrency::streams::streambuf<uint8_t>::pos_type(m_position), std::ios_base::in);

    concurrency::streams::streambuf<uint8_t> streambuf = m_buffer;

    return pplx::create_async([streambuf, buffer, options, count](pplx::progress_reporter<uint32> reporter) {
        auto sbuf = streambuf;
        auto local_buf = ref new ::Platform::Array<unsigned char, 1>(count);

        uint8_t* ptr = nullptr;
        size_t acquired_size = 0;

        if (sbuf.acquire(ptr, acquired_size) && acquired_size >= count)
        {
            _acquire_protector prot(sbuf, ptr);

            IUnknown* pUnk = reinterpret_cast<IUnknown*>(buffer);
            ::Windows::Storage::Streams::IBufferByteAccess* pBufferByteAccess = nullptr;
            HRESULT hr = pUnk->QueryInterface(IID_PPV_ARGS(&pBufferByteAccess));
            __abi_ThrowIfFailed(hr);

            _IUnknown_protector unkprot(pBufferByteAccess);

            byte* buffer_data = nullptr;
            hr = pBufferByteAccess->Buffer(&buffer_data);
            __abi_ThrowIfFailed(hr);

            memcpy(buffer_data, ptr, count);

            prot.m_size = count;
            buffer->Length = count;

            return pplx::task_from_result(buffer);
        }
        else
        {
            if (acquired_size > 0)
            {
                sbuf.release(ptr, 0);
            }

            IUnknown* pUnk = reinterpret_cast<IUnknown*>(buffer);
            ::Windows::Storage::Streams::IBufferByteAccess* pBufferByteAccess = nullptr;
            HRESULT hr = pUnk->QueryInterface(IID_PPV_ARGS(&pBufferByteAccess));
            __abi_ThrowIfFailed(hr);

            _IUnknown_protector unkprot(pBufferByteAccess);

            byte* buffer_data = nullptr;
            hr = pBufferByteAccess->Buffer(&buffer_data);
            __abi_ThrowIfFailed(hr);

            pBufferByteAccess->AddRef();

            return sbuf.getn(buffer_data, count).then([buffer, pBufferByteAccess, count](pplx::task<size_t> written) {
                _IUnknown_protector unkprot(pBufferByteAccess);
                buffer->Length = (unsigned int)written.get();
                return pplx::task_from_result(buffer);
            });
        }
    });
}

Windows::Foundation::IAsyncOperationWithProgress<unsigned int, unsigned int> ^
    IRandomAccessStream_bridge::WriteAsync(Windows::Storage::Streams::IBuffer ^ buffer)
{
    if (!m_buffer.can_write())
    {
        return pplx::create_async([](pplx::progress_reporter<uint32> reporter) { return 0U; });
    }

    m_buffer.seekpos(concurrency::streams::streambuf<uint8_t>::pos_type(m_position), std::ios_base::out);

    concurrency::streams::streambuf<uint8_t> streambuf = m_buffer;

    return pplx::create_async([buffer, streambuf](pplx::progress_reporter<uint32> reporter) {
        auto size = buffer->Length;
        auto sbuf = streambuf;
        uint8_t* ptr = sbuf.alloc(size);

        if (ptr != nullptr)
        {
            {
                _alloc_protector prot(sbuf);

                IUnknown* pUnk = reinterpret_cast<IUnknown*>(buffer);
                ::Windows::Storage::Streams::IBufferByteAccess* pBufferByteAccess = nullptr;
                HRESULT hr = pUnk->QueryInterface(IID_PPV_ARGS(&pBufferByteAccess));
                __abi_ThrowIfFailed(hr);

                _IUnknown_protector unkprot(pBufferByteAccess);

                byte* buffer_data = nullptr;
                hr = pBufferByteAccess->Buffer(&buffer_data);
                __abi_ThrowIfFailed(hr);

                memcpy(ptr, buffer_data, size);

                prot.m_size = size;
            }
            return pplx::task_from_result((unsigned int)size);
        }
        else
        {
            IUnknown* pUnk = reinterpret_cast<IUnknown*>(buffer);
            ::Windows::Storage::Streams::IBufferByteAccess* pBufferByteAccess = nullptr;
            HRESULT hr = pUnk->QueryInterface(IID_PPV_ARGS(&pBufferByteAccess));
            __abi_ThrowIfFailed(hr);

            _IUnknown_protector unkprot(pBufferByteAccess);

            byte* buffer_data = nullptr;
            hr = pBufferByteAccess->Buffer(&buffer_data);
            __abi_ThrowIfFailed(hr);

            pBufferByteAccess->AddRef();

            return sbuf.putn_nocopy(buffer_data, size).then([pBufferByteAccess](pplx::task<size_t> size) {
                pBufferByteAccess->Release();
                return (unsigned int)size.get();
            });
        }
    });
}

Windows::Foundation::IAsyncOperation<bool> ^ IRandomAccessStream_bridge::FlushAsync()
{
    concurrency::streams::streambuf<uint8_t> streambuf = m_buffer;
    return pplx::create_async([streambuf]() {
        if (!streambuf.can_write())
        {
            return pplx::task_from_result(false);
        }

        auto sbuf = streambuf;
        return sbuf.sync().then([] { return pplx::task_from_result(true); });
    });
}

} // namespace details
} // namespace streams
} // namespace Concurrency

Windows::Storage::Streams::IInputStream ^
    Concurrency::streams::winrt_stream::create_input_stream(const concurrency::streams::streambuf<uint8_t>& buffer)
{
    return ref new ::Concurrency::streams::details::IRandomAccessStream_bridge(buffer, 0);
}

Windows::Storage::Streams::IOutputStream ^
    Concurrency::streams::winrt_stream::create_output_stream(const concurrency::streams::streambuf<uint8_t>& buffer)
{
    return ref new Concurrency::streams::details::IRandomAccessStream_bridge(buffer, 0);
}

Windows::Storage::Streams::IRandomAccessStream ^ Concurrency::streams::winrt_stream::create_random_access_stream(
                                                     const concurrency::streams::streambuf<uint8_t>& buffer)
{
    return ref new Concurrency::streams::details::IRandomAccessStream_bridge(buffer);
}

#pragma warning(pop)
