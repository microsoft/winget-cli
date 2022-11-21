
namespace xlang
{
    template <typename D, typename T, typename Version = impl::no_collection_version>
    struct iterable_base : Version
    {
        template <typename U>
        static constexpr auto const& wrap_value(U const& value) noexcept
        {
            return value;
        }

        template <typename U>
        static constexpr auto const& unwrap_value(U const& value) noexcept
        {
            return value;
        }

        auto First()
        {
            return make<iterator>(static_cast<D*>(this));
        }

    protected:

        template<typename InputIt, typename Size, typename OutputIt>
        auto copy_n(InputIt first, Size count, OutputIt result) const
        {
            if constexpr (std::is_same_v<T, decltype(*std::declval<D const>().get_container().begin())> && !impl::is_key_value_pair<T>::value)
            {
                std::copy_n(first, count, result);
            }
            else
            {
                return std::transform(first, std::next(first, count), result, [&](auto&& value)
                {
                    if constexpr (!impl::is_key_value_pair<T>::value)
                    {
                        return static_cast<D const&>(*this).unwrap_value(value);
                    }
                    else
                    {
                        return make<impl::key_value_pair<T>>(static_cast<D const&>(*this).unwrap_value(value.first), static_cast<D const&>(*this).unwrap_value(value.second));
                    }
                });
            }
        }

    private:

        struct iterator : Version::iterator_type, implements<iterator, Foundation::Collections::IIterator<T>>
        {
            void abi_enter()
            {
                m_owner->abi_enter();
                this->check_version(*m_owner);
            }

            void abi_exit()
            {
                m_owner->abi_exit();
            }

            explicit iterator(D* const owner) noexcept :
                Version::iterator_type(*owner),
                m_current(owner->get_container().begin()),
                m_end(owner->get_container().end())
            {
                m_owner.copy_from(owner);
            }

            T Current() const
            {
                if (m_current == m_end)
                {
                    throw out_of_bounds_error();
                }

                if constexpr (!impl::is_key_value_pair<T>::value)
                {
                    return m_owner->unwrap_value(*m_current);
                }
                else
                {
                    return make<impl::key_value_pair<T>>(m_owner->unwrap_value(m_current->first), m_owner->unwrap_value(m_current->second));
                }
            }

            bool HasCurrent() const noexcept
            {
                return m_current != m_end;
            }

            bool MoveNext() noexcept
            {
                if (m_current != m_end)
                {
                    ++m_current;
                }

                return HasCurrent();
            }

            uint32_t GetMany(array_view<T> values)
            {
                uint32_t const actual = (std::min)(static_cast<uint32_t>(std::distance(m_current, m_end)), values.size());
                m_owner->copy_n(m_current, actual, values.begin());
                std::advance(m_current, actual);
                return actual;
            }

        private:

            com_ptr<D> m_owner;
            decltype(m_owner->get_container().begin()) m_current;
            decltype(m_owner->get_container().end()) const m_end;
        };
    };

    template <typename D, typename T, typename Version = impl::no_collection_version>
    struct vector_view_base : iterable_base<D, T, Version>
    {
        T GetAt(uint32_t const index) const
        {
            if (index >= Size())
            {
                throw out_of_bounds_error();
            }

            return static_cast<D const&>(*this).unwrap_value(*std::next(static_cast<D const&>(*this).get_container().begin(), index));
        }

        uint32_t Size() const noexcept
        {
            return static_cast<uint32_t>(std::distance(static_cast<D const&>(*this).get_container().begin(), static_cast<D const&>(*this).get_container().end()));
        }

        bool IndexOf(T const& value, uint32_t& index) const noexcept
        {
            auto first = std::find_if(static_cast<D const&>(*this).get_container().begin(), static_cast<D const&>(*this).get_container().end(), [&](auto&& match)
            {
                return value == static_cast<D const&>(*this).unwrap_value(match);
            });

            index = static_cast<uint32_t>(first - static_cast<D const&>(*this).get_container().begin());
            return index < Size();
        }

        uint32_t GetMany(uint32_t const startIndex, array_view<T> values) const
        {
            if (startIndex >= Size())
            {
                return 0;
            }

            uint32_t const actual = (std::min)(Size() - startIndex, values.size());
            this->copy_n(static_cast<D const&>(*this).get_container().begin() + startIndex, actual, values.begin());
            return actual;
        }
    };

