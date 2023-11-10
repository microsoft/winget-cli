/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Apple-specific pplx implementations
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include "stdafx.h"

#include "pplx/pplx.h"
#include <dispatch/dispatch.h>
#include <errno.h>
#include <libkern/OSAtomic.h>
#include <pthread.h>
#include <sys/time.h>

// DEVNOTE:
// The use of mutexes is suboptimal for synchronization of task execution.
// Given that scheduler implementations should use GCD queues, there are potentially better mechanisms available to
// coordinate tasks (such as dispatch groups).

namespace pplx
{
namespace details
{
namespace platform
{
_PPLXIMP long GetCurrentThreadId()
{
    pthread_t threadId = pthread_self();
    return (long)threadId;
}

void YieldExecution() { sleep(0); }

} // namespace platform

void apple_scheduler::schedule(TaskProc_t proc, void* param)
{
    dispatch_queue_t queue = dispatch_get_global_queue(DISPATCH_QUEUE_PRIORITY_DEFAULT, 0);
    dispatch_async_f(queue, param, proc);
}

} // namespace details

} // namespace pplx
