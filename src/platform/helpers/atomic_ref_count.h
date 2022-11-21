#pragma once

#include <atomic>
#include <stdint.h>
#include "pal_internal.h"

namespace xlang::impl
{
    struct atomic_ref_count
    {
        int32_t operator++() noexcept;
        int32_t operator--() noexcept;

        int32_t get_count() const noexcept;

    private:
        std::atomic<int32_t> count{ 1 };
    };

    inline int32_t atomic_ref_count::operator++() noexcept
    {
        // Safe to increment relaxed. New references can only be formed from an existing reference.
        // Passing an existing reference from one thread to another, must already provide synchronization.
        auto result = count.fetch_add(1, std::memory_order_relaxed) + 1;
        XLANG_ASSERT(result > 1);
        return result;
    }

    inline int32_t atomic_ref_count::operator--() noexcept
    {
        // This could be std::memory_order_acq_rel, but would result in an unneeded "aquire"
        // operations when the counter has not yet reached zero. This case is protected via the
        // fence later.
        // memory_order_release on decrement forces all other writes on the current thread to
        // be visible before the decremented refcount is stored
        auto result = count.fetch_sub(1, std::memory_order_release) - 1;
        XLANG_ASSERT(result >= 0);
        if (result == 0)
        {
            // This acquire fence ensures that all relaxed/nonatomic stores preceding the release in other threads
            // will happen before anything after the fence in this thread. And at this point, we should be the only
            // thread with a reference to the object, so it is now safe to delete.
            std::atomic_thread_fence(std::memory_order_acquire);
        }
        return result;
    }

    inline int32_t atomic_ref_count::get_count() const noexcept
    {
        return count.load(std::memory_order_acquire);
    }
}
