#include "pch.h"
#include <atomic_ref_count.h>

using namespace std;

void verify_error_info(
    xlang_error_info* error_info,
    xlang_result expected_error,
    xlang_string expected_message = nullptr,
    xlang_string expected_projection_identifier = nullptr,
    xlang_string expected_language_error = nullptr,
    xlang_unknown* expected_execution_trace = nullptr,
    xlang_unknown* expected_language_information = nullptr,
    bool has_propagated_error = false)
{
    INFO("Verifying properties of xlang error info");
    xlang_result error{};
    error_info->GetError(&error);
    REQUIRE(error == expected_error);

    xlang_string message{};
    error_info->GetMessage(&message);
    REQUIRE(message == expected_message);

    xlang_string projection_identifier{};
    error_info->GetProjectionIdentifier(&projection_identifier);
    REQUIRE(projection_identifier == expected_projection_identifier);

    xlang_unknown* execution_trace{};
    error_info->GetExecutionTrace(&execution_trace);
    REQUIRE(execution_trace == expected_execution_trace);

    xlang_string language_error;
    error_info->GetLanguageError(&language_error);
    REQUIRE(language_error == expected_language_error);

    xlang_unknown* language_information{};
    error_info->GetLanguageInformation(&language_information);
    REQUIRE(language_information == expected_language_information);

    xlang_error_info* propagated_error;
    error_info->GetPropagatedError(&propagated_error);
    if (has_propagated_error)
    {
        REQUIRE(propagated_error != nullptr);
        propagated_error->Release();
    }
    else
    {
        REQUIRE(propagated_error == nullptr);
    }
}

TEST_CASE("Error origination with only result and message")
{
    basic_string_view<xlang_char8> message = "This is an error";
    xlang_string abi_message{};
    REQUIRE(xlang_create_string_utf8(message.data(), message.size(), &abi_message) == nullptr);
    
    INFO("Originating error");
    xlang_error_info* result = xlang_originate_error(xlang_result::access_denied, abi_message);
    REQUIRE(result != nullptr);

    verify_error_info(result, xlang_result::access_denied, abi_message);
    REQUIRE(result->Release() == 0);
    result = nullptr;
}

inline constexpr xlang_guid iinformation_guid{ 0xd06e3e2e, 0xe412, 0x4e1f, { 0xa3, 0xb1, 0xb7, 0x4c, 0x20, 0x27, 0xd9, 0xa5 } };

struct XLANG_NOVTABLE iinformation : xlang_unknown
{
    virtual xlang_error_info* get_information(bool* information) noexcept = 0;
};

struct information : iinformation
{
    explicit information(bool language_information) noexcept :
        m_language_information{ language_information }
    {
    }
    com_interop_result XLANG_CALL QueryInterface(xlang_guid const& id, void** object) noexcept final
    {
        *object = nullptr;

        if (id == xlang_unknown_guid)
        {
            static_assert(std::is_base_of_v<xlang_unknown, iinformation>, "Can only combine these two cases if this is true.");
            *object = static_cast<xlang_unknown*>(this);
        }
        else if (id == iinformation_guid)
        {
            *object = static_cast<iinformation*>(this);
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

    xlang_error_info* get_information(bool* language_information) noexcept override
    {
        *language_information = m_language_information;
        return nullptr; // xlang_error_ok;
    }

private:
    bool m_language_information;
    xlang::impl::atomic_ref_count m_count;
};

TEST_CASE("Error origination with all parameters")
{
    basic_string_view<xlang_char8> str = "This is another error";
    xlang_string message{};
    REQUIRE(xlang_create_string_utf8(str.data(), str.size(), &message) == nullptr);
    str = "native";
    xlang_string projection_identifier{};
    REQUIRE(xlang_create_string_utf8(str.data(), str.size(), &projection_identifier) == nullptr);
    str = "0x80070057";
    xlang_string language_error{};
    REQUIRE(xlang_create_string_utf8(str.data(), str.size(), &language_error) == nullptr);
    xlang_unknown* execution_trace = new information(false);
    xlang_unknown* language_information = new information(true);

    INFO("Originating error");
    xlang_error_info* result = xlang_originate_error(
        xlang_result::invalid_arg,
        message,
        projection_identifier,
        language_error,
        execution_trace,
        language_information);
    REQUIRE(result != nullptr);

    verify_error_info(
        result,
        xlang_result::invalid_arg,
        message,
        projection_identifier,
        language_error,
        execution_trace,
        language_information);

    INFO("Propagating error with all parameters");
    str = "python";
    xlang_string projection_identifier2{};
    REQUIRE(xlang_create_string_utf8(str.data(), str.size(), &projection_identifier2) == nullptr);
    str = "ValueError";
    xlang_string language_error2{};
    REQUIRE(xlang_create_string_utf8(str.data(), str.size(), &language_error2) == nullptr);
    xlang_unknown* execution_trace2 = new information(false);
    xlang_unknown* language_information2 = new information(true);

    result->PropagateError(projection_identifier2, language_error2, execution_trace2, language_information2);
    verify_error_info(
        result,
        xlang_result::invalid_arg,
        message,
        projection_identifier,
        language_error,
        execution_trace,
        language_information,
        true);

    xlang_error_info* propagated_error{};
    result->GetPropagatedError(&propagated_error);
    REQUIRE(propagated_error != nullptr);
    verify_error_info(
        propagated_error,
        xlang_result::invalid_arg,
        message,
        projection_identifier2,
        language_error2,
        execution_trace2,
        language_information2);

    INFO("Propagating error with only projection identifier");
    result->PropagateError(projection_identifier, nullptr, nullptr, nullptr);

    xlang_error_info* propagated_error2{};
    propagated_error->GetPropagatedError(&propagated_error2);
    REQUIRE(propagated_error2 != nullptr);
    verify_error_info(
        propagated_error2,
        xlang_result::invalid_arg,
        message,
        projection_identifier);

    REQUIRE(propagated_error2->Release() == 1);
    propagated_error2 = nullptr;
    REQUIRE(propagated_error->Release() == 1);
    propagated_error = nullptr;
    REQUIRE(result->Release() == 0);
    result = nullptr;
}