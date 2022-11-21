#include "string_base.h"
#include "string_reference.h"
#include "heap_string.h"

namespace xlang::impl
{
    void string_base::release_base() noexcept
    {
        if (this->is_reference())
        {
            static_cast<string_reference*>(this)->release();
        }
        else
        {
            static_cast<heap_string*>(this)->release();
        }
    }

    string_base* string_base::duplicate_base()
    {
        if (this->is_reference())
        {
            auto str = static_cast<string_reference*>(this);
            // This is a string reference. Create a ref counted string and return it.
            if (is_utf8())
            {
                return heap_string::create(str->get_buffer<xlang_char8>(), str->get_length(), str->get_alternate());
            }
            else
            {
                return heap_string::create(str->get_buffer<char16_t>(), str->get_length(), str->get_alternate());
            }
        }
        else
        {
            static_cast<heap_string*>(this)->addref();
            return this;
        }
    }
}