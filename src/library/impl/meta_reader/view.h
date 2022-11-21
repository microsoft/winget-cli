
namespace xlang::meta::reader
{
    template <typename T>
    auto const& begin(std::pair<T, T> const& values) noexcept
    {
        return values.first;
    }

    template <typename T>
    auto const& end(std::pair<T, T> const& values) noexcept
    {
        return values.second;
    }

    template <typename T>
    auto distance(std::pair<T, T> const& values) noexcept
    {
        return values.second - values.first;
    }

    template <typename Container, typename T>
    auto equal_range(Container const& container, T const& value) noexcept
    {
        return std::equal_range(container.begin(), container.end(), value);
    }

    template <typename Container, typename T, typename Compare>
    auto equal_range(Container const& container, T const& value, Compare compare) noexcept
    {
        return std::equal_range(container.begin(), container.end(), value, compare);
    }

    struct byte_view;
    inline int32_t uncompress_signed(byte_view& cursor, uint32_t length);

    struct byte_view
    {
        byte_view() noexcept = default;
        byte_view(byte_view const&) noexcept = default;
        byte_view& operator=(byte_view const&) noexcept = default;

        byte_view(byte_view&& other) noexcept :
            m_first(std::exchange(other.m_first, {})),
            m_last(std::exchange(other.m_last, {}))
        {
        }

        byte_view& operator=(byte_view&& other) noexcept
        {
            m_first = std::exchange(other.m_first, {});
            m_last = std::exchange(other.m_last, {});
            return *this;
        }

        byte_view(uint8_t const* const first, uint8_t const* const last) noexcept :
            m_first(first),
            m_last(last)
        {
        }

        auto begin() const noexcept
        {
            return m_first;
        }

        auto end() const noexcept
        {
            return m_last;
        }

        uint32_t size() const noexcept
        {
            return static_cast<uint32_t>(end() - begin());
        }

        explicit operator bool() const noexcept
        {
            return size() > 0;
        }

        byte_view seek(uint32_t const offset) const
        {
            check_available(offset);
            return{ m_first + offset, m_last };
        }

        byte_view sub(uint32_t const offset, uint32_t const size) const
        {
            check_available(offset + size);
            return{ m_first + offset, m_first + offset + size };
        }

        template <typename T>
        T const& as(uint32_t const offset = 0) const
        {
            check_available(offset + sizeof(T));
            return reinterpret_cast<T const&>(*(m_first + offset));
        }

        std::string_view as_string(uint32_t const offset = 0) const
        {
            static_assert(sizeof(uint8_t) == 1);
            check_available(offset + 1);
            auto const length = as<uint8_t>(offset);
            if (length == 0)
            {
                return "";
            }
            else if (length == 0xff)
            {
                return { nullptr, 0 };
            }
            else
            {
                check_available(offset + 1 + length);
                return { reinterpret_cast<char const*>(m_first + offset + 1), length };
            }
        }

        template <typename T>
        auto as_array(uint32_t const offset, uint32_t const count) const
        {
            check_available(offset + count * sizeof(T));
            return reinterpret_cast<T const*>(m_first + offset);
        }

    private:

        void check_available(uint32_t const offset) const
        {
            if (m_first + offset > m_last)
            {
                throw_invalid("Buffer too small");
            }
        }

        uint8_t const* m_first{};
        uint8_t const* m_last{};
    };

    struct file_view : byte_view
    {
        file_view(file_view const&) = delete;
        file_view& operator=(file_view const&) = delete;
        file_view(file_view&&) noexcept = default;
        file_view& operator=(file_view&&) noexcept = default;

        file_view(std::string_view const& path) : byte_view{ open_file(path) }, m_backed_by_file{ true }
        {
        }

        file_view(uint8_t const* const first, uint8_t const* const last) noexcept : byte_view{ first, last }, m_backed_by_file{ false }
        {
        }

        ~file_view() noexcept
        {
            if (m_backed_by_file)
            {
#if XLANG_PLATFORM_WINDOWS
                UnmapViewOfFile(begin());
#else
                munmap(const_cast<void*>(reinterpret_cast<void const*>(begin())), size());
#endif
            }
        }

    private:

        bool m_backed_by_file;

#if XLANG_PLATFORM_WINDOWS
        struct handle
        {
            HANDLE value{};

            ~handle() noexcept
            {
                if (value)
                {
                    CloseHandle(value);
                }
            }

            explicit operator bool() const noexcept
            {
                return value != 0;
            }
        };
#endif

        struct file_handle
        {
#if XLANG_PLATFORM_WINDOWS
            using handle_type = HANDLE;
#else
            using handle_type = int;
            static constexpr handle_type INVALID_HANDLE_VALUE = -1;
#endif

            handle_type value{ INVALID_HANDLE_VALUE };

            ~file_handle() noexcept
            {
                if (value != INVALID_HANDLE_VALUE)
                {
#if XLANG_PLATFORM_WINDOWS
                    CloseHandle(value);
#else
                    close(value);
#endif
                }
            }

            explicit operator bool() const noexcept
            {
                return value != INVALID_HANDLE_VALUE;
            }
        };

        static byte_view open_file(std::string_view const& path)
        {
#if XLANG_PLATFORM_WINDOWS
            auto input = c_str(path);

            auto const input_length = static_cast<uint32_t>(path.length() + 1);
            int buffer_length = MultiByteToWideChar(CP_UTF8, 0, input, input_length, 0, 0);
            std::vector<wchar_t> output = std::vector<wchar_t>(buffer_length);
            int result = MultiByteToWideChar(CP_UTF8, 0, input, input_length, output.data(), buffer_length);

            if (result == 0)
            {
                switch (GetLastError())
                {
                case ERROR_INSUFFICIENT_BUFFER:
                    throw_invalid("Insufficient buffer size");
                case ERROR_NO_UNICODE_TRANSLATION:
                    throw_invalid("Untranslatable path");
                default:
                    throw_invalid("Could not convert path");
                }
            }

            file_handle file{ CreateFile2(output.data(), GENERIC_READ, FILE_SHARE_READ, OPEN_EXISTING, nullptr) };

            if (!file)
            {
                throw_invalid("Could not open file '", path, "'");
            }

            LARGE_INTEGER size{};
            GetFileSizeEx(file.value, &size);

            if (!size.QuadPart)
            {
                return{};
            }

            handle mapping{ CreateFileMappingW(file.value, nullptr, PAGE_READONLY, 0, 0, nullptr) };

            if (!mapping)
            {
                throw_invalid("Could not open file '", path, "'");
            }

            auto const first{ static_cast<uint8_t const*>(MapViewOfFile(mapping.value, FILE_MAP_READ, 0, 0, 0)) };
            return{ first, first + size.QuadPart };
#else
            file_handle file{ open(c_str(path), O_RDONLY, 0) };
            if (!file)
            {
                throw_invalid("Could not open file '", path, "'");
            }

            struct stat st;
            int ret = fstat(file.value, &st);
            if (ret < 0)
            {
                throw_invalid("Could not open file '", path, "'");
            }
            if (!st.st_size)
            {
                return{};
            }

            auto const first = static_cast<uint8_t const*>(mmap(nullptr, st.st_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, file.value, 0));
            if (first == MAP_FAILED)
            {
                throw_invalid("Could not open file '", path, "'");
            }

            return{ first, first + st.st_size };
#endif
        }
    };
}
