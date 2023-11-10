/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Windows specific implementation of PPL constructs
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if !defined(_WIN32) || CPPREST_FORCE_PPLX

#include "pplx/pplxwin.h"

// Disable false alarm code analysis warning
#pragma warning(disable : 26165 26110)
namespace pplx
{
namespace details
{
namespace platform
{
_PPLXIMP long __cdecl GetCurrentThreadId() { return (long)(::GetCurrentThreadId()); }

_PPLXIMP void __cdecl YieldExecution() { YieldProcessor(); }

_PPLXIMP size_t __cdecl CaptureCallstack(void** stackData, size_t skipFrames, size_t captureFrames)
{
    (stackData);
    (skipFrames);
    (captureFrames);

    size_t capturedFrames = 0;
    // RtlCaptureSTackBackTrace is not available in MSDK, so we only call it under Desktop or _DEBUG MSDK.
    //  For MSDK unsupported version, we will return zero frame number.
#if !defined(__cplusplus_winrt)
    capturedFrames = RtlCaptureStackBackTrace(
        static_cast<DWORD>(skipFrames + 1), static_cast<DWORD>(captureFrames), stackData, nullptr);
#endif // !__cplusplus_winrt
    return capturedFrames;
}

#if defined(__cplusplus_winrt)
volatile long s_asyncId = 0;

_PPLXIMP unsigned int __cdecl GetNextAsyncId() { return static_cast<unsigned int>(_InterlockedIncrement(&s_asyncId)); }

#endif // defined(__cplusplus_winrt)

void InitializeCriticalSection(LPCRITICAL_SECTION _cs)
{
#ifndef __cplusplus_winrt
    // InitializeCriticalSection can cause STATUS_NO_MEMORY see C28125
    __try
    {
        ::InitializeCriticalSection(_cs);
    }
    __except (GetExceptionCode() == STATUS_NO_MEMORY ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
    {
        throw ::std::bad_alloc();
    }
#else  // ^^^ __cplusplus_winrt ^^^ // vvv !__cplusplus_winrt vvv
    InitializeCriticalSectionEx(_cs, 0, 0);
#endif // __cplusplus_winrt
}

} // namespace platform

//
// Event implementation
//
_PPLXIMP event_impl::event_impl()
{
    static_assert(sizeof(HANDLE) <= sizeof(_M_impl), "HANDLE version mismatch");

#ifndef __cplusplus_winrt
    _M_impl = CreateEvent(NULL, true, false, NULL);
#else
    _M_impl = CreateEventEx(NULL, NULL, CREATE_EVENT_MANUAL_RESET, EVENT_ALL_ACCESS);
#endif // !__cplusplus_winrt

    if (_M_impl != NULL)
    {
        ResetEvent(static_cast<HANDLE>(_M_impl));
    }
}

_PPLXIMP event_impl::~event_impl() { CloseHandle(static_cast<HANDLE>(_M_impl)); }

_PPLXIMP void event_impl::set() { SetEvent(static_cast<HANDLE>(_M_impl)); }

_PPLXIMP void event_impl::reset() { ResetEvent(static_cast<HANDLE>(_M_impl)); }

_PPLXIMP unsigned int event_impl::wait(unsigned int timeout)
{
    DWORD waitTime = (timeout == event_impl::timeout_infinite) ? INFINITE : (DWORD)timeout;
    DWORD status = WaitForSingleObjectEx(static_cast<HANDLE>(_M_impl), waitTime, 0);
    _ASSERTE((status == WAIT_OBJECT_0) || (waitTime != INFINITE));

    return (status == WAIT_OBJECT_0) ? 0 : event_impl::timeout_infinite;
}

//
// critical_section implementation
//
// TFS# 612702 -- this implementation is unnecessarily recursive. See bug for details.
_PPLXIMP critical_section_impl::critical_section_impl()
{
    static_assert(sizeof(CRITICAL_SECTION) <= sizeof(_M_impl), "CRITICAL_SECTION version mismatch");

    platform::InitializeCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&_M_impl));
}

_PPLXIMP critical_section_impl::~critical_section_impl()
{
    DeleteCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&_M_impl));
}

_PPLXIMP void critical_section_impl::lock() { EnterCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&_M_impl)); }

_PPLXIMP void critical_section_impl::unlock() { LeaveCriticalSection(reinterpret_cast<LPCRITICAL_SECTION>(&_M_impl)); }

#if _WIN32_WINNT >= _WIN32_WINNT_VISTA
//
// reader_writer_lock implementation
//
_PPLXIMP reader_writer_lock_impl::reader_writer_lock_impl() : m_locked_exclusive(false)
{
    static_assert(sizeof(SRWLOCK) <= sizeof(_M_impl), "SRWLOCK version mismatch");
    InitializeSRWLock(reinterpret_cast<PSRWLOCK>(&_M_impl));
}

