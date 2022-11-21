#include "pal_internal.h"
#include "pal_error.h"
#include "atomic_ref_count.h"
#include <xlang/base.h>

namespace xlang::impl
{
    struct error_info : xlang_error_info
    {
        // Used to construct constant errors used in out of memory scenarios.
        explicit error_info(xlang_result result) noexcept :
            m_result{ result },
            m_modifiable{ false }
        {
        }

        explicit error_info(
            xlang_result result,
            xlang_string message,
            xlang_string projection_identifier,
            xlang_string language_error,
            xlang_unknown* execution_trace,
            xlang_unknown* language_information
        ) noexcept :
            m_result{ result }
        {   
            m_execution_trace.copy_from(execution_trace);
            m_language_information.copy_from(language_information);

            try
            {
                copy_from_abi(m_message, message);
                copy_from_abi(m_projection_identifier, projection_identifier);
                copy_from_abi(m_language_error, language_error);
            }
            catch (...)
            {
                // Even if any of the string copies fail, we still have a xlang_result to represent the error.
            }
        }

        com_interop_result XLANG_CALL QueryInterface(xlang_guid const& id, void** object) noexcept final
        {
            if (id == xlang_unknown_guid)
            {
                static_assert(std::is_base_of_v<xlang_unknown, xlang_error_info>, "Can only combine these two cases if this is true.");
                *object = static_cast<xlang_unknown*>(this);
            }
            else if (id == xlang_error_info_guid)
            {
                *object = static_cast<xlang_error_info*>(this);
            }
            else
            {
                *object = nullptr;
                return com_interop_result::no_interface;
            }
            AddRef();
            return com_interop_result::success;
        }

        uint32_t XLANG_CALL AddRef() noexcept final
        {
            return ++m_count;
        }

        uint32_t XLANG_CALL Release() noexcept final
        {
            auto result = --m_count;
            if (result == 0)
            {
                delete this;
            }
            return result;
        }

        void GetError(xlang_result* error) noexcept override
        {
            *error = m_result;
        }

        void GetMessage(xlang_string* message) noexcept override
        {
            *message = nullptr;
            copy_to_abi(m_message, *message);
        }

        void GetLanguageError(xlang_string* language_error) noexcept override
        {
            *language_error = nullptr;
            copy_to_abi(m_language_error, *language_error);
        }

        void GetExecutionTrace(xlang_unknown** execution_trace) noexcept override
        {
            m_execution_trace.copy_to(execution_trace);
        }

        void GetProjectionIdentifier(xlang_string* projection_identifier) noexcept override
        {
            *projection_identifier = nullptr;
            copy_to_abi(m_projection_identifier, *projection_identifier);
        }

        void GetLanguageInformation(xlang_unknown** language_information) noexcept override
        {
            m_language_information.copy_to(language_information);
        }

        void GetPropagatedError(xlang_error_info** propagated_error) noexcept override
        {
            m_next_propagated_error.copy_to(propagated_error);
        }

        void PropagateError(
            xlang_string projection_identifier,
            xlang_string language_error,
            xlang_unknown* execution_trace,
            xlang_unknown* language_information
        ) noexcept override
        {
            // If the error info can't be modified, don't collect infomration on propagations
            // as this error info can be reused for another error.
            if (!m_modifiable)
            {
                return;
            }

            com_ptr<xlang_error_info> propagated_error;
            propagated_error.attach(
                xlang_originate_error(
                    m_result,
                    get_abi(m_message),
                    projection_identifier,
                    language_error,
                    execution_trace,
                    language_information));

            error_info* last_propagated_error = this;
            while (last_propagated_error->m_next_propagated_error != nullptr)
            {
                last_propagated_error = static_cast<error_info*>(last_propagated_error->m_next_propagated_error.get());
            }
            last_propagated_error->m_next_propagated_error = propagated_error;
        }

    private:
        xlang_result m_result{};
        hstring m_message{};
        hstring m_language_error{};
        com_ptr<xlang_unknown> m_execution_trace;
        hstring m_projection_identifier{};
        com_ptr<xlang_unknown> m_language_information;
        com_ptr<xlang_error_info> m_next_propagated_error;
        bool m_modifiable{ true };
        atomic_ref_count m_count;
    };

    error_info error_code_errors [] = {
        error_info {xlang_result::access_denied},
        error_info {xlang_result::bounds},
        error_info {xlang_result::fail},
        error_info {xlang_result::handle},
        error_info {xlang_result::invalid_arg},
        error_info {xlang_result::invalid_state},
        error_info {xlang_result::no_interface},
        error_info {xlang_result::not_impl},
        error_info {xlang_result::out_of_memory},
        error_info {xlang_result::pointer},
        error_info {xlang_result::type_load}
    };
}

[[nodiscard]] XLANG_PAL_EXPORT xlang_error_info* XLANG_CALL xlang_originate_error(
    xlang_result error,
    xlang_string message,
    xlang_string projection_identifier,
    xlang_string language_error,
    xlang_unknown* execution_trace,
    xlang_unknown* language_information
) XLANG_NOEXCEPT
{
    xlang_error_info* error_info =
        new (std::nothrow) xlang::impl::error_info
    {
        error,
        message,
        projection_identifier,
        language_error,
        execution_trace,
        language_information
    };

    // If failed to construct, use the statically allocated ones.
    if (error_info == nullptr)
    {
        error_info = &xlang::impl::error_code_errors[static_cast<int>(error)];
        error_info->AddRef();
    }

    return error_info;
}