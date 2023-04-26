// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <winrt/Windows.Foundation.h>
#include <memory>

namespace AppInstaller::WinRT
{
    namespace details
    {
        // Type erasing interface for winrt cancellation token.
        struct AsyncCancellationTypeErasure
        {
            virtual ~AsyncCancellationTypeErasure() = default;

            virtual bool IsCancelled() const noexcept = 0;
            virtual void Callback(winrt::delegate<>&& callback) const noexcept = 0;
        };

        // Type containing winrt cancellation token wrapper.
        template <typename Promise>
        struct AsyncCancellationT : public AsyncCancellationTypeErasure
        {
            using Token = winrt::impl::cancellation_token<Promise>;

            AsyncCancellationT(Token&& token) : m_token(std::move(token)) {}

            bool IsCancelled() const noexcept override
            {
                return m_token();
            }

            void Callback(winrt::delegate<>&& callback) const noexcept override
            {
                m_token.callback(std::move(callback));
            }

        private:
            Token m_token;
        };
    }

    // May hold a cancellation token and provide the ability to check its status.
    // If empty, it will act as if it is never cancelled.
    struct AsyncCancellation
    {
        // Create an empty cancellation object, which will never be cancelled.
        AsyncCancellation() = default;

        // Create a cancellation object from the winrt token.
        template <typename Promise>
        AsyncCancellation(winrt::impl::cancellation_token<Promise>&& token)
        {
            m_token = std::make_unique<details::AsyncCancellationT<Promise>>(std::move(token));
        }

        // Returns true if the operation has been cancelled, false if not.
        bool IsCancelled() const noexcept
        {
            return m_token ? m_token->IsCancelled() : false;
        }

        // Sets a callback that will be invoked on cancellation.
        void Callback(winrt::delegate<>&& callback) const noexcept
        {
            if (m_token)
            {
                m_token->Callback(std::move(callback));
            }
        }

    private:
        std::unique_ptr<details::AsyncCancellationTypeErasure> m_token;
    };

    namespace details
    {
        // Type erasing interface for winrt progess token.
        template <typename Progress>
        struct AsyncProgessTypeErasure
        {
            virtual ~AsyncProgessTypeErasure() = default;

            virtual void Progress(Progress const& result) const = 0
        };

        // Type containing winrt progress token wrapper.
        template <typename Promise, typename Progress>
        struct AsyncProgressT : public AsyncProgessTypeErasure
        {
            using Token = winrt::impl::progress_token<Promise, Progress>;

            AsyncProgressT(Token&& token) : m_token(std::move(token)) {}

            void Progress(Progress const& result) const override
            {
                m_token(result);
            }

        private:
            Token m_token;
        };
    }

    // May hold a progress token and provide the ability to send progress updates.
    // If empty, progress will be dropped on calls here.
    template <typename ProgressT>
    struct AsyncProgress : public AsyncCancellation
    {
        // Create an empty progress object.
        AsyncProgress() = default;

        // Create a progress object from the winrt token.
        template <typename Promise>
        AsyncProgress(winrt::impl::progress_token<Promise, Progress>&& progress, winrt::impl::cancellation_token<Promise>&& cancellation) :
            AsyncCancellation(std::move(cancellation))
        {
            m_token = std::make_unique<details::AsyncProgressT<Promise, ProgressT>>(std::move(token));
        }

        // Sends progress if this object is not empty.
        void Progress(ProgressT const& result) const
        {
            if (m_token)
            {
                m_token->Progress(result);
            }
        }

    private:
        std::unique_ptr<details::AsyncProgessTypeErasure<ProgressT>> m_token;
    };
}