_PPLXIMP void reader_writer_lock_impl::lock()
{
    AcquireSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&_M_impl));
    m_locked_exclusive = true;
}

_PPLXIMP void reader_writer_lock_impl::lock_read() { AcquireSRWLockShared(reinterpret_cast<PSRWLOCK>(&_M_impl)); }

_PPLXIMP void reader_writer_lock_impl::unlock()
{
    if (m_locked_exclusive)
    {
        m_locked_exclusive = false;
        ReleaseSRWLockExclusive(reinterpret_cast<PSRWLOCK>(&_M_impl));
    }
    else
    {
        ReleaseSRWLockShared(reinterpret_cast<PSRWLOCK>(&_M_impl));
    }
}
#endif // _WIN32_WINNT >= _WIN32_WINNT_VISTA

//
// scheduler implementation
//
#if defined(__cplusplus_winrt)

_PPLXIMP void windows_scheduler::schedule(TaskProc_t proc, _In_ void* param)
{
    auto workItemHandler = ref new Windows::System::Threading::WorkItemHandler(
        [proc, param](Windows::Foundation::IAsyncAction ^) { proc(param); });

    Windows::System::Threading::ThreadPool::RunAsync(workItemHandler);
}
#else // ^^^ __cplusplus_winrt ^^^ // vvv !__cplusplus_winrt vvv

#if _WIN32_WINNT < _WIN32_WINNT_VISTA
struct _Scheduler_Param
{
    TaskProc_t m_proc;
    void* m_param;

    _Scheduler_Param(TaskProc_t proc, _In_ void* param) : m_proc(proc), m_param(param) {}

    static DWORD CALLBACK DefaultWorkCallback(LPVOID lpParameter)
    {
        auto schedulerParam = (_Scheduler_Param*)(lpParameter);

        schedulerParam->m_proc(schedulerParam->m_param);

        delete schedulerParam;

        return 1;
    }
};

_PPLXIMP void windows_scheduler::schedule(TaskProc_t proc, _In_ void* param)
{
    auto schedulerParam = new _Scheduler_Param(proc, param);
    auto work = QueueUserWorkItem(_Scheduler_Param::DefaultWorkCallback, schedulerParam, WT_EXECUTELONGFUNCTION);

    if (!work)
    {
        delete schedulerParam;
        throw utility::details::create_system_error(GetLastError());
    }
}
#else  // ^^^ _WIN32_WINNT < _WIN32_WINNT_VISTA ^^^ // vvv _WIN32_WINNT >= _WIN32_WINNT_VISTA vvv
struct _Scheduler_Param
{
    TaskProc_t m_proc;
    void* m_param;

    _Scheduler_Param(TaskProc_t proc, _In_ void* param) : m_proc(proc), m_param(param) {}

    static void CALLBACK DefaultWorkCallback(PTP_CALLBACK_INSTANCE, PVOID pContext, PTP_WORK)
    {
        auto schedulerParam = (_Scheduler_Param*)(pContext);

        schedulerParam->m_proc(schedulerParam->m_param);

        delete schedulerParam;
    }
};

_PPLXIMP void windows_scheduler::schedule(TaskProc_t proc, _In_ void* param)
{
    auto schedulerParam = new _Scheduler_Param(proc, param);
    auto work = CreateThreadpoolWork(_Scheduler_Param::DefaultWorkCallback, schedulerParam, NULL);

    if (work == nullptr)
    {
        delete schedulerParam;
        throw utility::details::create_system_error(GetLastError());
    }

    SubmitThreadpoolWork(work);
    CloseThreadpoolWork(work);
}
#endif // _WIN32_WINNT < _WIN32_WINNT_VISTA

#endif // __cplusplus_winrt
} // namespace details

} // namespace pplx

#else // ^^^ !defined(_WIN32) || CPPREST_FORCE_PPLX ^^^ // vvv defined(_WIN32) && !CPPREST_FORCE_PPLX vvv
namespace Concurrency
{
void __cdecl set_cpprestsdk_ambient_scheduler(const std::shared_ptr<scheduler_interface>& _Scheduler)
{
    pplx::set_ambient_scheduler(_Scheduler);
}

const std::shared_ptr<scheduler_interface>& __cdecl get_cpprestsdk_ambient_scheduler()
{
    const auto& tmp = pplx::get_ambient_scheduler(); // putting this in a temporary reference variable to workaround
                                                     // VS2013 compiler bugs
    return tmp;
}

} // namespace Concurrency
#endif
