#define CPPXLANG_VERSION "%"

#ifdef _WIN32
extern "C"
__declspec(selectany) char const* const cppxlang_projection_identifier = "cppxlang_" CPPXLANG_VERSION;
#else
extern "C"
inline char const* const cppxlang_projection_identifier = "cppxlang_" CPPXLANG_VERSION;
#endif

namespace xlang::impl
{
#ifdef _WIN32

    constexpr int32_t hresult_from_win32(uint32_t const x) noexcept
    {
        return (int32_t)(x) <= 0 ? (int32_t)(x) : (int32_t)(((x) & 0x0000FFFF) | (7 << 16) | 0x80000000);
    }

    constexpr xlang_result xlang_result_from_hresult(HRESULT const result)
    {
        switch (result)
        {
        case S_OK:
            return xlang_result::success;
        case E_ACCESSDENIED:
            return xlang_result::access_denied;
        case E_BOUNDS:
            return xlang_result::bounds;
        case E_FAIL:
            return xlang_result::fail;
        case E_HANDLE:
            return xlang_result::handle;
        case E_INVALIDARG:
            return xlang_result::invalid_arg;
        case E_ILLEGAL_STATE_CHANGE:
            return xlang_result::invalid_state;
        case E_NOINTERFACE:
            return xlang_result::no_interface;
        case E_NOTIMPL:
            return xlang_result::not_impl;
        case E_OUTOFMEMORY:
            return xlang_result::out_of_memory;
        case E_POINTER:
            return xlang_result::pointer;
        case REGDB_E_CLASSNOTREG:
        case static_cast<HRESULT>(0x80131522):
            return xlang_result::type_load;
        }

        return xlang_result::fail;
    }
#endif

    constexpr xlang_result xlang_result_from_com(com_interop_result const result)
    {
        switch (result)
        {
        case com_interop_result::success:
            return xlang_result::success;
        case com_interop_result::no_interface:
            return xlang_result::no_interface;
        case com_interop_result::pointer:
            return xlang_result::pointer;
        }

#ifdef _WIN32
        return xlang_result_from_hresult(static_cast<HRESULT>(result));
#else
        return xlang_result::fail;
#endif
    }
}

namespace xlang
{
    struct xlang_error
    {
        xlang_error() noexcept = default;
        xlang_error(xlang_error&&) = default;
        xlang_error& operator=(xlang_error&&) = default;

        xlang_error(xlang_error const& other) noexcept :
            m_code(other.m_code)
        {
        }

        xlang_error& operator=(xlang_error const& other) noexcept
        {
            m_code = other.m_code;
            return *this;
        }

        explicit xlang_error(xlang_result const code) noexcept : m_code(code)
        {
            originate(code, nullptr);
        }

        xlang_error(xlang_result const code, param::hstring const& message) noexcept : m_code(code)
        {
            originate(code, get_abi(message));
        }

        xlang_error(xlang_result const code, xlang_error_info* error_info) noexcept : m_code(code)
        {
            if (error_info == nullptr)
            {
                originate(code, nullptr);
            }
            else
            {
                m_error_info.copy_from(error_info);
                m_error_info->PropagateError(get_abi(m_projection_identifer), nullptr, nullptr, nullptr);
            }
        }

        xlang_result code() const noexcept
        {
            return m_code;
        }

        hstring message() const noexcept
        {
            hstring message;
            if (m_error_info)
            {
                m_error_info->GetMessage(put_abi(message));
            }

            return message;
        }

        xlang_error_info* to_abi() const noexcept
        {
            return com_ptr<xlang_error_info>(m_error_info).detach();
        }

    private:

        void originate(xlang_result const code, xlang_string message) noexcept
        {
            m_error_info.attach(xlang_originate_error(code, message, get_abi(m_projection_identifer)));
        }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

        uint32_t const m_debug_magic{ 0xAABBCCDD };
        xlang_result m_code{ xlang_result::fail };
        com_ptr<xlang_error_info> m_error_info;
        hstring m_projection_identifer{ cppxlang_projection_identifier };

#ifdef __clang__
#pragma clang diagnostic pop
#endif
    };

