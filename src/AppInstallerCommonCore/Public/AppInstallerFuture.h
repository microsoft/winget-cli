// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <future>

namespace AppInstaller
{
    // Future wrapper that enables progress to be hooked up by caller.
    template <typename Result>
    struct Future
    {
        Future(std::future&& other);
        Future(const std::shared_future& other);

        Future(const Future&) = default;
        Future& operator=(const Future&) = default;

        Future(Future&&) = default;
        Future& operator=(Future&&) = default;

        // Cancel the processing of the future, if possible.
        void Cancel();

        T Get();

    private:
        std::shared_future m_future;
    };
}
