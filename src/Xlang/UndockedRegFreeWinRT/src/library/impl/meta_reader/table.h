
namespace xlang::meta::reader
{
    struct database;
    struct cache;

    struct table_base
    {
        explicit table_base(database const* database) noexcept : m_database(database)
        {
        }

        database const& get_database() const noexcept
        {
            return *m_database;
        }

        uint32_t size() const noexcept
        {
            return m_row_count;
        }

        uint32_t row_size() const noexcept
        {
            return m_row_size;
        }

        uint32_t column_size(uint32_t const column) const noexcept
        {
            return m_columns[column].size;
        }

        template <typename T>
        T get_value(uint32_t const row, uint32_t const column) const
        {
            static_assert(std::is_enum_v<T> || std::is_integral_v<T>);
            uint32_t const data_size = m_columns[column].size;
            XLANG_ASSERT(data_size == 1 || data_size == 2 || data_size == 4 || data_size == 8);
            XLANG_ASSERT(data_size <= sizeof(T));

            if (row > size())
            {
                throw_invalid("Invalid row index");
            }

            uint8_t const* ptr = m_data + row * m_row_size + m_columns[column].offset;
            switch (data_size)
            {
            case  1:
            {
                uint8_t temp = *ptr;
                return static_cast<T>(temp);
            }
            case 2:
            {
                uint16_t temp = *reinterpret_cast<uint16_t const*>(ptr);
                return static_cast<T>(temp);
            }
            case 4:
            {
                uint32_t temp = *reinterpret_cast<uint32_t const*>(ptr);
                return static_cast<T>(temp);
            }
            default:
            {
                uint64_t temp = *reinterpret_cast<uint64_t const*>(ptr);
                return static_cast<T>(temp);
            }
            }
        }

    private:

        friend database;

        struct column
        {
            uint8_t offset;
            uint8_t size;
        };

        database const* m_database;
        uint8_t const* m_data{};
        uint32_t m_row_count{};
        uint8_t m_row_size{};
        std::array<column, 6> m_columns{};

        void set_row_count(uint32_t const row_count) noexcept
        {
            XLANG_ASSERT(!m_row_count);
            m_row_count = row_count;
        }

        void set_columns(uint8_t const a, uint8_t const b = 0, uint8_t const c = 0, uint8_t const d = 0, uint8_t const e = 0, uint8_t const f = 0) noexcept
        {
            XLANG_ASSERT(a);
            XLANG_ASSERT(a <= 8);
            XLANG_ASSERT(b <= 8);
            XLANG_ASSERT(c <= 8);
            XLANG_ASSERT(d <= 8);
            XLANG_ASSERT(e <= 8);
            XLANG_ASSERT(f <= 8);

            XLANG_ASSERT(!m_row_size);
            m_row_size = a + b + c + d + e + f;
            XLANG_ASSERT(m_row_size < UINT8_MAX);

            m_columns[0] = { 0, a };
            if (b) { m_columns[1] = { static_cast<uint8_t>(a), b }; }
            if (c) { m_columns[2] = { static_cast<uint8_t>(a + b), c }; }
            if (d) { m_columns[3] = { static_cast<uint8_t>(a + b + c), d }; }
            if (e) { m_columns[4] = { static_cast<uint8_t>(a + b + c + d), e }; }
            if (f) { m_columns[5] = { static_cast<uint8_t>(a + b + c + d + e), f }; }
        }

        void set_data(byte_view& view) noexcept
        {
            XLANG_ASSERT(!m_data);

            if (m_row_count)
            {
                XLANG_ASSERT(m_row_size);
                m_data = view.begin();
                view = view.seek(m_row_count * m_row_size);
            }
        }

        uint8_t index_size() const noexcept
        {
            return m_row_count < (1 << 16) ? 2 : 4;
        }
    };

    template <typename T>
    struct index_base
    {
        index_base() noexcept = default;

        index_base(table_base const* const table, T const type, uint32_t const row) noexcept :
            m_table{ table },
            m_value{ ((row + 1) << coded_index_bits_v<T>) | static_cast<uint32_t>(type) }
        {
        }

        index_base(table_base const* const table, uint32_t const value) noexcept :
            m_table{ table },
            m_value{ value }
        {
        }

        explicit operator bool() const noexcept
        {
            return m_value != 0;
        }

        T type() const noexcept
        {
            return static_cast<T>(m_value & ((1 << coded_index_bits_v<T>) - 1));
        }

        uint32_t index() const noexcept
        {
            return (m_value >> coded_index_bits_v<T>) - 1;
        }

        template <typename Row>
        Row get_row() const;

        bool operator==(index_base const& other) const noexcept
        {
            return m_value == other.m_value;
        }

        bool operator!=(index_base const& other) const noexcept
        {
            return !(*this == other);
        }

