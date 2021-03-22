/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * PPL interfaces
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#ifndef _PPLXINTERFACE_H
#define _PPLXINTERFACE_H

#if (defined(_MSC_VER) && (_MSC_VER >= 1800)) && !CPPREST_FORCE_PPLX
#error This file must not be included for Visual Studio 12 or later
#endif

#if defined(_CRTBLD)
#elif defined(_WIN32)
#if (_MSC_VER >= 1700)
#define _USE_REAL_ATOMICS
#endif
#else // GCC compiler
#define _USE_REAL_ATOMICS
#endif

#include <memory>
#ifdef _USE_REAL_ATOMICS
#include <atomic>
#endif

#define _pplx_cdecl __cdecl

namespace pplx
{
/// <summary>
///     An elementary abstraction for a task, defined as <c>void (__cdecl * TaskProc_t)(void *)</c>. A <c>TaskProc</c>
///     is called to invoke the body of a task.
/// </summary>
/**/
typedef void(_pplx_cdecl* TaskProc_t)(void*);

/// <summary>
///     Scheduler Interface
/// </summary>
struct __declspec(novtable) scheduler_interface
{
    virtual void schedule(TaskProc_t, _In_ void*) = 0;
};

/// <summary>
///     Represents a pointer to a scheduler. This class exists to allow the
///     the specification of a shared lifetime by using shared_ptr or just
///     a plain reference by using raw pointer.
/// </summary>
struct scheduler_ptr
{
    /// <summary>
    /// Creates a scheduler pointer from shared_ptr to scheduler
    /// </summary>
    explicit scheduler_ptr(std::shared_ptr<scheduler_interface> scheduler) : m_sharedScheduler(std::move(scheduler))
    {
        m_scheduler = m_sharedScheduler.get();
    }

    /// <summary>
    /// Creates a scheduler pointer from raw pointer to scheduler
    /// </summary>
    explicit scheduler_ptr(_In_opt_ scheduler_interface* pScheduler) : m_scheduler(pScheduler) {}

    /// <summary>
    /// Behave like a pointer
    /// </summary>
    scheduler_interface* operator->() const { return get(); }

    /// <summary>
    ///  Returns the raw pointer to the scheduler
    /// </summary>
    scheduler_interface* get() const { return m_scheduler; }

    /// <summary>
    /// Test whether the scheduler pointer is non-null
    /// </summary>
    operator bool() const { return get() != nullptr; }

private:
    std::shared_ptr<scheduler_interface> m_sharedScheduler;
    scheduler_interface* m_scheduler;
};

/// <summary>
///     Describes the execution status of a <c>task_group</c> or <c>structured_task_group</c> object.  A value of this
///     type is returned by numerous methods that wait on tasks scheduled to a task group to complete.
/// </summary>
/// <seealso cref="task_group Class"/>
/// <seealso cref="task_group::wait Method"/>
/// <seealso cref="task_group::run_and_wait Method"/>
/// <seealso cref="structured_task_group Class"/>
/// <seealso cref="structured_task_group::wait Method"/>
/// <seealso cref="structured_task_group::run_and_wait Method"/>
/**/
enum task_group_status
{
    /// <summary>
    ///     The tasks queued to the <c>task_group</c> object have not completed.  Note that this value is not presently
    ///     returned by the Concurrency Runtime.
    /// </summary>
    /**/
    not_complete,

    /// <summary>
    ///     The tasks queued to the <c>task_group</c> or <c>structured_task_group</c> object completed successfully.
    /// </summary>
    /**/
    completed,

    /// <summary>
    ///     The <c>task_group</c> or <c>structured_task_group</c> object was canceled.  One or more tasks may not have
    ///     executed.
    /// </summary>
    /**/
    canceled
};

namespace details
{
/// <summary>
///     Atomics
/// </summary>
#ifdef _USE_REAL_ATOMICS
typedef std::atomic<long> atomic_long;
typedef std::atomic<size_t> atomic_size_t;

template<typename _T>
_T atomic_compare_exchange(std::atomic<_T>& _Target, _T _Exchange, _T _Comparand)
{
    _T _Result = _Comparand;
    _Target.compare_exchange_strong(_Result, _Exchange);
    return _Result;
}

template<typename _T>
_T atomic_exchange(std::atomic<_T>& _Target, _T _Value)
{
    return _Target.exchange(_Value);
}

template<typename _T>
_T atomic_increment(std::atomic<_T>& _Target)
{
    return _Target.fetch_add(1) + 1;
}

template<typename _T>
_T atomic_decrement(std::atomic<_T>& _Target)
{
    return _Target.fetch_sub(1) - 1;
}

template<typename _T>
_T atomic_add(std::atomic<_T>& _Target, _T value)
{
    return _Target.fetch_add(value) + value;
}

#else // not _USE_REAL_ATOMICS

typedef long volatile atomic_long;
typedef size_t volatile atomic_size_t;

template<class T>
inline T atomic_exchange(T volatile& _Target, T _Value)
{
    return _InterlockedExchange(&_Target, _Value);
}

inline long atomic_increment(long volatile& _Target) { return _InterlockedIncrement(&_Target); }

inline long atomic_add(long volatile& _Target, long value) { return _InterlockedExchangeAdd(&_Target, value) + value; }

inline size_t atomic_increment(size_t volatile& _Target)
{
#if (defined(_M_IX86) || defined(_M_ARM))
    return static_cast<size_t>(_InterlockedIncrement(reinterpret_cast<long volatile*>(&_Target)));
#else
    return static_cast<size_t>(_InterlockedIncrement64(reinterpret_cast<__int64 volatile*>(&_Target)));
#endif
}

inline long atomic_decrement(long volatile& _Target) { return _InterlockedDecrement(&_Target); }

inline size_t atomic_decrement(size_t volatile& _Target)
{
#if (defined(_M_IX86) || defined(_M_ARM))
    return static_cast<size_t>(_InterlockedDecrement(reinterpret_cast<long volatile*>(&_Target)));
#else
    return static_cast<size_t>(_InterlockedDecrement64(reinterpret_cast<__int64 volatile*>(&_Target)));
#endif
}

inline long atomic_compare_exchange(long volatile& _Target, long _Exchange, long _Comparand)
{
    return _InterlockedCompareExchange(&_Target, _Exchange, _Comparand);
}

inline size_t atomic_compare_exchange(size_t volatile& _Target, size_t _Exchange, size_t _Comparand)
{
#if (defined(_M_IX86) || defined(_M_ARM))
    return static_cast<size_t>(_InterlockedCompareExchange(
        reinterpret_cast<long volatile*>(_Target), static_cast<long>(_Exchange), static_cast<long>(_Comparand)));
#else
    return static_cast<size_t>(_InterlockedCompareExchange64(reinterpret_cast<__int64 volatile*>(_Target),
                                                             static_cast<__int64>(_Exchange),
                                                             static_cast<__int64>(_Comparand)));
#endif
}
#endif // _USE_REAL_ATOMICS

} // namespace details
} // namespace pplx

#endif // _PPLXINTERFACE_H
