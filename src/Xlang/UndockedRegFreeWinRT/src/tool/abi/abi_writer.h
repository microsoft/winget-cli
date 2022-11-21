#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "common.h"
#include "meta_reader.h"
#include "metadata_cache.h"
#include "namespace_iterator.h"
#include "text_writer.h"
#include "versioning.h"

struct basic_writer : xlang::text::writer_base<basic_writer> {};

template <typename Int>
struct format_hex
{
    static_assert(std::is_unsigned_v<Int>);
    Int value;
};
template <typename Int>
format_hex(Int) -> format_hex<Int>;

struct indent { std::size_t additional_indentation = 0; };

struct writer : xlang::text::writer_base<writer>
{
    writer(abi_configuration const& config) :
        m_config(config)
    {
    }

    using writer_base::write;

    abi_configuration const& config() const noexcept
    {
        return m_config;
    }

    void write_code(std::string_view value)
    {
        xlang::text::bind_list("::", namespace_range{ value })(*this);
    }

    static void write_uppercase(writer& w, std::string_view str)
    {
        for (auto ch : str)
        {
            w.write(static_cast<char>(::toupper(ch)));
        }
    }

    static void write_lowercase(writer& w, std::string_view str)
    {
        for (auto ch : str)
        {
            w.write(static_cast<char>(::tolower(ch)));
        }
    }

    template <typename Int>
    void write(format_hex<Int> val)
    {
        write_value(val.value);
    }

    void write(indent value)
    {
        for (std::size_t i = 0; i < (m_indentation + value.additional_indentation); ++i)
        {
            write("    ");
        }
    }

    void write_value(bool value)
    {
        write(value ? "TRUE" : "FALSE");
    }

    void write_value(char16_t value)
    {
        write_printf("%#0hx", value);
    }

    void write_value(int8_t value)
    {
        write_printf("%hhd", value);
    }

    void write_value(uint8_t value)
    {
        write_printf("%#0hhx", value);
    }

    void write_value(int16_t value)
    {
        write_printf("%hd", value);
    }

    void write_value(uint16_t value)
    {
        write_printf("%#0hx", value);
    }

    void write_value(int32_t value)
    {
        write_printf("%d", value);
    }

    void write_value(uint32_t value)
    {
        write_printf("%#0x", value);
    }

    void write_value(int64_t value)
    {
        write_printf("%lld", value);
    }

    void write_value(uint64_t value)
    {
        write_printf("%#0llx", value);
    }

    void write_value(float value)
    {
        write_printf("%f", value);
    }

    void write_value(double value)
    {
        write_printf("%f", value);
    }

    void write_value(std::string_view value)
    {
        write("\"%\"", value);
    }

    void write(xlang::meta::reader::Constant const& value)
    {
        using namespace xlang::meta::reader;
        switch (value.Type())
        {
        case ConstantType::Boolean:
            write_value(value.ValueBoolean());
            break;
        case ConstantType::Char:
            write_value(value.ValueChar());
            break;
        case ConstantType::Int8:
            write_value(value.ValueInt8());
            break;
        case ConstantType::UInt8:
            write_value(value.ValueUInt8());
            break;
        case ConstantType::Int16:
            write_value(value.ValueInt16());
            break;
        case ConstantType::UInt16:
            write_value(value.ValueUInt16());
            break;
        case ConstantType::Int32:
            write_value(value.ValueInt32());
            break;
        case ConstantType::UInt32:
            write_value(value.ValueUInt32());
            break;
        case ConstantType::Int64:
            write_value(value.ValueInt64());
            break;
        case ConstantType::UInt64:
            write_value(value.ValueUInt64());
            break;
        case ConstantType::Float32:
            write_value(value.ValueFloat32());
            break;
        case ConstantType::Float64:
            write_value(value.ValueFloat64());
            break;
        case ConstantType::String:
            write_value(value.ValueString());
            break;
        case ConstantType::Class:
            write("null");
            break;
        }
    }

    void push_namespace(std::string_view ns);
    void push_inline_namespace(std::string_view ns);
    void pop_namespace();
    void pop_inline_namespace();

    bool push_contract_guard(version ver);
    void push_contract_guard(contract_version ver);
    void push_contract_guard(contract_history vers);
    void pop_contract_guards(std::size_t count);

    bool begin_declaration(std::string_view mangledName)
    {
        auto [itr, added] = m_declaredTypes.emplace(mangledName, declaration_stage::begin);
        return added;
    }

    void end_declaration(std::string_view mangledName)
    {
        auto itr = m_declaredTypes.find(mangledName);
        XLANG_ASSERT(itr != m_declaredTypes.end());
        XLANG_ASSERT(itr->second != declaration_stage::end);
        itr->second = declaration_stage::end;
    }

    bool should_forward_declare(std::string_view mangledName)
    {
        auto [itr, added] = m_declaredTypes.emplace(mangledName, declaration_stage::begin);
        if (itr->second != declaration_stage::begin)
        {
            return false;
        }

        itr->second = declaration_stage::forward;
        return true;
    }

    void begin_c_interface()
    {
        XLANG_ASSERT(m_namespaceStack.empty());
        XLANG_ASSERT(m_contractGuardStack.empty());
        m_declaredTypes.clear();
    }

private:

    abi_configuration const& m_config;

    std::size_t m_indentation = 0;
    std::vector<std::string_view> m_namespaceStack;

    std::vector<contract_history> m_contractGuardStack;

    enum class declaration_stage
    {
        begin,
        forward,
        end,
    };

    // A set of already declared (or in the case of generics, defined) types
    std::map<std::string_view, declaration_stage> m_declaredTypes;
};

void write_abi_header(std::string_view fileName, abi_configuration const& config, type_cache const& types);