    template <typename D, typename T>
    struct vector_base : vector_view_base<D, T, impl::collection_version>
    {
        Foundation::Collections::IVectorView<T> GetView() const noexcept
        {
            return static_cast<D const&>(*this);
        }

        void SetAt(uint32_t const index, T const& value)
        {
            if (index >= static_cast<D const&>(*this).get_container().size())
            {
                throw out_of_bounds_error();
            }

            this->increment_version();
            static_cast<D&>(*this).get_container()[index] = static_cast<D const&>(*this).wrap_value(value);
        }

        void InsertAt(uint32_t const index, T const& value)
        {
            if (index > static_cast<D const&>(*this).get_container().size())
            {
                throw out_of_bounds_error();
            }

            this->increment_version();
            static_cast<D&>(*this).get_container().insert(static_cast<D const&>(*this).get_container().begin() + index, static_cast<D const&>(*this).wrap_value(value));
        }

        void RemoveAt(uint32_t const index)
        {
            if (index >= static_cast<D const&>(*this).get_container().size())
            {
                throw out_of_bounds_error();
            }

            this->increment_version();
            static_cast<D&>(*this).get_container().erase(static_cast<D const&>(*this).get_container().begin() + index);
        }

        void Append(T const& value)
        {
            this->increment_version();
            static_cast<D&>(*this).get_container().push_back(static_cast<D const&>(*this).wrap_value(value));
        }

        void RemoveAtEnd()
        {
            if (static_cast<D const&>(*this).get_container().empty())
            {
                throw out_of_bounds_error();
            }

            this->increment_version();
            static_cast<D&>(*this).get_container().pop_back();
        }

        void Clear() noexcept
        {
            this->increment_version();
            static_cast<D&>(*this).get_container().clear();
        }

        void ReplaceAll(array_view<T const> value)
        {
            this->increment_version();
            assign(value.begin(), value.end());
        }

    private:

        template <typename InputIt>
        void assign(InputIt first, InputIt last)
        {
            using container_type = std::remove_reference_t<decltype(static_cast<D&>(*this).get_container())>;

            if constexpr (std::is_same_v<T, typename container_type::value_type>)
            {
                static_cast<D&>(*this).get_container().assign(first, last);
            }
            else
            {
                auto& container = static_cast<D&>(*this).get_container();
                container.clear();
                container.reserve(std::distance(first, last));

                std::transform(first, last, std::back_inserter(container), [&](auto&& value)
                {
                    return static_cast<D const&>(*this).wrap_value(value);
                });
            }
        }
    };

    template <typename D, typename K, typename V, typename Version = impl::no_collection_version>
    struct map_view_base : iterable_base<D, Foundation::Collections::IKeyValuePair<K, V>, Version>
    {
        V Lookup(K const& key) const
        {
            auto pair = static_cast<D const&>(*this).get_container().find(static_cast<D const&>(*this).wrap_value(key));

            if (pair == static_cast<D const&>(*this).get_container().end())
            {
                throw out_of_bounds_error();
            }

            return static_cast<D const&>(*this).unwrap_value(pair->second);
        }

        uint32_t Size() const noexcept
        {
            return static_cast<uint32_t>(static_cast<D const&>(*this).get_container().size());
        }

        bool HasKey(K const& key) const noexcept
        {
            return static_cast<D const&>(*this).get_container().find(static_cast<D const&>(*this).wrap_value(key)) != static_cast<D const&>(*this).get_container().end();
        }
    };

    template <typename D, typename K, typename V>
    struct map_base : map_view_base<D, K, V, impl::collection_version>
    {
        Foundation::Collections::IMapView<K, V> GetView() const
        {
            return static_cast<D const&>(*this);
        }

        bool Insert(K const& key, V const& value)
        {
            this->increment_version();
            auto pair = static_cast<D&>(*this).get_container().insert_or_assign(static_cast<D const&>(*this).wrap_value(key), static_cast<D const&>(*this).wrap_value(value));
            return !pair.second;
        }

        void Remove(K const& key)
        {
            this->increment_version();
            static_cast<D&>(*this).get_container().erase(static_cast<D const&>(*this).wrap_value(key));
        }

        void Clear() noexcept
        {
            this->increment_version();
            static_cast<D&>(*this).get_container().clear();
        }
    };
}
