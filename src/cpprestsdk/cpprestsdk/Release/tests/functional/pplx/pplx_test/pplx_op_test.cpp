/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests for PPLX operations.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

pplx::details::atomic_long s_flag;

#if defined(_MSC_VER)

class pplx_dflt_scheduler : public pplx::scheduler_interface
{
    struct _Scheduler_Param
    {
        pplx::TaskProc_t m_proc;
        void* m_param;

        _Scheduler_Param(pplx::TaskProc_t proc, void* param) : m_proc(proc), m_param(param) {}
    };

    static void CALLBACK DefaultWorkCallbackTest(PTP_CALLBACK_INSTANCE, PVOID pContext, PTP_WORK)
    {
        auto schedulerParam = std::unique_ptr<_Scheduler_Param>(static_cast<_Scheduler_Param*>(pContext));

        schedulerParam->m_proc(schedulerParam->m_param);
    }

    virtual void schedule(pplx::TaskProc_t proc, void* param)
    {
        pplx::details::atomic_increment(s_flag);
        auto schedulerParam = std::unique_ptr<_Scheduler_Param>(new _Scheduler_Param(proc, param));
        auto work = CreateThreadpoolWork(DefaultWorkCallbackTest, schedulerParam.get(), NULL);

        if (work == nullptr)
        {
            throw utility::details::create_system_error(GetLastError());
        }

        SubmitThreadpoolWork(work);
        CloseThreadpoolWork(work);
        schedulerParam.release();
    }
};

#else
class pplx_dflt_scheduler : public pplx::scheduler_interface
{
    std::unique_ptr<crossplat::threadpool> m_pool;

    virtual void schedule(pplx::TaskProc_t proc, void* param)
    {
        pplx::details::atomic_increment(s_flag);
        m_pool->service().post([=]() -> void { proc(param); });
    }

public:
    pplx_dflt_scheduler() : m_pool(crossplat::threadpool::construct(4)) {}
};
#endif

