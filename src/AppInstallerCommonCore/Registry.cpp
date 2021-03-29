// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "Public/winget/Registry.h"
#include "Public/AppInstallerStrings.h"
#include "Public/AppInstallerLogging.h"


namespace AppInstaller::Registry
{
    namespace
    {
        std::wstring_view ConvertBytesToWideStringView(const std::vector<BYTE>& data)
        {
            THROW_HR_IF(E_NOT_VALID_STATE, (data.size() % sizeof(wchar_t)) != 0);
            std::wstring_view result{ reinterpret_cast<const wchar_t*>(data.data()), data.size() / sizeof(wchar_t) };

            // Registry values may or may not be null terminated; we will remove any trailing nulls
            while (!result.empty() && result.back() == L'\0')
            {
                result = result.substr(0, result.size() - 1);
            }

            return result;
        }

        std::wstring ConvertBytesToWideString(const std::vector<BYTE>& data)
        {
            return std::wstring{ ConvertBytesToWideStringView(data) };
        }

        std::string ConvertBytesToString(const std::vector<BYTE>& data)
        {
            return Utility::ConvertToUTF8(ConvertBytesToWideStringView(data));
        }

        uint32_t ConvertBytesToUInt32LE(const std::vector<BYTE>& data)
        {
            THROW_HR_IF(E_NOT_VALID_STATE, data.size() != sizeof(uint32_t));
            uint32_t result = 0;
            uint32_t shift = 0;

            for (const BYTE datum : data)
            {
                result |= ((static_cast<uint32_t>(datum) & 0xFF) << shift);
                shift += 8;
            }

            return result;
        }
    }

    namespace details
    {
        ValueTypeSpecifics<REG_NONE>::value_t ValueTypeSpecifics<REG_NONE>::Convert(const std::vector<BYTE>& data)
        {
            return data;
        }

        ValueTypeSpecifics<REG_SZ>::value_t ValueTypeSpecifics<REG_SZ>::Convert(const std::vector<BYTE>& data)
        {
            return ConvertBytesToString(data);
        }

        ValueTypeSpecifics<REG_EXPAND_SZ>::value_t ValueTypeSpecifics<REG_EXPAND_SZ>::Convert(const std::vector<BYTE>& data)
        {
            return Utility::ConvertToUTF8(Utility::ExpandEnvironmentVariables(ConvertBytesToWideString(data)));
        }

        ValueTypeSpecifics<REG_BINARY>::value_t ValueTypeSpecifics<REG_BINARY>::Convert(const std::vector<BYTE>& data)
        {
            return data;
        }

        ValueTypeSpecifics<REG_DWORD_LITTLE_ENDIAN>::value_t ValueTypeSpecifics<REG_DWORD_LITTLE_ENDIAN>::Convert(const std::vector<BYTE>& data)
        {
            return ConvertBytesToUInt32LE(data);
        }
    }

    Value::Value(DWORD type, std::vector<BYTE>&& data) : m_type(static_cast<Type>(type)), m_data(std::move(data))
    {
    }

    bool Value::HasCompatibleType(Type type) const
    {
        // Allow interop between String and ExpandString
        if ((m_type == Type::String || m_type == Type::ExpandString) && (type == Type::String || type == Type::ExpandString))
        {
            return true;
        }

        return m_type == type;
    }

    Key::Key(HKEY key)
    {
        Initialize(key, {}, 0, KEY_READ, false);
    }

    Key::Key(HKEY key, std::string_view subKey, DWORD options, REGSAM access)
    {
        Initialize(key, Utility::ConvertToUTF16(subKey), options, access, false);
    }

    Key::Key(HKEY key, const std::wstring& subKey, DWORD options, REGSAM access)
    {
        Initialize(key, subKey, options, access, false);
    }

    std::string Key::SubKeyRef::Name() const
    {
        return Utility::ConvertToUTF8(m_subKeyName);
    }

    Key Key::SubKeyRef::Open() const
    {
        return { m_parentKey.get(), m_subKeyName, 0, m_access };
    }

    Key::SubKeyRef::SubKeyRef(const wil::shared_hkey& key, REGSAM access) :
        m_parentKey(key), m_access(access), m_subKeyName(64, L'\0')
    {
        Enum(0);
    }

