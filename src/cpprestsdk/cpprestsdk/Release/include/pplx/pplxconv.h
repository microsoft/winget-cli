/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Utilities to convert between PPL tasks and PPLX tasks
 *
 * For the latest on this and related APIs, please see: https://github.com/Microsoft/cpprestsdk
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#pragma once

#ifndef _PPLXCONV_H
#define _PPLXCONV_H

#ifndef _WIN32
#error This is only supported on Windows
#endif

#if defined(_MSC_VER) && (_MSC_VER >= 1700) && (_MSC_VER < 1800) && !CPPREST_FORCE_PPLX

#include "pplx/pplxtasks.h"
#include <ppltasks.h>

namespace pplx
{
namespace _Ppl_conv_helpers
{
template<typename _Tc, typename _F>
auto _Set_value(_Tc _Tcp, const _F& _Func) -> decltype(_Tcp.set(_Func()))
{
    return _Tcp.set(_Func());
}

template<typename _Tc, typename _F>
auto _Set_value(_Tc _Tcp, const _F& _Func, ...) -> decltype(_Tcp.set())
{
    _Func();
    return _Tcp.set();
}

template<typename _TaskType, typename _OtherTaskType, typename _OtherTCEType>
_OtherTaskType _Convert_task(_TaskType _Task)
{
    _OtherTCEType _Tc;
    _Task.then([_Tc](_TaskType _Task2) {
        try
        {
            _Ppl_conv_helpers::_Set_value(_Tc, [=] { return _Task2.get(); });
        }
        catch (...)
        {
            _Tc.set_exception(std::current_exception());
        }
    });
    _OtherTaskType _T_other(_Tc);
    return _T_other;
}
} // namespace _Ppl_conv_helpers

template<typename _TaskType>
concurrency::task<_TaskType> pplx_task_to_concurrency_task(pplx::task<_TaskType> _Task)
{
    return _Ppl_conv_helpers::_Convert_task<typename pplx::task<_TaskType>,
                                            concurrency::task<_TaskType>,
                                            concurrency::task_completion_event<_TaskType>>(_Task);
}

template<typename _TaskType>
pplx::task<_TaskType> concurrency_task_to_pplx_task(concurrency::task<_TaskType> _Task)
{
    return _Ppl_conv_helpers::_Convert_task<typename concurrency::task<_TaskType>,
                                            pplx::task<_TaskType>,
                                            pplx::task_completion_event<_TaskType>>(_Task);
}
} // namespace pplx

#endif

#endif // _PPLXCONV_H