namespace tests
{
namespace functional
{
namespace pplx_tests
{
SUITE(pplx_op_tests)
{
    TEST(task_from_value)
    {
        auto val = pplx::task_from_result<int>(17);

        VERIFY_ARE_EQUAL(val.get(), 17);
    }

    TEST(task_from_value_with_continuation)
    {
        auto val = pplx::task_from_result<int>(17);

        int v = 0;

        auto t = val.then([&](int x) { v = x; });
        t.wait();

        VERIFY_ARE_EQUAL(v, 17);
    }

    TEST(create_task)
    {
        auto val = pplx::create_task([]() { return 17; });

        VERIFY_ARE_EQUAL(val.get(), 17);
    }

    TEST(create_task_with_continuation)
    {
        auto val = pplx::create_task([]() { return 17; });

        int v = 0;

        auto t = val.then([&](int x) { v = x; });
        t.wait();

        VERIFY_ARE_EQUAL(v, 17);
    }

    TEST(task_from_event)
    {
        pplx::task_completion_event<int> tce;
        auto val = pplx::create_task(tce);
        tce.set(17);

        VERIFY_ARE_EQUAL(val.get(), 17);
    }

    TEST(task_from_event_with_continuation)
    {
        pplx::task_completion_event<int> tce;
        auto val = pplx::create_task(tce);

        int v = 0;

        auto t = val.then([&](int x) { v = x; });

        tce.set(17);
        t.wait();

        VERIFY_ARE_EQUAL(v, 17);
    }

    TEST(task_from_event_is_done)
    {
        pplx::task_completion_event<long> tce;
        auto val = pplx::create_task(tce);

        pplx::details::atomic_long v(0);

        auto t = val.then([&](long x) { pplx::details::atomic_exchange(v, x); });

        // The task should not have started yet.
        bool isDone = t.is_done();
        VERIFY_ARE_EQUAL(isDone, false);

        // Start the task
        tce.set(17);

        // Wait for the lambda to finish running
        while (!t.is_done())
        {
            // Yield.
        }

        // Verify that the lambda did run
        VERIFY_ARE_EQUAL(v, 17);

        // Wait for the task.
        t.wait();

        VERIFY_ARE_EQUAL(v, 17);
    }

    TEST(task_from_event_with_exception)
    {
        pplx::task_completion_event<long> tce;
        auto val = pplx::create_task(tce);

        pplx::details::atomic_long v(0);

        auto t = val.then([&](long x) { pplx::details::atomic_exchange(v, x); });

        // Start the task
        tce.set_exception(pplx::invalid_operation());

        // Wait for the lambda to finish running
        while (!t.is_done())
        {
            // Yield.
        }

        // Verify that the lambda did run
        VERIFY_ARE_EQUAL(v, 0);

        // Wait for the task.
        try
        {
            t.wait();
        }
        catch (pplx::invalid_operation)
        {
        }
        catch (std::exception_ptr)
        {
            v = 1;
        }

        VERIFY_ARE_EQUAL(v, 0);
    }

    TEST(schedule_task_hold_then_release)
    {
        pplx::details::atomic_long flag(0);

        pplx::task<void> t1([&flag]() {
            while (flag == 0)
                ;
        });

        pplx::details::atomic_exchange(flag, 1L);
        t1.wait();
    }

    // TFS # 521911
    TEST(schedule_two_tasks)
    {
        pplx_dflt_scheduler sched;
        pplx::details::atomic_exchange(s_flag, 0L);

        auto nowork = []() {};

        auto defaultTask = pplx::create_task(nowork);
        defaultTask.wait();
        VERIFY_ARE_EQUAL(s_flag, 0);

        pplx::task_completion_event<void> tce;
        auto t = pplx::create_task(tce, sched);

        // 2 continuations to be scheduled on the scheduler.
        // Note that task "t" is not scheduled.
        auto t1 = t.then(nowork).then(nowork);

        tce.set();
        t1.wait();

        VERIFY_ARE_EQUAL(s_flag, 2);
    }

    TEST(task_throws_exception)
    {
        pplx::extensibility::event_t ev;
        bool caught = false;

        // Ensure that exceptions thrown from user lambda
        // are indeed propagated and do not escape out of
        // the task.
        auto t1 = pplx::create_task([&ev]() {
            ev.set();
            throw std::logic_error("Should not escape");
        });

        auto t2 = t1.then([]() { VERIFY_IS_TRUE(false); });

        // Ensure that we do not inline the work on this thread
        ev.wait();

        try
        {
            t2.wait();
        }
        catch (std::exception&)
        {
            caught = true;
        }

        VERIFY_IS_TRUE(caught);
    }

    pplx::task<int> make_unwrapped_task()
    {
        pplx::task<int> t1([]() { return 10; });

        return pplx::task<int>([t1]() { return t1; });
    }

    TEST(unwrap_task)
    {
        pplx::task<int> t = make_unwrapped_task();
        int n = t.get();
        VERIFY_ARE_EQUAL(n, 10);
    }

    TEST(task_from_event_with_tb_continuation)
    {
        volatile long flag = 0;

        pplx::task_completion_event<int> tce;
        auto val = pplx::create_task(tce).then([=, &flag](pplx::task<int> op) -> short {
            flag = 1;
            return (short)op.get();
        });

        tce.set(17);

        VERIFY_ARE_EQUAL(val.get(), 17);
        VERIFY_ARE_EQUAL(flag, 1);
    }

    class fcc_370010
    {
    public:
        fcc_370010(pplx::task_completion_event<bool> op) : m_op(op) {}

        virtual void on_closed(bool result)
        {
            m_op.set(result);
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdelete-non-virtual-dtor"
#endif
            delete this;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
        }

    private:
        pplx::task_completion_event<bool> m_op;
    };

    TEST(BugRepro370010)
    {
        auto result_tce = pplx::task_completion_event<bool>();

        auto f = new fcc_370010(result_tce);

        pplx::task<void> dummy([f]() { f->on_closed(true); });

        auto result = pplx::task<bool>(result_tce);

        VERIFY_IS_TRUE(result.get());
    }

    TEST(event_set_reset_timeout, "Ignore", "785846")
    {
        pplx::extensibility::event_t ev;

        ev.set();

        // Wait should succeed as the event was set above
        VERIFY_IS_TRUE(ev.wait(0) == 0);

        // wait should succeed as this is manual reset
        VERIFY_IS_TRUE(ev.wait(0) == 0);

        ev.reset();

        // wait should fail as the event is reset (not set)
        VERIFY_IS_TRUE(ev.wait(0) == pplx::extensibility::event_t::timeout_infinite);
    }

} // SUITE(pplx_op_tests)

} // namespace pplx_tests
} // namespace functional
} // namespace tests
