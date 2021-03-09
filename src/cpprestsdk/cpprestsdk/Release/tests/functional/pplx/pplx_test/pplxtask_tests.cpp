/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Basic tests for PPLX operations
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/
#include "stdafx.h"

using namespace ::pplx;
using namespace ::tests::common::utilities;

namespace tests
{
namespace functional
{
namespace PPLX
{
static void IsTrue(bool condition, const wchar_t*, ...) { VERIFY_IS_TRUE(condition); }

static void IsFalse(bool condition, ...) { VERIFY_IS_TRUE(condition == false); }

static void LogFailure(const wchar_t* msg, ...)
{
    wprintf(L"%s", msg);
    VERIFY_IS_TRUE(false);
}

namespace helpers
{
static int FibSerial(int n)
{
    if (n < 2) return n;

    return FibSerial(n - 1) + FibSerial(n - 2);
}

static void DoRandomParallelWork()
{
    int param = (rand() % 8) + 20;
    // Calculate fib in serial
    volatile int val = FibSerial(param);
    val;
}

template<typename _EX, typename _T>
bool VerifyException(task<_T>& task)
{
    bool gotException = true;
    bool wrongException = false;

    try
    {
        task.get();
        gotException = false;
    }
    catch (const _EX&)
    {
    }
    catch (...)
    {
        wrongException = true;
    }

    return (gotException && !wrongException);
}

template<typename _T>
bool VerifyNoException(task<_T>& task)
{
    try
    {
        task.get();
    }
    catch (...)
    {
        return false;
    }
    return true;
}

template<typename _T>
bool VerifyCanceled(task<_T>& task)
{
    try
    {
        task.get();
    }
    catch (task_canceled&)
    {
        return true;
    }
    catch (...)
    {
        return false;
    }
    return false;
}

template<typename _T>
void ObserveException(task<_T>& task)
{
    try
    {
        task.get();
    }
    catch (...)
    {
    }
}

template<typename Iter>
void ObserveAllExceptions(Iter begin, Iter end)
{
    typedef typename std::iterator_traits<Iter>::value_type::result_type TaskType;
    for (auto it = begin; it != end; ++it)
    {
        ObserveException(*it);
    }
}
} // namespace helpers

SUITE(pplxtask_tests)
{
    TEST(TestCancellationTokenRegression)
    {
        for (int i = 0; i < 500; i++)
        {
            task_completion_event<void> tce;
            task<void> starter(tce);

            cancellation_token_source ct;

            task<int> t1 = starter.then([]() -> int { return 47; }, ct.get_token());

            task<int> t2([]() -> int { return 82; });

            task<int> t3([]() -> int { return 147; });

            auto t4 = (t1 && t2 && t3).then([=](std::vector<int> vec) -> int { return vec[0] + vec[1] + vec[3]; });

            ct.cancel();

            tce.set();
            // this should not hang
            task_status t4Status = t4.wait();
            IsTrue(t4Status == canceled,
                   L"operator && did not properly cancel. Expected: %d, Actual: %d",
                   canceled,
                   t4Status);
        }
    }
    TEST(TestTasks_basic)
    {
        {
            task<int> t1([]() -> int { return 47; });

            auto t2 = t1.then([=](int i) -> float {
                IsTrue(i == 47,
                       L"Continuation did not recieve the correct value from ancestor. Expected: 47, Actual: %d",
                       i);
                return (float)i / 2;
            });

            float t2Result = t2.get();
            IsTrue(t2Result == 23.5,
                   L"Continuation task did not produce the correct result. Expected: 23.5, Actual: %f",
                   t2Result);

            task_status t2Status = t2.wait();
            IsTrue(t2Status == completed,
                   L"Continuation task was not in completed state. Expected: %d, Actual: %d",
                   completed,
                   t2Status);

            task<int> t3([]() -> int { return 0; });

            IsTrue(t1 == t1, L"task operator== resulted false on equivalent tasks");
            IsFalse(t1 != t1, L"task operator!= resulted true on equivalent tasks");
            IsFalse(t1 == t3, L"task operator== resulted true on different tasks");
            IsTrue(t1 != t3, L"task operator!= resulted false on different tasks");

            t3.wait();
        }
    }

    TEST(TestTasks_default_construction)
    {
        // Test that default constructed task<T> properly throw exceptions
        {
            task<int> t1;

            try
            {
                t1.wait();
                LogFailure(L"t1.wait() should have thrown an exception");
            }
            catch (invalid_operation)
            {
            }

            try
            {
                t1.get();
                LogFailure(L"t1.get() should have thrown an exception");
            }
            catch (invalid_operation)
            {
            }

            try
            {
                t1.then([](int i) { return i; });

                LogFailure(L"t1.then() should have thrown an exception");
            }
            catch (invalid_operation)
            {
            }
        }
    }

    TEST(TestTasks_void_tasks)
    {
        // Test void tasks
        {
            int value = 0;
            task<void> t1([&value]() { value = 147; });

            auto t2 = t1.then([&]() {
                IsTrue(value == 147,
                       L"void continuation did not recieve the correct value from ancestor. Expected: 147, Actual: %d",
                       value);
                value++;
            });

            IsTrue(t2.wait() == completed, L"void task was not in completed state.");

            IsTrue(value == 148, L"void tasks did not properly execute. Expected: 148, Actual: %d", value);

            task<void> t3([]() {});

            IsTrue(t1 == t1, L"task operator== resulted false on equivalent tasks");
            IsFalse(t1 != t1, L"task operator!= resulted true on equivalent tasks");
            IsFalse(t1 == t3, L"task operator== resulted true on different tasks");
            IsTrue(t1 != t3, L"task operator!= resulted false on different tasks");
        }
    }

    TEST(TestTasks_void_tasks_default_construction)
    {
        // Test that default constructed task<void> properly throw exceptions
        {
            task<void> t1;

            try
            {
                t1.wait();
                LogFailure(L"t1.wait() should have thrown an exception");
            }
            catch (invalid_operation)
            {
            }

            try
            {
                t1.get();
                LogFailure(L"t1.get() should have thrown an exception");
            }
            catch (invalid_operation)
            {
            }

            try
            {
                t1.then([]() {});
                LogFailure(L"t1.contiue_with() should have thrown an exception");
            }
            catch (invalid_operation)
            {
            }
        }
    }

    TEST(TestTasks_movable_then)
    {
#ifndef _MSC_VER
        // create movable only type
        struct A
        {
            A() = default;
            A(A&&) = default;
            A& operator=(A&&) = default;

            // explicitly delete copy functions
            A(const A&) = delete;
            A& operator=(const A&) = delete;

            char operator()(int) { return 'c'; }
        } a;

        task<int> task = create_task([] { return 2; });
        auto f = task.then(std::move(a));

        IsTrue(f.get() == 'c', L".then should be able to work with movable functors");
#endif // _MSC_VER
    }

    TEST(TestTasks_constant_this)
    {
#ifdef _MSC_VER
#if _MSC_VER < 1700
        // Dev10 compiler gives an error => .then(func) where func = int!
#else
        {
            // Test constant 'this' pointer in member functions then(), wait() and get(),
            // so that they can be used in Lambda.
            task<int> t1([]() -> int { return 0; });

            auto func = [t1]() -> int {
                t1.then([](int last) -> int { return last; });
                t1.wait();
                return t1.get();
            };

            IsTrue(func() == 0, L"Tasks should be able to used inside a Lambda.");
        }
#endif // _MSC_VER < 1700
#endif // _MSC_VER
    }

    TEST(TestTasks_fire_and_forget)
    {
        // Test Fire-and-forget behavior
        extensibility::event_t evt;
        bool flag = false;
        {
            task<int> t1([&flag, &evt]() -> int {
                flag = true;
                evt.set();
                return 0;
            });
        }

        evt.wait();
        IsTrue(flag == true, L"Fire-and-forget task did not properly execute.");
    }
    TEST(TestTasks_create_task)
    {
        // test create task
        task<int> t1 = create_task([]() -> int { return 4; });
        IsTrue(t1.get() == 4, L"create_task for simple task did not properly execute.");
        IsTrue(create_task(t1).get() == 4, L"create_task from a task task did not properly execute.");
        task<void> t2 = create_task([]() {});
        task<int> t3 = create_task([]() -> task<int> { return create_task([]() -> int { return 4; }); });
        IsTrue(t3.get() == 4, L"create_task for task unwrapping did not properly execute.");
    }

    TEST(TestTaskCompletionEvents_basic)
    {
        task_completion_event<int> tce;
        task<int> completion(tce);
        auto completion2 = create_task(tce);

        task<void> setEvent([=]() { tce.set(50); });

        int result = completion.get();
        IsTrue(result == 50, L"Task Completion Event did not get the right result. Expected: 50, Actual: %d", result);
        IsTrue(completion2.get() == 50,
               L"create_task didn't construct correct task for task_completion_event, Expected: 50, Actual: %d",
               result);
    }

    TEST(TestTaskCompletionEvents_basic2)
    {
        task_completion_event<void> tce;
        task<void> completion(tce);
        auto completion2 = create_task(tce);

        task<void> setEvent([=]() { tce.set(); });

        // this should not hang, because of the set of tce
        completion.wait();
        completion2.wait();
    }

    TEST(TestTaskCompletionEvents_set_exception_basic)
    {
        task_completion_event<void> tce;
        task<void> t(tce);
        tce.set_exception(42);

        t.then([=](task<void> p) {
             try
             {
                 p.get();
                 IsTrue(false, L"Exception not propagated to task t when calling set_exception.");
             }
             catch (int n)
             {
                 IsTrue(n == 42, L"%ws:%u:bad exception value", __FILE__, __LINE__);
             }
         })
            .wait();
    }

    TEST(TestTaskCompletionEvents_set_exception_multiple)
    {
        task_completion_event<void> tce;
        task<void> t(tce);
        tce.set_exception(42);

        t.then([=](task<void> p) {
             try
             {
                 p.get();
                 IsTrue(false, L"Exception not propagated to task t's first continuation when calling set_exception.");
             }
             catch (int n)
             {
                 IsTrue(n == 42, L"%ws:%u:bad exception value", __FILE__, __LINE__);
             }
         })
            .wait();

        t.then([=](task<void> p) {
             try
             {
                 p.get();
                 IsTrue(false, L"Exception not propagated to task t's second continuation when calling set_exception.");
             }
             catch (int n)
             {
                 IsTrue(n == 42, L"%ws:%u:bad exception value", __FILE__, __LINE__);
             }
         })
            .wait();
    }

    TEST(TestTaskCompletionEvents_set_exception_struct)
    {
#if defined(_MSC_VER) && _MSC_VER < 1700
        // The Dev10 compiler hits an ICE with this code
#else
        struct s
        {
        };

        task_completion_event<void> tce;
        task<void> t(tce);
        tce.set_exception(s());
        t.then([=](task<void> p) {
             try
             {
                 p.get();
                 IsTrue(false, L"Exception not caught.");
             }
             catch (s)
             {
                 // Do nothing
             }
             catch (...)
             {
                 IsTrue(false, L"%ws:%u:not the right exception", __FILE__, __LINE__);
             }
         })
            .wait();
#endif // _MSC_VER < 1700
    }

    TEST(TestTaskCompletionEvents_multiple_tasks)
    {
        task_completion_event<void> tce;
        task<void> t1(tce);
        task<void> t2(tce);
        tce.set_exception(1);

        t1.then([=](task<void> p) {
            try
            {
                p.get();
                IsTrue(false, L"An exception was not thrown when calling t1.get().  An exception was expected.");
            }
            catch (int ex)
            {
                IsTrue(ex == 1, L"%ws:%u:wrong exception value", __FILE__, __LINE__);
            }
            catch (...)
            {
                IsTrue(false, L"%ws:%u:not the right exception", __FILE__, __LINE__);
            }
        });

        t2.then([=](task<void> p) {
            try
            {
                p.get();
                IsTrue(false, L"An exception was not thrown when calling t2.get().  An exception was expected.");
            }
            catch (int ex)
            {
                IsTrue(ex == 1, L"%ws:%u:wrong exception value", __FILE__, __LINE__);
            }
            catch (...)
            {
                IsTrue(false, L"%ws:%u:not the right exception", __FILE__, __LINE__);
            }
        });
    }

    TEST(TestTaskCompletionEvents_set_exception_after_set)
    {
        task_completion_event<int> tce;
        task<int> t(tce);
        tce.set(1);
        auto result = tce.set_exception(std::current_exception());
        IsFalse(result, L"set_exception must return false, but did not");
        t.then([=](task<int> p) {
             try
             {
                 int n = p.get();
                 IsTrue(n == 1, L"Value not properly propagated to continuation");
             }
             catch (...)
             {
                 IsTrue(false, L"An exception was unexpectedly thrown in the continuation task");
             }
         })
            .wait();
    }

    TEST(TestTaskCompletionEvents_set_exception_after_set2)
    {
        task_completion_event<int> tce;
        task<int> t(tce);
        tce.set_exception(1);
        auto result = tce.set_exception(2);
        IsFalse(result, L"set_exception must return false, but did not");
        t.then([=](task<int> p) {
             try
             {
                 p.get();
                 IsTrue(false, L"%ws:%u:expected exception not thrown", __FILE__, __LINE__);
             }
             catch (int n)
             {
                 IsTrue(n == 1, L"%ws:%u:unexpected exception payload", __FILE__, __LINE__);
             }
         })
            .wait();
    }

    TEST(TestTaskCompletionEvents_set_after_set_exception)
    {
        task_completion_event<int> tce;
        task<int> t(tce);
        tce.set_exception(42);
        tce.set(1); // should be no-op
        t.then([=](task<int> p) {
             try
             {
                 p.get();
                 IsTrue(false, L"Exception should have been thrown here.");
             }
             catch (int e)
             {
                 IsTrue(e == 42, L"%ws:%u:not the right exception value", __FILE__, __LINE__);
             }
             catch (...)
             {
                 IsTrue(false, L"%ws:%u:not the right exception", __FILE__, __LINE__);
             }
         })
            .wait();
    }

    TEST(TestTaskOperators_and_or)
    {
        task<int> t1([]() -> int { return 47; });

        task<int> t2([]() -> int { return 82; });

        auto t3 = (t1 && t2).then([=](std::vector<int> vec) -> int {
            IsTrue(vec.size() == 2,
                   L"operator&& did not produce a correct vector size. Expected: 2, Actual: %d",
                   vec.size());
            IsTrue(vec[0] == 47, L"operator&& did not produce a correct vector[0]. Expected: 47, Actual: %d", vec[0]);
            IsTrue(vec[1] == 82, L"operator&& did not produce a correct vector[1]. Expected: 82, Actual: %d", vec[1]);
            return vec[0] + vec[1];
        });

        int t3Result = t3.get();
        IsTrue(t3Result == 129,
               L"operator&& task did not produce the correct result. Expected: 129, Actual: %d",
               t3Result);
    }

    TEST(TestTaskOperators_and_or2)
    {
        task<int> t1([]() -> int { return 47; });

        task<int> t2([]() -> int { return 82; });

        task<int> t3([]() -> int { return 147; });

        task<int> t4([]() -> int { return 192; });

        auto t5 = (t1 && t2 && t3 && t4).then([=](std::vector<int> vec) -> int {
            IsTrue(vec.size() == 4,
                   L"operator&& did not produce a correct vector size. Expected: 4, Actual: %d",
                   vec.size());
            IsTrue(vec[0] == 47, L"operator&& did not produce a correct vector[0]. Expected: 47, Actual: %d", vec[0]);
            IsTrue(vec[1] == 82, L"operator&& did not produce a correct vector[1]. Expected: 82, Actual: %d", vec[1]);
            IsTrue(vec[2] == 147, L"operator&& did not produce a correct vector[2]. Expected: 147, Actual: %d", vec[2]);
            IsTrue(vec[3] == 192, L"operator&& did not produce a correct vector[3]. Expected: 192, Actual: %d", vec[3]);
            int count = 0;
            for (unsigned i = 0; i < vec.size(); i++)
                count += vec[i];
            return count;
        });

        int t5Result = t5.get();
        IsTrue(t5Result == 468,
               L"operator&& task did not produce the correct result. Expected: 468, Actual: %d",
               t5Result);
    }

    TEST(TestTaskOperators_and_or3)
    {
        task<int> t1([]() -> int { return 47; });

        task<int> t2([]() -> int { return 82; });

        task<int> t3([]() -> int { return 147; });

        task<int> t4([]() -> int { return 192; });

        auto t5 = ((t1 && t2) && (t3 && t4)).then([=](std::vector<int> vec) -> int {
            IsTrue(vec.size() == 4,
                   L"operator&& did not produce a correct vector size. Expected: 4, Actual: %d",
                   vec.size());
            IsTrue(vec[0] == 47, L"operator&& did not produce a correct vector[0]. Expected: 47, Actual: %d", vec[0]);
            IsTrue(vec[1] == 82, L"operator&& did not produce a correct vector[1]. Expected: 82, Actual: %d", vec[1]);
            IsTrue(vec[2] == 147, L"operator&& did not produce a correct vector[2]. Expected: 147, Actual: %d", vec[2]);
            IsTrue(vec[3] == 192, L"operator&& did not produce a correct vector[3]. Expected: 192, Actual: %d", vec[3]);
            int count = 0;
            for (unsigned i = 0; i < vec.size(); i++)
                count += vec[i];
            return count;
        });

        int t5Result = t5.get();
        IsTrue(t5Result == 468,
               L"operator&& task did not produce the correct result. Expected: 468, Actual: %d",
               t5Result);
    }

    TEST(TestTaskOperators_and_or4)
    {
        extensibility::event_t evt;

        task<int> t1([&evt]() -> int {
            evt.wait();
            return 47;
        });

        task<int> t2([]() -> int { return 82; });

        auto t3 = (t1 || t2).then([=](int p) -> int {
            IsTrue(p == 82, L"operator|| did not get the right result. Expected: 82, Actual: %d", p);
            return p;
        });

        t3.wait();

        evt.set();
        t1.wait();
    }

    TEST(TestTaskOperators_and_or5)
    {
        extensibility::event_t evt;

        task<int> t1([&evt]() -> int {
            evt.wait();
            return 47;
        });

        task<int> t2([&evt]() -> int {
            evt.wait();
            return 82;
        });

        task<int> t3([]() -> int { return 147; });

        task<int> t4([&evt]() -> int {
            evt.wait();
            return 192;
        });

        auto t5 = (t1 || t2 || t3 || t4).then([=](int result) -> int {
            IsTrue(result == 147, L"operator|| did not produce a correct result. Expected: 147, Actual: %d", result);
            return result;
        });

        t5.wait();

        evt.set();
        t1.wait();
        t2.wait();
        t4.wait();
    }

    TEST(TestTaskOperators_and_or_sequence)
    {
        // testing ( t1 && t2 ) || t3, operator&& finishes first
        extensibility::event_t evt;

        task<int> t1([]() -> int { return 47; });

        task<int> t2([]() -> int { return 82; });

        task<int> t3([&evt]() -> int {
            evt.wait();
            return 147;
        });

        auto t4 = ((t1 && t2) || t3).then([=](std::vector<int> vec) -> int {
            IsTrue(vec.size() == 2,
                   L"(t1 && t2) || t3 did not produce a correct vector size. Expected: 2, Actual: %d",
                   vec.size());
            IsTrue(vec[0] == 47,
                   L"(t1 && t2) || t3 did not produce a correct vector[0]. Expected: 47, Actual: %d",
                   vec[0]);
            IsTrue(vec[1] == 82,
                   L"(t1 && t2) || t3 did not produce a correct vector[1]. Expected: 82, Actual: %d",
                   vec[1]);
            return vec[0] + vec[1];
        });

        int t4Result = t4.get();
        IsTrue(t4.get() == 129,
               L"(t1 && t2) || t3 task did not produce the correct result. Expected: 129, Actual: %d",
               t4Result);

        evt.set();
        t3.wait();
    }

    TEST(TestTaskOperators_and_or_sequence2)
    {
        // testing ( t1 && t2 ) || t3, operator|| finishes first
        extensibility::event_t evt;

        task<int> t1([&evt]() -> int {
            evt.wait();
            return 47;
        });

        task<int> t2([&evt]() -> int {
            evt.wait();
            return 82;
        });

        task<int> t3([]() -> int { return 147; });

        auto t4 = ((t1 && t2) || t3).then([=](std::vector<int> vec) -> int {
            IsTrue(vec.size() == 1,
                   L"(t1 && t2) || t3 did not produce a correct vector size. Expected: 1, Actual: %d",
                   vec.size());
            IsTrue(vec[0] == 147,
                   L"(t1 && t2) || t3 did not produce a correct vector[0]. Expected: 147, Actual: %d",
                   vec[0]);
            return vec[0];
        });

        int t4Result = t4.get();
        IsTrue(t4.get() == 147,
               L"(t1 && t2) || t3 task did not produce the correct result. Expected: 147, Actual: %d",
               t4Result);

        evt.set();
        t1.wait();
        t2.wait();
    }

    TEST(TestTaskOperators_and_or_sequence3)
    {
        // testing t1 && (t2 || t3)
        extensibility::event_t evt;

        task<int> t1([]() -> int { return 47; });

        task<int> t2([&evt]() -> int {
            evt.wait();
            return 82;
        });

        task<int> t3([]() -> int { return 147; });

        auto t4 = (t1 && (t2 || t3)).then([=](std::vector<int> vec) -> int {
            IsTrue(vec.size() == 2,
                   L"t1 && (t2 || t3) did not produce a correct vector size. Expected: 2, Actual: %d",
                   vec.size());
            IsTrue(vec[0] == 47,
                   L"t1 && (t2 || t3) did not produce a correct vector[0]. Expected: 47, Actual: %d",
                   vec[0]);
            IsTrue(vec[1] == 147,
                   L"t1 && (t2 || t3) did not produce a correct vector[1]. Expected: 147, Actual: %d",
                   vec[1]);
            return vec[0] + vec[1];
        });

        int t4Result = t4.get();
        IsTrue(t4.get() == 194,
               L"t1 && (t2 || t3) task did not produce the correct result. Expected: 194 Actual: %d",
               t4Result);

        evt.set();
        t2.wait();
    }

    TEST(TestTaskOperators_cancellation)
    {
        task_completion_event<void> tce;
        task<void> starter(tce);

        cancellation_token_source ct;

        task<int> t1 = starter.then([]() -> int { return 47; }, ct.get_token());

        task<int> t2([]() -> int { return 82; });

        task<int> t3([]() -> int { return 147; });

        auto t4 = (t1 && t2 && t3).then([=](std::vector<int> vec) -> int { return vec[0] + vec[1] + vec[3]; });

        ct.cancel();

        tce.set();
        // this should not hang
        task_status t4Status = t4.wait();
        IsTrue(
            t4Status == canceled, L"operator && did not properly cancel. Expected: %d, Actual: %d", canceled, t4Status);
    }

    TEST(TestTaskOperators_cancellation_and)
    {
        task_completion_event<void> tce;
        task<void> starter(tce);

        cancellation_token_source ct;

        task<void> t1 = starter.then([]() -> void {}, ct.get_token());

        task<void> t2([]() -> void {});

        task<void> t3([]() -> void {});

        auto t4 = (t1 && t2 && t3).then([=]() {});

        ct.cancel();

        tce.set();
        // this should not hang
        task_status t4Status = t4.wait();
        IsTrue(
            t4Status == canceled, L"operator && did not properly cancel. Expected: %d, Actual: %d", canceled, t4Status);
    }

    TEST(TestTaskOperators_cancellation_or)
    {
        task_completion_event<void> tce;
        task<void> starter(tce);

        cancellation_token_source ct1;
        cancellation_token_source ct2;
        cancellation_token_source ct3;

        task<int> t1 = starter.then([]() -> int { return 47; }, ct1.get_token());

        task<int> t2 = starter.then([]() -> int { return 82; }, ct2.get_token());

        task<int> t3 = starter.then([]() -> int { return 147; }, ct3.get_token());

        auto t4 = (t1 || t2 || t3).then([=](int result) -> int { return result; });

        ct1.cancel();
        ct2.cancel();
        ct3.cancel();

        tce.set();
        // this should not hang
        task_status t4Status = t4.wait();
        IsTrue(
            t4Status == canceled, L"operator || did not properly cancel. Expected: %d, Actual: %d", canceled, t4Status);
    }
    TEST(TestTaskOperators_cancellation_or2)
    {
        task_completion_event<void> tce;
        task<void> starter(tce);

        cancellation_token_source ct1;
        cancellation_token_source ct2;
        cancellation_token_source ct3;

        task<void> t1 = starter.then([]() -> void {}, ct1.get_token());

        task<void> t2 = starter.then([]() -> void {}, ct2.get_token());

        task<void> t3 = starter.then([]() -> void {}, ct3.get_token());

        auto t4 = (t1 || t2 || t3).then([=]() {});

        ct1.cancel();
        ct2.cancel();
        ct3.cancel();

        tce.set();
        // this should not hang
        task_status t4Status = t4.wait();
        IsTrue(
            t4Status == canceled, L"operator || did not properly cancel. Expected: %d, Actual: %d", canceled, t4Status);
    }

    TEST(TestTaskOperators_cancellation_complex)
    {
        extensibility::event_t evt1, evt2;
        pplx::details::atomic_long n(0);

        cancellation_token_source ct;

        task<void> t1(
            [&n, &evt1, &evt2]() {
                pplx::details::atomic_add(n, 1L); // this should execute
                evt2.set();
                evt1.wait();
            },
            ct.get_token());

        task<void> t2 = t1.then([&n]() {
            pplx::details::atomic_add(n, 10L); // this should NOT execute
        });

        task<void> t3 = t1.then([&n]() {
            pplx::details::atomic_add(n, 100L); // this should NOT execute
        });

        task<void> t4 = t1.then([&n](task<void> taskResult) {
            pplx::details::atomic_add(n, 1000L); // this should execute
        });

        task<void> t5 = t1.then([&n](task<void> taskResult) {
            try
            {
                taskResult.get();
                pplx::details::atomic_add(n, 10000L); // this should execute
            }
            catch (task_canceled&)
            {
                pplx::details::atomic_add(n, 100000L); // this should NOT execute
            }
        });

        evt2.wait();
        ct.cancel();
        evt1.set();

        IsTrue((t2 && t3).wait() == canceled, L"(t1 && t2) was not canceled");
        IsTrue((t2 || t3 || t4 || t5).wait() == completed, L"(t2 || t3 || t4 || t5) did not complete");
        IsTrue((t4 && t5).wait() == completed, L"(t4 && t5) did not complete");

        try
        {
            t1.get();
        }
        catch (task_canceled&)
        {
            LogFailure(L"get() on canceled task t1 should not throw a task_canceled exception.");
        }

        try
        {
            t2.get();
            LogFailure(L"get() on canceled task t2 should throw a task_canceled exception.");
        }
        catch (task_canceled&)
        {
        }

        try
        {
            t3.get();
            LogFailure(L"get() on canceled task t3 should throw a task_canceled exception.");
        }
        catch (task_canceled&)
        {
        }

        try
        {
            t4.get();
            t5.get();
        }
        catch (...)
        {
            LogFailure(L"get() on completed tasks threw an exception.");
        }
        IsTrue(
            n == 11001L,
            L"The right result was not obtained from the sequence of tasks that executed. Expected: 11001, Actual: %d",
            static_cast<long>(n));
    }

    TEST(TestTaskOperators_cancellation_exception)
    {
        extensibility::event_t evt1, evt2;
        pplx::details::atomic_long n(0);

        cancellation_token_source ct;

        task<void> t1(
            [&n, &evt1, &evt2]() {
                evt2.set();
                evt1.wait();
            },
            ct.get_token());

        task<void> t2([&n]() { throw 42; });

        for (int i = 0; i < 5; ++i)
        {
            try
            {
                t2.get();
                LogFailure(L"Exception was not received from t2.get()");
            }
            catch (int x)
            {
                IsTrue(x == 42, L"Incorrect integer was thrown from t2.get(). Expected: 42, Actual: %d", x);
            }
            catch (task_canceled&)
            {
                LogFailure(L"task_canceled was thrown from t2.get() when an integer was expected");
            }
        }

        for (int i = 0; i < 5; ++i)
        {
            try
            {
                t2.wait();
                LogFailure(L"Exception was not received from t2.wait()");
            }
            catch (int x)
            {
                IsTrue(x == 42, L"Incorrect integer was thrown from t2.wait(). Expected: 42, Actual: %d", x);
            }
            catch (task_canceled&)
            {
                LogFailure(L"task_canceled was thrown from t2.wait() when an integer was expected");
            }
        }

        task<void> t3 = t1.then([&n]() {
            pplx::details::atomic_add(n, 1L); // this should NOT execute,
        });

        task<void> t4 = t1.then([&n](task<void> taskResult) {
            pplx::details::atomic_add(n, 10L); // this should execute
        });

        task<void> t5 = t2.then([&n]() {
            pplx::details::atomic_add(n, 100L); // this should NOT execute
        });

        task<void> t6 = t2.then([&n](task<void> taskResult) {
            pplx::details::atomic_add(n, 1000L);  // this should execute
            taskResult.get();                     // should throw 42
            pplx::details::atomic_add(n, 10000L); // this should NOT execute
        });

        task<void> t7 = t2.then([&n, this](task<void> taskResult) {
            try
            {
                taskResult.get();
                pplx::details::atomic_add(n, 100000L); // this should NOT execute
            }
            catch (int x)
            {
                IsTrue(
                    x == 42,
                    L"Incorrect integer exception was received in t7 from taskresult.get(). Expected: 42, Actual: %d",
                    x);
                pplx::details::atomic_add(n, 1000000L); // this should execute
            }
            catch (task_canceled)
            {
                LogFailure(L"task_canceled was thrown by taskResult.get() in t7");
            }
            catch (...)
            {
                LogFailure(L"A random exception was thrown by taskResult.get() in t7");
            }

            throw 96;
        });

        task<void> t8 = (t6 || t7).then([&n, this](task<void> taskResult) {
            try
            {
                taskResult.get();
                pplx::details::atomic_add(n, 1000000L); // this should NOT execute
            }
            catch (int x)
            {
                IsTrue((x == 42 || x == 96),
                       L"Incorrect integer exception was received in t7 from taskresult.get(). Expected: 42 or 96, "
                       L"Actual: %d",
                       x);
                pplx::details::atomic_add(n, 100000000L); // this should execute
            }
            catch (task_canceled)
            {
                LogFailure(L"(t6 || t7) was canceled without an exception");
            }
            catch (...)
            {
                LogFailure(L"(t6 || t7) was canceled with an unexpected exception");
            }
        });

        // Cancel t1 now that t2 is guaranteed canceled with an exception
        evt2.wait();
        ct.cancel();
        evt1.set();

        try
        {
            task_status status = (t1 && t2).wait();
            IsTrue((status == canceled),
                   L"(t1 && t2).wait() did not return canceled. Expected: %d, Actual %d",
                   canceled,
                   status);
        }
        catch (int x)
        {
            IsTrue(x == 42,
                   L"Incorrect integer exception was received from (t1 && t2).wait(). Expected: 42, Actual: %d",
                   x);
        }

        try
        {
            task_status status = t3.wait();
            IsTrue((status == canceled),
                   L"t3.wait() did not returned canceled. Expected: %d, Actual %d",
                   canceled,
                   status);
        }
        catch (task_canceled&)
        {
            LogFailure(L"t3.wait() threw task_canceled instead of returning canceled");
        }
        catch (...)
        {
            LogFailure(L"t3.wait() threw an unexpected exception");
        }

        try
        {
            task_status status = t4.wait();
            IsTrue((status == completed),
                   L"t4.wait() did not returned completed. Expected: %d, Actual %d",
                   completed,
                   status);
        }
        catch (...)
        {
            LogFailure(L"t4.wait() threw an unexpected exception");
        }

        try
        {
            t5.wait();
            LogFailure(L"t5.wait() did not throw an exception");
        }
        catch (int x)
        {
            IsTrue(x == 42, L"Incorrect integer exception was received from t5.wait(). Expected: 42, Actual: %d", x);
        }

        // Observe the exceptions from t5, t6 and t7
        helpers::ObserveException(t5);
        helpers::ObserveException(t6);
        helpers::ObserveException(t7);

        try
        {
            (t1 || t6).get();
            LogFailure(L"(t1 || t6).get() should throw an exception.");
        }
        catch (task_canceled&)
        {
            LogFailure(L"(t1 || t6).get() threw task_canceled when an int was expected.");
        }
        catch (int x)
        {
            IsTrue(
                (x == 42 || x == 96),
                L"Incorrect integer exception was received from (t1 || t6 || t7).get(). Expected: 42 or 96, Actual: %d",
                x);
        }

        t8.wait();

        IsTrue(n == 101001010L,
               L"The right result was not obtained from the sequence of tasks that executed. Expected 101001010, "
               L"actual %d",
               101001010,
               static_cast<long>(n));
    }

    TEST(TestTaskOperators_when_all_cancellation)
    {
        // A task that participates in a 'when all' operation is canceled and then throws an exception. Verify that
        // value and task based continuations of the when all task see the exception.
        extensibility::event_t evt1, evt2;

        cancellation_token_source ct;

        task<void> t1(
            [&evt1, &evt2]() {
                evt2.set();
                evt1.wait();
                os_utilities::sleep(100);
                throw 42;
            },
            ct.get_token());

        task<void> t2([]() { helpers::DoRandomParallelWork(); });

        task<void> t3([]() { helpers::DoRandomParallelWork(); });

        task<void> whenAllTask = t1 && t2 && t3;

        task<void> t4 = whenAllTask.then([this](task<void> t) {
            IsFalse(helpers::VerifyCanceled(t), L"%ws:%u:t should be canceled by token", __FILE__, __LINE__);
            IsTrue(helpers::VerifyException<int>(t), L"%ws:%u:exception from t is unexpected", __FILE__, __LINE__);
        });

        task<void> t5 =
            whenAllTask.then([this]() { LogFailure(L"%ws:%u:t5 was unexpectedly executed", __FILE__, __LINE__); });

        evt2.wait();
        ct.cancel();
        evt1.set();

        IsFalse(helpers::VerifyCanceled(t5), L"%ws:%u:t5 should be canceled", __FILE__, __LINE__);
    }

    TEST(TestTaskOperators_when_all_cancellation_sequence)
    {
        // A task that participates in a 'when all' operation throws an exception, but a continuation of the when all
        // task is canceled before this point. Ensure that continuation does not get the exception but others do.
        extensibility::event_t evt1, evt2;

        cancellation_token_source ct;

        task<void> t1([&evt1, &evt2]() {
            evt2.set();
            evt1.wait();
            os_utilities::sleep(100);
            throw 42;
        });

        task<void> t2([]() { helpers::DoRandomParallelWork(); });

        task<void> t3([]() { helpers::DoRandomParallelWork(); });

        task<void> whenAllTask = t1 && t2 && t3;

        task<void> t4 = whenAllTask.then([this](task<void> t) {
            IsFalse(helpers::VerifyCanceled(t), L"%ws:%u:t was unexpectedly canceled", __FILE__, __LINE__);
            IsTrue(helpers::VerifyException<int>(t),
                   L"%ws:%u:Did not receive the correct exception from t",
                   __FILE__,
                   __LINE__);
        });

        task<void> t5 =
            whenAllTask.then([this]() { LogFailure(L"%ws:%u:t5 was unexpectedly executed", __FILE__, __LINE__); });

        task<void> t6 = whenAllTask.then(
            [this](task<void> t) {
                IsTrue(helpers::VerifyCanceled(t), L"%ws:%u:t was not canceled as expected", __FILE__, __LINE__);
            },
            ct.get_token());

        evt2.wait();
        ct.cancel();
        evt1.set();

        IsTrue(helpers::VerifyException<int>(t5),
               L"%ws:%u:Did not receive the correct exception from t5",
               __FILE__,
               __LINE__);
    }

    TEST(TestTaskOperators_and_cancellation_multiple_tokens)
    //
    // operator&& with differing tokens:
    //
    {
        cancellation_token_source ct1;
        cancellation_token_source ct2;
        cancellation_token_source ct3;
        cancellation_token_source ct4;

        task<int> t1([]() -> int { return 42; }, ct1.get_token());

        task<int> t2([]() -> int { return 77; }, ct2.get_token());

        task<int> t3([]() -> int { return 92; }, ct3.get_token());

        task<int> t4([]() -> int { return 147; }, ct4.get_token());

        auto t5 = t1 && t2 && t3 && t4;

        extensibility::event_t ev1, ev2;

        auto t6 = t5.then([&ev1, &ev2](std::vector<int> iVec) -> int {
            ev2.set();
            ev1.wait();
            return iVec[0] + iVec[1] + iVec[2] + iVec[3];
        });

        auto t7 = t6.then([](int val) -> int { return val; });

        ev2.wait();
        ct3.cancel();
        ev1.set();
        t6.wait();
        t7.wait();

        bool caughtCanceled = false;

        try
        {
            t7.get();
        }
        catch (task_canceled&)
        {
            caughtCanceled = true;
        }

        IsTrue(caughtCanceled, L"Cancellation token was not joined/inherited on operator&&");
    }

    struct TestException1
    {
    };

    struct TestException2
    {
    };

    // CodePlex 292
    static int ThrowFunc() { throw 42; }

    TEST(TestContinuationsWithTask1)
    {
        int n2 = 0;

        task<int> t([&]() -> int { return 10; });

        t.then([&](task<int> ti) { n2 = ti.get(); }).wait();

        VERIFY_IS_TRUE(n2 == 10);
    }

    TEST(TestContinuationsWithTask2)
    {
        int n = 0;

        task<void> tt1([]() {});
        auto tt2 = tt1.then([&]() -> task<void> {
            task<void> tt3([&]() { n = 1; });
            return tt3;
        });

        tt2.get();
        VERIFY_IS_TRUE(n == 1);

        task<void> tt4 = tt2.then([&]() -> task<void> {
            task<void> tt5([&]() { n = 2; });
            return tt5;
        });
        tt4.get();
        VERIFY_IS_TRUE(n == 2);
    }

    TEST(TestContinuationsWithTask3)
    {
        bool gotException = true;
        int n2 = 0;
        task<int> t(ThrowFunc);
        t.then([&](task<int> ti) {
             try
             {
                 ti.get();
                 gotException = false;
             }
             catch (int)
             {
                 n2 = 20;
             }
         })
            .wait();

        VERIFY_IS_TRUE(gotException);
        VERIFY_IS_TRUE(n2 == 20);
    }

    TEST(TestContinuationsWithTask4)
    {
        int n2 = 0;

        task<int> t([&]() -> int { return 10; });

        t.then([&](int n) -> task<int> {
             task<int> t2([n]() -> int { return n + 10; });
             return t2;
         })
            .then([&](int n) { n2 = n; })
            .wait();

        VERIFY_IS_TRUE(n2 == 20);
    }

    TEST(TestContinuationsWithTask5)
    {
        int n2 = 0;

        task<int> t([&]() -> int { return 10; });

        t.then([&](task<int> tn) -> task<int> {
             int n = tn.get();
             task<int> t2([n]() -> int { return n + 10; });
             return t2;
         })
            .then([&](task<int> n) { n2 = n.get(); })
            .wait();

        VERIFY_IS_TRUE(n2 == 20);
    }

    TEST(TestContinuationsWithTask6)
    {
        pplx::details::atomic_long hit(0);
        auto* hitptr = &hit;
        task<int> t([]() { return 10; });

        auto ot = t.then([hitptr](int n) -> task<int> {
            auto hitptr1 = hitptr;
            task<int> it([n, hitptr1]() -> int {
                os_utilities::sleep(100);
                pplx::details::atomic_exchange(*hitptr1, 1L);
                return n * 2;
            });

            return it;
        });

        int value = ot.get();
        VERIFY_IS_TRUE(value == 20 && hit != 0);
    }

    TEST(TestContinuationsWithTask7)
    {
        volatile long hit = 0;
        volatile long* hitptr = &hit;

        task<int> t([]() { return 10; });

        auto ot = t.then([hitptr](int n) -> task<int> {
            task<int> it([n, hitptr]() -> int { throw TestException1(); });

            return it;
        });

        VERIFY_IS_TRUE(helpers::VerifyException<TestException1>(ot));
    }

    TEST(TestContinuationsWithTask8)
    {
        volatile long hit = 0;
        volatile long* hitptr = &hit;

        task<int> t([]() { return 10; });

        auto ot = t.then([hitptr](int n) -> task<int> {
            volatile long* hitptr1 = hitptr;
            task<int> it([n, hitptr1]() -> int {
                os_utilities::sleep(100);
                os_utilities::interlocked_exchange(hitptr1, 1);

                // This test is needed to disable an optimizer dead-code check that
                // winds up generating errors in VS 2010.
                if (n == 10) throw TestException2();

                return n * 3;
            });

            return it;
        });

        VERIFY_IS_TRUE(helpers::VerifyException<TestException2>(ot),
                       "(7) Inner task exception not propagated out of outer .get()");
        VERIFY_IS_TRUE(hit != 0, "(7) Expected inner task hit marker to be set!");
    }

    TEST(TestContinuationsWithTask9)
    {
        volatile long hit = 0;
        volatile long* hitptr = &hit;
        extensibility::event_t e;
        task<int> it;

        task<int> t([]() { return 10; });

        auto ot = t.then([hitptr, &it, &e](int n) -> task<int> {
            volatile long* hitptr1 = hitptr;
            it = task<int>([hitptr1, n]() -> int {
                os_utilities::interlocked_exchange(hitptr1, 1);
                // This test is needed to disable an optimizer dead-code check that
                // winds up generating errors in VS 2010.
                if (n == 10) throw TestException1();
                return n * 5;
            });

            e.set();
            os_utilities::sleep(100);
            // This test is needed to disable an optimizer dead-code check that
            // winds up generating errors in VS 2010.
            if (n == 10) throw TestException2();
            return it;
        });

        e.wait();

        VERIFY_IS_TRUE(helpers::VerifyException<TestException2>(ot),
                       "(8) Outer task exception not propagated when inner task also throws");
        VERIFY_IS_TRUE(helpers::VerifyException<TestException1>(it),
                       "(8) Inner task exception not explicitly propgated on pass out / get");
        VERIFY_IS_TRUE(hit != 0, "(8) Inner hit marker expected!");
    }

    TEST(TestContinuationsWithTask10)
    {
        volatile long hit = 0;

        task<int> t([]() { return 10; });

        auto ot = t.then([&](int n) -> task<int> {
            task<int> it([&, n]() -> int {
                os_utilities::sleep(100);
                // This test is needed to disable an optimizer dead-code check that
                // winds up generating errors in VS 2010.
                if (n == 10) throw TestException1();
                return n * 6;
            });
            return it;
        });

        auto otc = ot.then([&](task<int> itp) {
            os_utilities::interlocked_exchange(&hit, 1);
            VERIFY_IS_TRUE(helpers::VerifyException<TestException1>(itp),
                           "(9) Outer task exception handling continuation did not get plumbed inner exception");
        });

        VERIFY_IS_TRUE(helpers::VerifyException<TestException1>(ot),
                       "(9) Inner task exception not propagated correctly");
        helpers::ObserveException(otc);
        VERIFY_IS_TRUE(hit != 0, "(9) Outer task exception handling continuation did not run!");
    }

    TEST(TestUnwrappingCtors)
    {
        int res;
        {
            // take task<int> in the ctor

            task<int> ti([]() -> int { return 1; });

            // Must unwrap:
            task<int> t1(ti);
            res = t1.get();
            VERIFY_IS_TRUE(res == 1, "unexpected value in TestUnwrappingCtors, location 1");
        }

        {
            // take lambda returning task<int> in the ctor

            // Must NOT unwrap:
            task<task<int>> t1([]() -> task<int> {
                task<int> ti([]() -> int { return 1; });
                return ti;
            });
            res = t1.get().get();
            VERIFY_IS_TRUE(res == 1, "unexpected value in TestUnwrappingCtors, location 2");

            // Must unwrap:
            task<int> t2([]() -> task<int> {
                task<int> ti([]() -> int { return 2; });
                return ti;
            });
            res = t2.get();
            VERIFY_IS_TRUE(res == 2, "unexpected value in TestUnwrappingCtors, location 3");

            res = t2.then([](int n) { return n + 1; }).get();
            VERIFY_IS_TRUE(res == 3, "unexpected value in TestUnwrappingCtors, location 4");
        }

        {
            int executed = 0;
            // take task<void> in the ctor
            task<void> ti([&]() { executed = 1; });

            // Must unwrap:
            task<void> t1(ti);
            t1.wait();
            VERIFY_IS_TRUE(executed == 1, "unexpected value in TestUnwrappingCtors, location 5");
        }

        {
            // take lambda returning task<void> in the ctor

            int executed = 0;
            int* executedPtr = &executed;

            // Must NOT unwrap:
            task<task<void>> t1([executedPtr]() -> task<void> {
                auto executedPtr1 = executedPtr;
                task<void> ti([executedPtr1]() { *executedPtr1 = 1; });
                return ti;
            });
            t1.get().get();
            VERIFY_IS_TRUE(executed == 1, "unexpected value in TestUnwrappingCtors, location 6");

            task<void> t2([]() {});
            // Must unwrap:
            task<void> t3 = t2.then([executedPtr]() -> task<void> {
                auto executedPtr1 = executedPtr;
                task<void> ti([executedPtr1]() { *executedPtr1 = 2; });
                return ti;
            });

            t3.wait();
            VERIFY_IS_TRUE(executed == 2, "unexpected value in TestUnwrappingCtors, location 7");

            // Must unwrap:
            task<void> t4([executedPtr]() -> task<void> {
                auto executedPtr1 = executedPtr;
                task<void> ti([executedPtr1]() { *executedPtr1 = 3; });
                return ti;
            });
            t4.wait();
            VERIFY_IS_TRUE(executed == 3, "unexpected value in TestUnwrappingCtors, location 8");

            t4.then([&]() { executed++; }).wait();
            VERIFY_IS_TRUE(executed == 4, "unexpected value in TestUnwrappingCtors, location 9");
        }

        {
            res = create_task([]() -> task<int> { return create_task([]() -> int { return 1; }); }).get();
            VERIFY_IS_TRUE(res == 1, "unexpected value in TestUnwrappingCtors, create_task, location 1");

            create_task([]() -> task<void> { return create_task([]() {}); }).wait();
        }

        {
            // BUG TFS: 344954
            cancellation_token_source cts, cts2;
            cts.cancel(); // Commenting this line out makes the program work!
            // Create a task that is always cancelled
            auto falseTask = create_task([]() {}, cts.get_token());
            cancellation_token ct2 = cts2.get_token();
            create_task(
                [falseTask]() {
                    // Task unwrapping!
                    // This should not crash
                    return falseTask;
                },
                ct2)
                .then([this, falseTask, ct2](task<void> t) -> task<void> {
                    VERIFY_IS_TRUE(t.wait() == canceled,
                                   "unexpected value in TestUnwrappingCtors, cancellation token, location 1");
                    VERIFY_IS_TRUE(!ct2.is_canceled(),
                                   "unexpected value in TestUnwrappingCtors, cancellation token, location 2");
                    // again, unwrapping in continuation
                    // this should not crash
                    return falseTask;
                })
                .then([this] {
                    VERIFY_IS_TRUE(false, "unexpected path in TestUnwrappingCtors, cancellation token, location 3");
                });
        }
    }

    TEST(TestNestedTasks)
    {
        {
            task<int> rootTask([]() -> int { return 234; });

            task<task<int>> resultTask = rootTask.then([](int value) -> task<task<int>> {
                return task<task<int>>([=]() -> task<int> {
                    auto val1 = value;
                    return task<int>([=]() -> int { return val1 + 22; });
                });
            });

            int n = resultTask.get().get();
            VERIFY_IS_TRUE(n == 256, "TestNestedTasks_1");
        }

        {
            // Same for void task
            int flag = 1;
            int* flagptr = &flag;
            task<void> rootTask([&]() { flag++; });

            task<task<void>> resultTask = rootTask.then([flagptr]() -> task<task<void>> {
                auto flag1 = flagptr;
                return task<task<void>>([flag1]() -> task<void> {
                    auto flag2 = flag1;
                    return task<void>([flag2]() { ++(flag2[0]); });
                });
            });

            resultTask.get().wait();
            VERIFY_IS_TRUE(flag == 3, "TestNestedTasks_2");
        }

        {
            task<int> rootTask([]() -> int { return 234; });

            task<task<task<int>>> resultTask = rootTask.then([](int value) -> task<task<task<int>>> {
                return task<task<task<int>>>([=]() -> task<task<int>> {
                    auto v1 = value;
                    return task<task<int>>([=]() -> task<int> {
                        auto v2 = v1;
                        return task<int>([=]() -> int { return v2 + 22; });
                    });
                });
            });

            int n = resultTask.get().get().get();
            VERIFY_IS_TRUE(n == 256, "TestNestedTasks_3");
        }

        {
            task<void> nestedTask;
            task<void> unwrap([&]() -> task<void> {
                nestedTask = task<void>([]() { cancel_current_task(); });
                return nestedTask;
            });
            task_status st = unwrap.wait();
            VERIFY_IS_TRUE(st == canceled, "TestNestedTasks_4");
            st = nestedTask.wait();
            VERIFY_IS_TRUE(st == canceled, "TestNestedTasks_5 ");
        }
    }

    template<typename Function>
    task<void> async_for(int start, int step, int end, Function func)
    {
        if (start < end)
        {
            return func(start).then([=]() -> task<void> { return async_for(start + step, step, end, func); });
        }
        else
        {
            return task<void>([] {});
        }
    }

    TEST(TestInlineChunker)
    {
        const int numiter = 1000;
        volatile int sum = 0;
        async_for(0,
                  1,
                  numiter,
                  [&](int) -> task<void> {
                      sum++;
                      return create_task([]() {});
                  })
            .wait();

        VERIFY_IS_TRUE(sum == numiter, "TestInlineChunker: async_for did not return correct result.");
    }

#if defined(_WIN32) && (_MSC_VER >= 1700) && (_MSC_VER < 1800)

    TEST(PPL_Conversions_basic)
    {
        pplx::task<int> t1([] { return 1; });
        concurrency::task<int> t2 = pplx::pplx_task_to_concurrency_task(t1);
        int n = t2.get();
        VERIFY_ARE_EQUAL(n, 1);

        pplx::task<int> t3 = pplx::concurrency_task_to_pplx_task(t2);
        int n2 = t3.get();
        VERIFY_ARE_EQUAL(n2, 1);
    }

    TEST(PPL_Conversions_Nested)
    {
        pplx::task<int> t1([] { return 12; });
        pplx::task<int> t2 = pplx::concurrency_task_to_pplx_task(pplx::pplx_task_to_concurrency_task(
            pplx::concurrency_task_to_pplx_task(pplx::pplx_task_to_concurrency_task(t1))));
        int n = t2.get();
        VERIFY_ARE_EQUAL(n, 12);
    }

    TEST(PPL_Conversions_Exceptions)
    {
        pplx::task<int> t1(ThrowFunc);
        concurrency::task<int> t2 = pplx::pplx_task_to_concurrency_task(t1);
        try
        {
            t2.get();
            VERIFY_IS_TRUE(false);
        }
        catch (int m)
        {
            VERIFY_ARE_EQUAL(m, 42);
        }

        pplx::task<int> t3 = pplx::concurrency_task_to_pplx_task(t2);
        try
        {
            t3.get();
            VERIFY_IS_TRUE(false);
        }
        catch (int m)
        {
            VERIFY_ARE_EQUAL(m, 42);
        }
    }

    TEST(PPL_Conversions_Basic_void)
    {
        pplx::task<void> t1([] {});
        concurrency::task<void> t2 = pplx::pplx_task_to_concurrency_task(t1);
        t2.get();

        pplx::task<void> t3 = pplx::concurrency_task_to_pplx_task(t2);
        t3.get();
    }

    TEST(PPL_Conversions_Exceptions_void)
    {
        pplx::task<void> t1([]() { throw 3; });
        concurrency::task<void> t2 = pplx::pplx_task_to_concurrency_task(t1);
        try
        {
            t2.get();
            VERIFY_IS_TRUE(false);
        }
        catch (int m)
        {
            VERIFY_ARE_EQUAL(m, 3);
        }

        pplx::task<void> t3 = pplx::concurrency_task_to_pplx_task(t2);
        try
        {
            t3.get();
            VERIFY_IS_TRUE(false);
        }
        catch (int m)
        {
            VERIFY_ARE_EQUAL(m, 3);
        }
    }

#endif

} // SUITE(pplxtask_tests)

} // namespace PPLX
} // namespace functional
} // namespace tests