    struct access_denied_error : xlang_error
    {
        access_denied_error() noexcept : xlang_error(xlang_result::access_denied) {}
        access_denied_error(param::hstring const& message) noexcept : xlang_error(xlang_result::access_denied, message) {}
        access_denied_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::access_denied, error_info) {}
    };

    struct out_of_bounds_error : xlang_error
    {
        out_of_bounds_error() noexcept : xlang_error(xlang_result::bounds) {}
        out_of_bounds_error(param::hstring const& message) noexcept : xlang_error(xlang_result::bounds, message) {}
        out_of_bounds_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::bounds, error_info) {}
    };

    struct invalid_handle_error : xlang_error
    {
        invalid_handle_error() noexcept : xlang_error(xlang_result::handle) {}
        invalid_handle_error(param::hstring const& message) noexcept : xlang_error(xlang_result::handle, message) {}
        invalid_handle_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::handle, error_info) {}
    };

    struct invalid_argument_error : xlang_error
    {
        invalid_argument_error() noexcept : xlang_error(xlang_result::invalid_arg) {}
        invalid_argument_error(param::hstring const& message) noexcept : xlang_error(xlang_result::invalid_arg, message) {}
        invalid_argument_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::invalid_arg, error_info) {}
    };

    struct invalid_state_error : xlang_error
    {
        invalid_state_error() noexcept : xlang_error(xlang_result::invalid_state) {}
        invalid_state_error(param::hstring const& message) noexcept : xlang_error(xlang_result::invalid_state, message) {}
        invalid_state_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::invalid_state, error_info) {}
    };

    struct no_interface_error : xlang_error
    {
        no_interface_error() noexcept : xlang_error(xlang_result::no_interface) {}
        no_interface_error(param::hstring const& message) noexcept : xlang_error(xlang_result::no_interface, message) {}
        no_interface_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::no_interface, error_info) {}
    };

    struct not_implemented_error : xlang_error
    {
        not_implemented_error() noexcept : xlang_error(xlang_result::not_impl) {}
        not_implemented_error(param::hstring const& message) noexcept : xlang_error(xlang_result::not_impl, message) {}
        not_implemented_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::not_impl, error_info) {}
    };

    struct pointer_error : xlang_error
    {
        pointer_error() noexcept : xlang_error(xlang_result::pointer) {}
        pointer_error(param::hstring const& message) noexcept : xlang_error(xlang_result::pointer, message) {}
        pointer_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::pointer, error_info) {}
    };

    struct type_load_error : xlang_error
    {
        type_load_error() noexcept : xlang_error(xlang_result::type_load) {}
        type_load_error(param::hstring const& message) noexcept : xlang_error(xlang_result::type_load, message) {}
        type_load_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::type_load, error_info) {}
    };

    struct cancelled_error : xlang_error
    {
        cancelled_error() noexcept : xlang_error(xlang_result::fail) {}
        cancelled_error(param::hstring const& message) noexcept : xlang_error(xlang_result::fail, message) {}
        cancelled_error(xlang_error_info* error_info) noexcept : xlang_error(xlang_result::fail, error_info) {}
    };

    [[noreturn]] inline XLANG_NOINLINE void throw_xlang_error(xlang_result const result, xlang_error_info* error_info = nullptr)
    {
        XLANG_ASSERT(result != xlang_result::success);

        if (result == xlang_result::out_of_memory)
        {
            throw std::bad_alloc();
        }

        if (result == xlang_result::access_denied)
        {
            throw access_denied_error(error_info);
        }

        if (result == xlang_result::bounds)
        {
            throw out_of_bounds_error(error_info);
        }

        if (result == xlang_result::handle)
        {
            throw invalid_handle_error(error_info);
        }

        if (result == xlang_result::invalid_arg)
        {
            throw invalid_argument_error(error_info);
        }

        if (result == xlang_result::invalid_state)
        {
            throw invalid_state_error(error_info);
        }

        if (result == xlang_result::no_interface)
        {
            throw no_interface_error(error_info);
        }

        if (result == xlang_result::not_impl)
        {
            throw not_implemented_error(error_info);
        }

        if (result == xlang_result::pointer)
        {
            throw pointer_error(error_info);
        }

        if (result == xlang_result::type_load)
        {
            throw type_load_error(error_info);
        }

        throw xlang_error(result, error_info);
    }

    inline XLANG_NOINLINE xlang_error_info* to_xlang_error() noexcept
    {
        try
        {
            throw;
        }
        catch (xlang_error const& e)
        {
            return e.to_abi();
        }
        XLANG_EXTERNAL_CATCH_CLAUSE
        catch (std::bad_alloc const& e)
        {
            return xlang_error(xlang_result::out_of_memory, to_hstring(e.what())).to_abi();
        }
        catch (std::out_of_range const& e)
        {
            return out_of_bounds_error(to_hstring(e.what())).to_abi();
        }
        catch (std::invalid_argument const& e)
        {
            return invalid_argument_error(to_hstring(e.what())).to_abi();
        }
        catch (std::exception const& e)
        {
            return xlang_error(xlang_result::fail, to_hstring(e.what())).to_abi();
        }
        catch (...)
        {
            std::terminate();
        }
    }

    inline void check_xlang_error(xlang_error_info* result)
    {
        if (result != nullptr)
        {
            com_ptr<xlang_error_info> error_info{ result, take_ownership_from_abi };
            xlang_result error;
            error_info->GetError(&error);
            throw_xlang_error(error, error_info.get());
        }
    }

    inline void check_com_interop_error(com_interop_result result)
    {
        if (result != com_interop_result::success)
        {
            throw_xlang_error(impl::xlang_result_from_com(result), nullptr);
        }
    }

#ifdef _WIN32

    template<typename T>
    void check_hresult(T result)
    {
        if (result != 0)
        {
            throw_xlang_error(impl::xlang_result_from_hresult(result));
        }
    }

    [[noreturn]] inline void throw_last_error()
    {
        throw_xlang_error(impl::xlang_result_from_hresult(HRESULT_FROM_WIN32(XLANG_GetLastError())));
    }

    template<typename T>
    T* check_pointer(T* pointer)
    {
        if (!pointer)
        {
            throw_last_error();
        }

        return pointer;
    }
#endif
}
