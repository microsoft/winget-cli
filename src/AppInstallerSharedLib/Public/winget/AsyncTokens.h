// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <AppInstallerLogging.h>
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
            virtual void Cancel() noexcept = 0;
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

            void Cancel() noexcept override
            {
                // This is a bit of a hack, but the cancellation_token provides no access to the underlying promise for the purpose of cancellation.
                static_assert(sizeof(Token) == sizeof(Promise*), "We expect that the cancellation_token has only 1 member and it is a Promise*");
                (*reinterpret_cast<Promise**>(&m_token))->Cancel();
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
            m_token = std::make_shared<details::AsyncCancellationT<Promise>>(std::move(token));
        }

        // Returns true if the operation has been cancelled, false if not.
        bool IsCancelled() const noexcept
        {
            return m_token ? m_token->IsCancelled() : false;
        }

        // Throws the appropriate exception if the operation has been cancelled.
        void ThrowIfCancelled() const
        {
            if (IsCancelled())
            {
                AICLI_LOG(Core, Warning, << "Operation cancelled");
                throw winrt::hresult_canceled();
            }
        }

        // Sets a callback that will be invoked on cancellation.
        void Callback(winrt::delegate<>&& callback) const noexcept
        {
            if (m_token)
            {
                m_token->Callback(std::move(callback));
            }
        }

        std::weak_ptr<details::AsyncCancellationTypeErasure> GetWeak()
        {
            return m_token;
        }

    private:
        std::shared_ptr<details::AsyncCancellationTypeErasure> m_token;
    };

    namespace details
    {
        // Type erasing interface for winrt progress token.
        template <typename ResultT, typename ProgressT>
        struct AsyncProgressTypeErasure
        {
            virtual ~AsyncProgressTypeErasure() = default;

            virtual void Progress(ProgressT const& progress) const = 0;

            virtual void Result(ResultT const& result) const = 0;
        };

        // Type containing winrt progress token wrapper.
        template <typename Promise, typename ResultT, typename ProgressT>
        struct AsyncProgressT : public AsyncProgressTypeErasure<ResultT, ProgressT>
        {
            using Token = winrt::impl::progress_token<Promise, ProgressT>;

            AsyncProgressT(Token&& token) : m_token(std::move(token)) {}

            void Progress(ProgressT const& progress) const override
            {
                m_token(progress);
            }

            void Result(ResultT const& result) const override
            {
                m_token.set_result(result);
            }

        private:
            Token m_token;
        };

        // Type containing winrt EventHandler.
        template <typename ResultT, typename ProgressT>
        struct AsyncProgressEventHandlerT : public AsyncProgressTypeErasure<ResultT, ProgressT>
        {
            using Token = winrt::Windows::Foundation::EventHandler<ProgressT>;

            AsyncProgressEventHandlerT(Token&& token) : m_token(std::move(token)) {}

            void Progress(ProgressT const& progress) const override
            {
                m_token(m_result, progress);
            }

            void Result(ResultT const& result) const override
            {
                m_result = result;
            }

        private:
            mutable ResultT m_result = nullptr;
            Token m_token;
        };
    }

    // May hold a progress token and provide the ability to send progress updates.
    // If empty, progress will be dropped on calls here.
    template <typename ResultT, typename ProgressT>
    struct AsyncProgress : public AsyncCancellation
    {
        // Create an empty progress object.
        AsyncProgress() = default;

        // Create a progress object from the winrt token.
        template <typename Promise>
        AsyncProgress(winrt::impl::progress_token<Promise, ProgressT>&& progress, winrt::impl::cancellation_token<Promise>&& cancellation) :
            AsyncCancellation(std::move(cancellation))
        {
            m_token = std::make_unique<details::AsyncProgressT<Promise, ResultT, ProgressT>>(std::move(progress));
        }

        // Create a progress object from an EventHandler.
        template <typename Promise>
        AsyncProgress(winrt::Windows::Foundation::EventHandler<ProgressT>&& progress, winrt::impl::cancellation_token<Promise>&& cancellation) :
            AsyncCancellation(std::move(cancellation))
        {
            m_token = std::make_unique<details::AsyncProgressEventHandlerT<ResultT, ProgressT>>(std::move(progress));
        }

        // Create a cancellation only object.
        template <typename Promise>
        AsyncProgress(winrt::impl::cancellation_token<Promise>&& cancellation) :
            AsyncCancellation(std::move(cancellation))
        {
        }

        // Sends progress if this object is not empty.
        void Progress(ProgressT const& progress) const
        {
            if (m_token)
            {
                m_token->Progress(progress);
            }
        }

        // Sets the result onto the progress object if it is not empty.
        void Result(ResultT const& result) const
        {
            if (m_token)
            {
                m_token->Result(result);
            }
        }

    private:
        std::unique_ptr<details::AsyncProgressTypeErasure<ResultT, ProgressT>> m_token;
    };
}
