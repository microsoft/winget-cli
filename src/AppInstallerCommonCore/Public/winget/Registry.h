// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#pragma once
#include <wil/resource.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>


namespace AppInstaller::Registry
{
    namespace details
    {
        template <DWORD Type>
        struct ValueTypeSpecifics
        {
            using value_t = void;

            static value_t Convert(const std::vector<BYTE>& data)
            {
                static_assert(false, "No Type specific override has been supplied");
            }
        };

        template <>
        struct ValueTypeSpecifics<REG_NONE>
        {
            using value_t = std::vector<BYTE>;
            static value_t Convert(const std::vector<BYTE>& data);
        };

        template <>
        struct ValueTypeSpecifics<REG_SZ>
        {
            using value_t = std::string;
            static value_t Convert(const std::vector<BYTE>& data);
        };

        template <>
        struct ValueTypeSpecifics<REG_EXPAND_SZ>
        {
            using value_t = std::string;
            static value_t Convert(const std::vector<BYTE>& data);
        };

        template <>
        struct ValueTypeSpecifics<REG_BINARY>
        {
            using value_t = std::vector<BYTE>;
            static value_t Convert(const std::vector<BYTE>& data);
        };

        template <>
        struct ValueTypeSpecifics<REG_DWORD_LITTLE_ENDIAN>
        {
            using value_t = uint32_t;
            static value_t Convert(const std::vector<BYTE>& data);
        };
    }

    struct Key;
    struct ValueList;

    // A registry value.
    struct Value
    {
        friend Key;
        friend ValueList;

        // The type of data stored in the Value.
        enum class Type : DWORD
        {
            None = REG_NONE,
            String = REG_SZ,
            ExpandString = REG_EXPAND_SZ,
            Binary = REG_BINARY,
            DWord = REG_DWORD,
            DWordLittleEndian = REG_DWORD_LITTLE_ENDIAN,
            DWordBigEndian = REG_DWORD_BIG_ENDIAN,
            MultiString = REG_MULTI_SZ,
            QWord = REG_QWORD,
            QWordLittleEndian = REG_QWORD_LITTLE_ENDIAN,
        };

        Type GetType() const { return m_type; }

        template <Type T>
        typename details::ValueTypeSpecifics<static_cast<DWORD>(T)>::value_t GetValue() const
        {
            auto value = TryGetValue<T>();
            if (!value.has_value())
            {
                THROW_HR(E_INVALIDARG);
            }

            return std::move(value.value());
        }

        template <Type T>
        typename std::optional<typename details::ValueTypeSpecifics<static_cast<DWORD>(T)>::value_t> TryGetValue() const
        {
            if (HasCompatibleType(T))
            {
                return details::ValueTypeSpecifics<static_cast<DWORD>(T)>::Convert(m_data);
            }
            else
            {
                return std::nullopt;
            }
        }

    private:
        Value(DWORD type, std::vector<BYTE>&& data);

        bool HasCompatibleType(Type type) const;

        Type m_type;
        std::vector<BYTE> m_data;
    };

    // Value iteration
    struct ValueList
    {
        friend Key;

        struct const_iterator;

        struct ValueRef : Value
        {
            friend const_iterator;

            // Gets the name of the value.
            std::string Name() const;

        private:
            ValueRef(std::wstring&& valueName, DWORD type, std::vector<BYTE>&& data);

            std::wstring m_valueName;
        };

        struct const_iterator
        {
            friend ValueList;

            const_iterator& operator++();
            const_iterator operator++(int);

            bool operator==(const const_iterator& other) const;
            bool operator!=(const const_iterator& other) const;

            const ValueRef& operator*() const;
            const ValueRef* operator->() const;

        private:
            // Create an iterator
            const_iterator(const wil::shared_hkey& key, DWORD index = 0);

            // Create an iterator for end
            const_iterator() = default;

            void GetValue();

            // An empty handle represents the end iterator.
            wil::shared_hkey m_key;
            DWORD m_index = 0;
            std::optional<ValueRef> m_value;
        };

        const_iterator begin() const;
        const_iterator end() const;

    private:
        ValueList(wil::shared_hkey key);

        wil::shared_hkey m_key;
    };

    // A registry key.
    struct Key
    {
        Key() = default;
        Key(HKEY key);
        Key(HKEY key, std::string_view subKey, DWORD options = 0, REGSAM access = KEY_READ);
        Key(HKEY key, const std::wstring& subKey, DWORD options = 0, REGSAM access = KEY_READ);

        // --== Sub-Key iteration ==--
        struct const_iterator;

        struct SubKeyRef
        {
            friend const_iterator;

            // Gets the name of the subkey.
            std::string Name() const;

            // Opens the subkey.
            Key Open() const;

            operator bool() const { return m_parentKey.operator bool(); }

        private:
            // For a valid iterator
            SubKeyRef(const wil::shared_hkey& key, REGSAM access);

            // For the end iterator
            SubKeyRef() = default;

            // Enumerates the subkey of m_parentKey at the given index.
            void Enum(DWORD index);

            wil::shared_hkey m_parentKey;
            REGSAM m_access = KEY_READ;
            std::wstring m_subKeyName;
        };

        struct const_iterator
        {
            friend Key;

            const_iterator& operator++();
            const_iterator operator++(int);

            bool operator==(const const_iterator& other) const;
            bool operator!=(const const_iterator& other) const;

            const SubKeyRef& operator*() const;
            const SubKeyRef* operator->() const;

        private:
            // Create an iterator for begin
            const_iterator(const wil::shared_hkey& key, REGSAM access);

            // Create an iterator for end
            const_iterator() = default;

            DWORD m_index = 0;
            SubKeyRef m_subkey;
        };

        const_iterator begin() const;
        const_iterator end() const;

        std::optional<Value> operator[](std::string_view name) const;
        std::optional<Value> operator[](const std::wstring& name) const;

        std::optional<Key> SubKey(std::string_view name, DWORD options = 0) const;
        std::optional<Key> SubKey(const std::wstring& name, DWORD options = 0) const;

        ValueList Values() const;

        operator bool() const { return m_key.operator bool(); }

        // Open a Key; will return an empty Key if the subkey does not exist.
        static Key OpenIfExists(HKEY key, std::string_view subKey = {}, DWORD options = 0, REGSAM access = KEY_READ);
        static Key OpenIfExists(HKEY key, const std::wstring& subKey = {}, DWORD options = 0, REGSAM access = KEY_READ);

    private:
        // When ignoring error, returns whether the key existed
        bool Initialize(HKEY key, const std::wstring& subKey, DWORD options, REGSAM access, bool ignoreErrorIfDoesNotExist);

        wil::shared_hkey m_key;
        REGSAM m_access = KEY_READ;
    };
}
