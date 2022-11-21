#include "iwidget.h"
#include <pal.h>
#include <atomic_ref_count.h>
#include <string_view>

struct widget : iwidget
{
    com_interop_result XLANG_CALL QueryInterface(xlang_guid const& id, void** object) noexcept final
    {
        *object = nullptr;

        if (id == xlang_unknown_guid)
        {
            static_assert(std::is_base_of_v<xlang_unknown, iwidget>, "Can only combine these two cases if this is true.");
            *object = static_cast<xlang_unknown*>(this);
        }
        else if (id == iwidget_guid)
        {
            *object = static_cast<iwidget*>(this);
        }
        else
        {
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

    xlang_error_info* get_answer(int32_t* answer) noexcept override
    {
        *answer = 42;
        return nullptr; // xlang_error_ok;
    }

    private:
        xlang::impl::atomic_ref_count m_count;
};

struct widget_factory : iwidget_factory
{
    com_interop_result XLANG_CALL QueryInterface(xlang_guid const& id, void** object) noexcept final
    {
        *object = nullptr;

        if (id == xlang_unknown_guid)
        {
            static_assert(std::is_base_of_v<xlang_unknown, iwidget_factory>, "Can only combine these two cases if this is true.");
            *object = static_cast<xlang_unknown*>(this);
        }
        else if (id == iwidget_factory_guid)
        {
            *object = static_cast<iwidget_factory*>(this);
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

    xlang_error_info* activate_widget(iwidget** instance) noexcept override
    {
        *instance = new (std::nothrow) widget{};
        if (!*instance)
        {
            return xlang_originate_error(xlang_result::out_of_memory);
        }

        return nullptr; // xlang_error_ok;

    }

    private:
        xlang::impl::atomic_ref_count m_count;
};

extern "C" xlang_error_info* XLANG_CALL xlang_lib_get_activation_factory(xlang_string class_name, xlang_guid const& iid, void** factory) noexcept
{
    *factory = nullptr;

    char16_t const* buffer_ref{};
    uint32_t length_ref{};
    xlang_error_info* result{};
    result = xlang_get_string_raw_buffer_utf16(class_name, &buffer_ref, &length_ref);
    if (result != nullptr)
    {
        return result;
    }

    std::u16string_view const name{ buffer_ref, length_ref };
    if (name == u"AbiComponent.Widget")
    {
        if (iid == xlang_unknown_guid || iid == iwidget_factory_guid)
        {
            *factory = new (std::nothrow) widget_factory();

            if (!*factory)
            {
                return xlang_originate_error(xlang_result::out_of_memory);
            }

            return nullptr; // xlang_error_ok;
        }

        return xlang_originate_error(xlang_result::no_interface);
    }

    return xlang_originate_error(xlang_result::type_load);
}