    void Key::SubKeyRef::Enum(DWORD index)
    {
        LSTATUS status = ERROR_SUCCESS;
        DWORD charCount = 0;

        while (m_subKeyName.size() < 4096)
        {
            charCount = wil::safe_cast<DWORD>(m_subKeyName.size());
            status = RegEnumKeyExW(m_parentKey.get(), index, &m_subKeyName[0], &charCount, nullptr, nullptr, nullptr, nullptr);

            if (status == ERROR_MORE_DATA)
            {
                // See if we can get away with the current capacity
                if (m_subKeyName.size() < m_subKeyName.capacity())
                {
                    m_subKeyName.resize(m_subKeyName.capacity());
                }
                else
                {
                    m_subKeyName.resize(m_subKeyName.capacity() * 2);
                }
            }
            else
            {
                break;
            }
        }

        if (status == ERROR_SUCCESS)
        {
            m_subKeyName.resize(wil::safe_cast<size_t>(charCount));
        }
        else if (status == ERROR_NO_MORE_ITEMS)
        {
            m_parentKey.reset();
        }
        else
        {
            THROW_IF_WIN32_ERROR(status);
        }
    }

    Key::const_iterator& Key::const_iterator::operator++()
    {
        m_subkey.Enum(++m_index);
        return *this;
    }

    Key::const_iterator Key::const_iterator::operator++(int)
    {
        const_iterator result = *this;
        m_subkey.Enum(++m_index);
        return result;
    }

    bool Key::const_iterator::operator==(const const_iterator& other) const
    {
        return (!m_subkey.m_parentKey && !other.m_subkey.m_parentKey) || (m_subkey.m_parentKey.get() == other.m_subkey.m_parentKey.get() && m_index == other.m_index);
    }

    bool Key::const_iterator::operator!=(const const_iterator& other) const
    {
        return !operator==(other);
    }

    const Key::SubKeyRef& Key::const_iterator::operator*() const
    {
        return m_subkey;
    }

    const Key::SubKeyRef* Key::const_iterator::operator->() const
    {
        return &m_subkey;
    }

    Key::const_iterator::const_iterator(const wil::shared_hkey& key, REGSAM access) :
        m_subkey(key, access)
    {
    }

    Key::const_iterator Key::begin() const
    {
        return { m_key, m_access };
    }

    Key::const_iterator Key::end() const
    {
        return {};
    }

    std::optional<Value> Key::operator[](std::string_view name) const
    {
        return operator[](Utility::ConvertToUTF16(name));
    }

    std::optional<Value> Key::operator[](const std::wstring& name) const
    {
        std::vector<BYTE> data;
        data.resize(64);

        LSTATUS status = ERROR_SUCCESS;
        DWORD type = 0;
        DWORD byteCount = 0;

        while (data.size() < (64 << 20))
        {
            byteCount = wil::safe_cast<DWORD>(data.size());
            status = RegGetValueW(m_key.get(), nullptr, name.c_str(), RRF_RT_ANY | RRF_NOEXPAND, &type, data.data(), &byteCount);

            if (status == ERROR_MORE_DATA && byteCount > data.size())
            {
                data.resize(byteCount);
            }
            else
            {
                break;
            }
        }

        if (status == ERROR_FILE_NOT_FOUND)
        {
            return {};
        }

        THROW_IF_WIN32_ERROR(status);

        // Resize to actual data size
        data.resize(byteCount);

        return Value{ type, std::move(data) };
    }

    Key Key::OpenIfExists(HKEY key, std::string_view subKey, DWORD options, REGSAM access)
    {
        return OpenIfExists(key, Utility::ConvertToUTF16(subKey), options, access);
    }

    Key Key::OpenIfExists(HKEY key, const std::wstring& subKey, DWORD options, REGSAM access)
    {
        Key result;
        result.Initialize(key, subKey, options, access, true);
        return result;
    }

    void Key::Initialize(HKEY key, const std::wstring& subKey, DWORD options, REGSAM access, bool ignoreErrorIfDoesNotExist)
    {
        LSTATUS status = RegOpenKeyExW(key, subKey.c_str(), options, access, &m_key);

        if (ignoreErrorIfDoesNotExist && status == ERROR_FILE_NOT_FOUND)
        {
            AICLI_LOG(Core, Verbose, << "Subkey '" << Utility::ConvertToUTF8(subKey) << "' was not found");
            return;
        }

        THROW_IF_WIN32_ERROR(status);
    }
}