        bool operator<(index_base const& other) const noexcept
        {
            return m_value < other.m_value;
        }

        database const& get_database() const noexcept
        {
            return m_table->get_database();
        }

    protected:

        table_base const* m_table{};
        uint32_t m_value{};
    };

    template <typename T>
    struct typed_index : index_base<T>
    {
        using index_base<T>::index_base;
    };

    template <> struct typed_index<MemberRefParent> : index_base<MemberRefParent>
    {
        using index_base<MemberRefParent>::index_base;

        auto TypeRef() const;
        auto TypeDef() const;
    };

    template <typename T>
    struct coded_index : typed_index<T>
    {
        coded_index() noexcept = default;

        coded_index(table_base const* const table, T const type, uint32_t const row) noexcept :
            typed_index<T>{ table, type, row }
        {
        }

        coded_index(table_base const* const table, uint32_t const value) noexcept :
            typed_index<T>{ table, value }
        {
        }
    };

    template <typename Row>
    struct row_base
    {
        using iterator_category = std::random_access_iterator_tag;
        using value_type = Row;
        using difference_type = int32_t;
        using pointer = value_type*;
        using reference = value_type&;
        using const_reference = value_type const&;

        row_base(table_base const* const table, uint32_t const index) noexcept : m_table(table), m_index(index)
        {
        }

        uint32_t index() const noexcept
        {
            return m_index;
        }

        template <typename T>
        auto coded_index() const noexcept
        {
            return reader::coded_index{ m_table, index_tag_v<T, Row>, index() };
        }

        template <typename T>
        T get_value(uint32_t const column) const
        {
            XLANG_ASSERT(*this);
            return m_table->get_value<T>(m_index, column);
        }

        template <typename T>
        auto get_list(uint32_t const column) const;

        template <typename T>
        auto get_target_row(uint32_t const column) const;

        template <typename T, uint32_t ParentColumn>
        auto get_parent_row() const;

        database const& get_database() const noexcept
        {
            return m_table->get_database();
        }

        cache const& get_cache() const noexcept
        {
            return get_database().get_cache();
        }

        reference operator++() noexcept
        {
            ++m_index;
            return static_cast<reference>(*this);
        }

        value_type operator++(int) noexcept
        {
            row_base temp{ *this };
            operator++();
            return temp;
        }

        reference operator--() noexcept
        {
            --m_index;
            return *this;
        }

        value_type operator--(int) noexcept
        {
            row_base temp{ *this };
            operator--();
            return temp;
        }

        reference operator+=(difference_type offset) noexcept
        {
            m_index += offset;
            return static_cast<reference>(*this);
        }

        value_type operator+(difference_type offset) const noexcept
        {
            return { m_table, m_index + offset };
        }

        reference operator-=(difference_type offset) noexcept
        {
            return *this += -offset;
        }

        value_type operator-(difference_type offset) const noexcept
        {
            return *this + -offset;
        }

        difference_type operator-(row_base const& other) const noexcept
        {
            XLANG_ASSERT(m_table == other.m_table);
            return m_index - other.m_index;
        }

        value_type operator[](difference_type offset) const noexcept
        {
            return { m_table, m_index + offset };
        }

        bool operator==(row_base const& other) const noexcept
        {
            return (m_table == other.m_table) && (m_index == other.m_index);
        }

        bool operator!=(row_base const& other) const noexcept
        {
            return !(*this == other);
        }

        bool operator<(row_base const& other) const noexcept
        {
            return m_table < other.m_table || (!(other.m_table < m_table) && m_index < other.m_index);
        }

        bool operator>(row_base const& other) const noexcept
        {
            return other < *this;
        }

        bool operator<=(row_base const& other) const noexcept
        {
            return !(other < *this);
        }

        bool operator>=(row_base const& other) const noexcept
        {
            return !(*this < other);
        }

        [[nodiscard]] const_reference operator*() const noexcept
        {
            return static_cast<const_reference>(*this);
        }

        explicit operator bool() const noexcept
        {
            return m_table != nullptr;
        }

    protected:

        row_base() noexcept = default;

        std::string_view get_string(uint32_t const column) const;
        byte_view get_blob(uint32_t const column) const;

        template <typename T>
        auto get_coded_index(uint32_t const column) const
        {
            return reader::coded_index<T>{ m_table, m_table->get_value<uint32_t>(m_index, column) };
        }

        table_base const* get_table() const noexcept
        {
            return m_table;
        }

    private:

        table_base const* m_table{};
        uint32_t m_index{};
    };

    template <typename T>
    struct table : table_base
    {
        explicit table(database const* const database) noexcept : table_base{ database }
        {
        }

        T begin() const noexcept
        {
            return { this, 0 };
        }

        T end() const noexcept
        {
            return { this, size() };
        }

        T operator[](uint32_t const row) const noexcept
        {
            return { this, row };
        }
    };
}
