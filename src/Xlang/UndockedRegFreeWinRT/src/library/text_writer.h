#pragma once

#include "impl/base.h"

namespace xlang::text
{
    template <typename T>
    struct writer_base
    {
        writer_base(writer_base const&) = delete;
        writer_base& operator=(writer_base const&) = delete;

        writer_base()
        {
            m_first.reserve(16 * 1024);
        }

        template <typename... Args>
        void write(std::string_view const& value, Args const&... args)
        {
#if defined(XLANG_DEBUG)
            auto expected = count_placeholders(value);
            auto actual = sizeof...(Args);
            XLANG_ASSERT(expected == actual);
#endif
            write_segment(value, args...);
        }

        template <typename... Args>
        std::string write_temp(std::string_view const& value, Args const&... args)
        {
#if defined(XLANG_DEBUG)
            bool restore_debug_trace = debug_trace;
            debug_trace = false;
#endif
            auto const size = m_first.size();

            XLANG_ASSERT(count_placeholders(value) == sizeof...(Args));
            write_segment(value, args...);

            std::string result{ m_first.data() + size, m_first.size() - size };
            m_first.resize(size);

#if defined(XLANG_DEBUG)
            debug_trace = restore_debug_trace;
#endif
            return result;
        }

        void write_impl(std::string_view const& value)
        {
            m_first.insert(m_first.end(), value.begin(), value.end());

#if defined(XLANG_DEBUG)
            if (debug_trace)
            {
                ::printf("%.*s", static_cast<int>(value.size()), value.data());
            }
#endif
        }

        void write_impl(char const value)
        {
            m_first.push_back(value);

#if defined(XLANG_DEBUG)
            if (debug_trace)
            {
                ::printf("%c", value);
            }
#endif
        }

        void write(std::string_view const& value)
        {
            static_cast<T*>(this)->write_impl(value);
        }

        void write(char const value)
        {
            static_cast<T*>(this)->write_impl(value);
        }

        void write_code(std::string_view const& value)
        {
            write(value);
        }

        template <typename F, typename = std::enable_if_t<std::is_invocable_v<F, T&>>>
        void write(F const& f)
        {
            f(*static_cast<T*>(this));
        }

        void write(int32_t const value)
        {
            write(std::to_string(value));
        }

        void write(uint32_t const value)
        {
            write(std::to_string(value));
        }

        void write(int64_t const value)
        {
            write(std::to_string(value));
        }

        void write(uint64_t const value)
        {
            write(std::to_string(value));
        }

        template <typename... Args>
        void write_printf(char const* format, Args const&... args)
        {
            char buffer[128];
#if XLANG_PLATFORM_WINDOWS
            size_t const size = sprintf_s(buffer, format, args...);
#else
            size_t const size = snprintf(buffer, sizeof(buffer), format, args...);
#endif
            write(std::string_view{ buffer, size });
        }

        template <auto F, typename List, typename... Args>
        void write_each(List const& list, Args const&... args)
        {
            for (auto&& item : list)
            {
                F(*static_cast<T*>(this), item, args...);
            }
        }

        void swap() noexcept
        {
            std::swap(m_second, m_first);
        }

        void flush_to_console() noexcept
        {
            printf("%.*s", static_cast<int>(m_first.size()), m_first.data());
            printf("%.*s", static_cast<int>(m_second.size()), m_second.data());
            m_first.clear();
            m_second.clear();
        }

        void flush_to_file(std::string const& filename)
        {
            if (!file_equal(filename))
            {
                std::ofstream file{ filename, std::ios::out | std::ios::binary };
                file.write(m_first.data(), m_first.size());
                file.write(m_second.data(), m_second.size());
            }
            m_first.clear();
            m_second.clear();
        }

        void flush_to_file(std::filesystem::path const& filename)
        {
            flush_to_file(filename.string());
        }

        std::string flush_to_string()
        {
            std::string result;
            result.reserve(m_first.size() + m_second.size());
            result.assign(m_first.begin(), m_first.end());
            result.append(m_second.begin(), m_second.end());
            m_first.clear();
            m_second.clear();
            return result;
        }

        char back()
        {
            return m_first.empty() ? char{} : m_first.back();
        }

        bool file_equal(std::string const& filename) const
        {
            if (!std::filesystem::exists(filename))
            {
                return false;
            }

            meta::reader::file_view file{ filename };

            if (file.size() != m_first.size() + m_second.size())
            {
                return false;
            }

            if (!std::equal(m_first.begin(), m_first.end(), file.begin(), file.begin() + m_first.size()))
            {
                return false;
            }

            return std::equal(m_second.begin(), m_second.end(), file.begin() + m_first.size(), file.end());
        }

#if defined(XLANG_DEBUG)
        bool debug_trace{};
#endif

    private:

        static constexpr uint32_t count_placeholders(std::string_view const& format) noexcept
        {
            uint32_t count{};
            bool escape{};

            for (auto c : format)
            {
                if (!escape)
                {
                    if (c == '^')
                    {
                        escape = true;
                        continue;
                    }

                    if (c == '%' || c == '@')
                    {
                        ++count;
                    }
                }
                escape = false;
            }

            return count;
        }

