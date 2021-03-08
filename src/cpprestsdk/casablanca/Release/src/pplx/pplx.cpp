/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Parallel Patterns Library implementation (common code across platforms)
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#if !defined(_WIN32) || CPPREST_FORCE_PPLX
#include "pplx/pplx.h"
#include <atomic>

namespace pplx
{
namespace details
{
/// <summary>
/// Spin lock to allow for locks to be used in global scope
/// </summary>
class _Spin_lock
{
public:
    _Spin_lock() : _M_lock() {}

    void lock()
    {
        while (_M_lock.test_and_set())
        {
            pplx::details::platform::YieldExecution();
        }
    }

    void unlock() { _M_lock.clear(); }

private:
    std::atomic_flag _M_lock;
};

typedef ::pplx::scoped_lock<_Spin_lock> _Scoped_spin_lock;
} // namespace details

static struct _pplx_g_sched_t
{
    typedef std::shared_ptr<pplx::scheduler_interface> sched_ptr;

    _pplx_g_sched_t() { m_state.store(post_ctor, std::memory_order_relaxed); }

    ~_pplx_g_sched_t() { m_state.store(post_dtor, std::memory_order_relaxed); }

    sched_ptr get_scheduler()
    {
        sched_ptr result;
        switch (m_state.load(std::memory_order_relaxed))
        {
            case post_ctor:
                // This is the 99.9% case.
                {
                    ::pplx::details::_Scoped_spin_lock lock(m_spinlock);
                    if (!m_scheduler)
                    {
                        m_scheduler = std::make_shared<::pplx::default_scheduler_t>();
                    }

                    result = m_scheduler;
                } // unlock

                break;
            default:
                // This case means the global m_scheduler is not available.
                // We spin off an individual scheduler instead.
                result = std::make_shared<::pplx::default_scheduler_t>();
                break;
        }

        return result;
    }

    void set_scheduler(sched_ptr scheduler)
    {
        const auto localState = m_state.load(std::memory_order_relaxed);
        if (localState == pre_ctor || localState == post_dtor)
        {
            throw invalid_operation("Scheduler cannot be initialized now");
        }

        ::pplx::details::_Scoped_spin_lock lock(m_spinlock);

        if (m_scheduler)
        {
            throw invalid_operation("Scheduler is already initialized");
        }

        m_scheduler = std::move(scheduler);
    }

    enum m_state_values
    {
        pre_ctor,
        post_ctor,
        post_dtor
    };

private:
    std::atomic<m_state_values> m_state;
    pplx::details::_Spin_lock m_spinlock;
    sched_ptr m_scheduler;
} _pplx_g_sched;

_PPLXIMP std::shared_ptr<pplx::scheduler_interface> _pplx_cdecl get_ambient_scheduler()
{
    return _pplx_g_sched.get_scheduler();
}

_PPLXIMP void _pplx_cdecl set_ambient_scheduler(std::shared_ptr<pplx::scheduler_interface> _Scheduler)
{
    _pplx_g_sched.set_scheduler(std::move(_Scheduler));
}

} // namespace pplx

#endif
