/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Linux specific pplx implementations
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#if (defined(_MSC_VER))
#error This file must not be included for Visual Studio
#endif

#ifndef _WIN32

#include "cpprest/details/cpprest_compat.h"
#include "pthread.h"
#include <signal.h>

#include <atomic>
#include <condition_variable>
#include <mutex>

#include "pplx/pplxinterface.h"

namespace pplx
{
namespace details
{
namespace platform
{
/// <summary>
/// Returns a unique identifier for the execution thread where this routine in invoked
/// </summary>
_PPLXIMP long _pplx_cdecl GetCurrentThreadId();

/// <summary>
/// Yields the execution of the current execution thread - typically when spin-waiting
/// </summary>
_PPLXIMP void _pplx_cdecl YieldExecution();

/// <summary>
/// Captures the callstack
/// </summary>
__declspec(noinline) inline static size_t CaptureCallstack(void**, size_t, size_t) { return 0; }
} // namespace platform

/// <summary>
/// Manual reset event
/// </summary>
class event_impl
{
private:
    std::mutex _lock;
    std::condition_variable _condition;
    bool _signaled;

public:
    static const unsigned int timeout_infinite = 0xFFFFFFFF;

    event_impl() : _signaled(false) {}

    void set()
    {
        std::lock_guard<std::mutex> lock(_lock);
        _signaled = true;
        _condition.notify_all();
    }

    void reset()
    {
        std::lock_guard<std::mutex> lock(_lock);
        _signaled = false;
    }

    unsigned int wait(unsigned int timeout)
    {
        std::unique_lock<std::mutex> lock(_lock);
        if (timeout == event_impl::timeout_infinite)
        {
            _condition.wait(lock, [this]() -> bool { return _signaled; });
            return 0;
        }
        else
        {
            std::chrono::milliseconds period(timeout);
            auto status = _condition.wait_for(lock, period, [this]() -> bool { return _signaled; });
            _ASSERTE(status == _signaled);
            // Return 0 if the wait completed as a result of signaling the event. Otherwise, return timeout_infinite
            // Note: this must be consistent with the behavior of the Windows version, which is based on
            // WaitForSingleObjectEx
            return status ? 0 : event_impl::timeout_infinite;
        }
    }

    unsigned int wait() { return wait(event_impl::timeout_infinite); }
};

/// <summary>
/// Reader writer lock
/// </summary>
class reader_writer_lock_impl
{
private:
    pthread_rwlock_t _M_reader_writer_lock;

public:
    class scoped_lock_read
    {
    public:
        explicit scoped_lock_read(reader_writer_lock_impl& _Reader_writer_lock)
            : _M_reader_writer_lock(_Reader_writer_lock)
        {
            _M_reader_writer_lock.lock_read();
        }

        ~scoped_lock_read() { _M_reader_writer_lock.unlock(); }

    private:
        reader_writer_lock_impl& _M_reader_writer_lock;
        scoped_lock_read(const scoped_lock_read&);                  // no copy constructor
        scoped_lock_read const& operator=(const scoped_lock_read&); // no assignment operator
    };

    reader_writer_lock_impl() { pthread_rwlock_init(&_M_reader_writer_lock, nullptr); }

    ~reader_writer_lock_impl() { pthread_rwlock_destroy(&_M_reader_writer_lock); }

    void lock() { pthread_rwlock_wrlock(&_M_reader_writer_lock); }

    void lock_read() { pthread_rwlock_rdlock(&_M_reader_writer_lock); }

    void unlock() { pthread_rwlock_unlock(&_M_reader_writer_lock); }
};

/// <summary>
/// Recursive mutex
/// </summary>
class recursive_lock_impl
{
public:
    recursive_lock_impl() : _M_owner(-1), _M_recursionCount(0) {}

    ~recursive_lock_impl()
    {
        _ASSERTE(_M_owner == -1);
        _ASSERTE(_M_recursionCount == 0);
    }

    void lock()
    {
        auto id = ::pplx::details::platform::GetCurrentThreadId();

        if (_M_owner == id)
        {
            _M_recursionCount++;
        }
        else
        {
            _M_cs.lock();
            _M_owner = id;
            _M_recursionCount = 1;
        }
    }

    void unlock()
    {
        _ASSERTE(_M_owner == ::pplx::details::platform::GetCurrentThreadId());
        _ASSERTE(_M_recursionCount >= 1);

        _M_recursionCount--;

        if (_M_recursionCount == 0)
        {
            _M_owner = -1;
            _M_cs.unlock();
        }
    }

private:
    std::mutex _M_cs;
    std::atomic<long> _M_owner;
    long _M_recursionCount;
};

#if defined(__APPLE__)
class apple_scheduler : public pplx::scheduler_interface
#else
class linux_scheduler : public pplx::scheduler_interface
#endif
{
public:
    _PPLXIMP virtual void schedule(TaskProc_t proc, _In_ void* param);
#if defined(__APPLE__)
    virtual ~apple_scheduler() {}
#else
    virtual ~linux_scheduler() {}
#endif
};

} // namespace details

/// <summary>
///  A generic RAII wrapper for locks that implements the critical_section interface
///  std::lock_guard
/// </summary>
template<class _Lock>
class scoped_lock
{
public:
    explicit scoped_lock(_Lock& _Critical_section) : _M_critical_section(_Critical_section)
    {
        _M_critical_section.lock();
    }

    ~scoped_lock() { _M_critical_section.unlock(); }

private:
    _Lock& _M_critical_section;

    scoped_lock(const scoped_lock&);                  // no copy constructor
    scoped_lock const& operator=(const scoped_lock&); // no assignment operator
};

// The extensibility namespace contains the type definitions that are used internally
namespace extensibility
{
typedef ::pplx::details::event_impl event_t;

typedef std::mutex critical_section_t;
typedef scoped_lock<critical_section_t> scoped_critical_section_t;

typedef ::pplx::details::reader_writer_lock_impl reader_writer_lock_t;
typedef scoped_lock<reader_writer_lock_t> scoped_rw_lock_t;
typedef ::pplx::extensibility::reader_writer_lock_t::scoped_lock_read scoped_read_lock_t;

typedef ::pplx::details::recursive_lock_impl recursive_lock_t;
typedef scoped_lock<recursive_lock_t> scoped_recursive_lock_t;
} // namespace extensibility

/// <summary>
/// Default scheduler type
/// </summary>
#if defined(__APPLE__)
typedef details::apple_scheduler default_scheduler_t;
#else
typedef details::linux_scheduler default_scheduler_t;
#endif

namespace details
{
/// <summary>
/// Terminate the process due to unhandled exception
/// </summary>
#ifndef _REPORT_PPLTASK_UNOBSERVED_EXCEPTION
#define _REPORT_PPLTASK_UNOBSERVED_EXCEPTION()                                                                         \
    do                                                                                                                 \
    {                                                                                                                  \
        raise(SIGTRAP);                                                                                                \
        std::terminate();                                                                                              \
    } while (false)
#endif //_REPORT_PPLTASK_UNOBSERVED_EXCEPTION
} // namespace details

// see: http://gcc.gnu.org/onlinedocs/gcc/Return-Address.html
// this is critical to inline
__attribute__((always_inline)) inline void* _ReturnAddress() { return __builtin_return_address(0); }

} // namespace pplx

#endif // !_WIN32