        void write_segment(std::string_view const& value)
        {
            auto offset = value.find_first_of("^");
            if (offset == std::string_view::npos)
            {
                write(value);
                return;
            }

            write(value.substr(0, offset));

            XLANG_ASSERT(offset != value.size() - 1);

            write(value[offset + 1]);
            write_segment(value.substr(offset + 2));
        }

        template <typename First, typename... Rest>
        void write_segment(std::string_view const& value, First const& first, Rest const&... rest)
        {
            auto offset = value.find_first_of("^%@");
            XLANG_ASSERT(offset != std::string_view::npos);
            write(value.substr(0, offset));

            if (value[offset] == '^')
            {
                XLANG_ASSERT(offset != value.size() - 1);

                write(value[offset + 1]);
                write_segment(value.substr(offset + 2), first, rest...);
            }
            else
            {
                if (value[offset] == '%')
                {
                    static_cast<T*>(this)->write(first);
                }
                else
                {
                    if constexpr (std::is_convertible_v<First, std::string_view>)
                    {
                        static_cast<T*>(this)->write_code(first);
                    }
                    else
                    {
                        XLANG_ASSERT(false); // '@' placeholders are only for text.
                    }
                }

                write_segment(value.substr(offset + 1), rest...);
            }
        }

        std::vector<char> m_second;
        std::vector<char> m_first;
    };


    template <typename T>
    struct indented_writer_base : writer_base<T>
    {
        struct indent_guard
        {
            indent_guard(indented_writer_base<T>& w, int32_t offset = 1) noexcept : m_writer(w), m_offset(offset)
            {
                m_writer.m_indent += m_offset;
            }

            ~indent_guard() noexcept
            {
                m_writer.m_indent -= m_offset;
            }

        private:
            indented_writer_base<T>& m_writer;
            int32_t m_offset{};
        };


        void write_indent()
        {
            for (int32_t i = 0; i < m_indent; i++)
            {
                writer_base<T>::write_impl("    ");
            }
        }

        void write_impl(std::string_view const& value)
        {
            std::string_view::size_type current_pos{ 0 };
            auto on_new_line = writer_base<T>::back() == '\n';

            while (true)
            {
                const auto pos = value.find('\n', current_pos);

                if (pos == std::string_view::npos)
                {
                    if (current_pos < value.size())
                    {
                        if (on_new_line)
                        {
                            write_indent();
                        }

                        writer_base<T>::write_impl(value.substr(current_pos));
                    }

                    return;
                }

                auto current_line = value.substr(current_pos, pos - current_pos + 1);
                auto empty_line = current_line[0] == '\n';

                if (on_new_line && !empty_line)
                {
                    write_indent();
                }

                writer_base<T>::write_impl(current_line);

                on_new_line = true;
                current_pos = pos + 1;
            }
        }

        void write_impl(char const value)
        {
            if (writer_base<T>::back() == '\n' && value != '\n')
            {
                write_indent();
            }

            writer_base<T>::write_impl(value);
        }

        template <typename... Args>
        std::string write_temp(std::string_view const& value, Args const& ... args)
        {
            auto restore_indent = m_indent;
            m_indent = 0;

            auto result = writer_base<T>::write_temp(value, args...);

            m_indent = restore_indent;

            return result;
        }

        int32_t m_indent{};
    };


    template <auto F, typename... Args>
    auto bind(Args&&... args)
    {
        return [&](auto& writer)
        {
            F(writer, args...);
        };
    }

    template <typename F, typename... Args>
    auto bind(F fwrite, Args const&... args)
    {
        return [&, fwrite](auto& writer)
        {
            fwrite(writer, args...);
        };
    }

    template <auto F, typename List, typename... Args>
    auto bind_each(List const& list, Args const&... args)
    {
        return [&](auto& writer)
        {
            for (auto&& item : list)
            {
                F(writer, item, args...);
            }
        };
    }

    template <typename List, typename... Args>
    auto bind_each(List const& list, Args const&... args)
    {
        return [&](auto& writer)
        {
            for (auto&& item : list)
            {
                writer.write(item, args...);
            }
        };
    }

    template <typename F, typename List, typename... Args>
    auto bind_each(F fwrite, List const& list, Args const&... args)
    {
        return [&, fwrite](auto& writer)
        {
            for (auto&& item : list)
            {
                fwrite(writer, item, args...);
            }
        };
    }

    template <auto F, typename T, typename... Args>
    auto bind_list(std::string_view const& delimiter, T const& list, Args const&... args)
    {
        return [&](auto& writer)
        {
            bool first{ true };

            for (auto&& item : list)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    writer.write(delimiter);
                }

                F(writer, item, args...);
            }
        };
    }

    template <typename T>
    auto bind_list(std::string_view const& delimiter, T const& list)
    {
        return [&](auto& writer)
        {
            bool first{ true };

            for (auto&& item : list)
            {
                if (first)
                {
                    first = false;
                }
                else
                {
                    writer.write(delimiter);
                }

                writer.write(item);
            }
        };
    }
}
