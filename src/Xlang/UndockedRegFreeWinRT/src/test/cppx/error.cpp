#include "pch.h"
#include <xlang/Foundation.Collections.h>
#include "catch.hpp"
#include <windows.h>

using namespace xlang;
using namespace Foundation;
using namespace Foundation::Collections;

template <typename T>
void test_exception(xlang_result const code)
{
    REQUIRE_THROWS_AS(throw_xlang_error(code), T);

    try
    {
        throw T();
    }
    catch (T const& e)
    {
        REQUIRE(code == e.code());
        REQUIRE("" == e.message());
    }

    try
    {
        com_ptr<xlang_error_info> error_info{ xlang_originate_error(code), take_ownership_from_abi };
        throw T(error_info.get());
    }
    catch (T const& e)
    {
        REQUIRE(code == e.code());
        REQUIRE("" == e.message());
    }

    try
    {
        throw T("custom message");
    }
    catch (T const& e)
    {
        REQUIRE(code == e.code());
        REQUIRE("custom message" == e.message());
    }

    try
    {
        hstring msg = to_hstring("custom message");
        com_ptr<xlang_error_info> error_info{ xlang_originate_error(code, get_abi(msg)), take_ownership_from_abi };
        throw T(error_info.get());
    }
    catch (T const& e)
    {
        REQUIRE(code == e.code());
        REQUIRE("custom message" == e.message());
    }
}

#pragma warning(disable: 4702)  // unreachable code
TEST_CASE("Errors")
{
    // Ensure these don't throw.
    check_xlang_error(nullptr);
    check_com_interop_error(com_interop_result::success);
    check_hresult(S_OK);

    // Ensure these throw.
    test_exception<access_denied_error>(xlang_result::access_denied);
    test_exception<out_of_bounds_error>(xlang_result::bounds);
    test_exception<invalid_handle_error>(xlang_result::handle);
    test_exception<invalid_argument_error>(xlang_result::invalid_arg);
    test_exception<invalid_state_error>(xlang_result::invalid_state);
    test_exception<no_interface_error>(xlang_result::no_interface);
    test_exception<not_implemented_error>(xlang_result::not_impl);
    test_exception<pointer_error>(xlang_result::pointer);
    test_exception<type_load_error>(xlang_result::type_load);

    // An error originates in a component and is consumed within C++/xlang.
    try
    {
        IIterable<int> values;
        copy_from_abi(values, get_abi(param::iterable<int>{}));
        values.First();
        FAIL(L"Previous line should throw");
    }
    catch (invalid_state_error const& e) // catching specific exception type
    {
        REQUIRE(xlang_result::invalid_state == e.code());
    }

    // The same with catching base exception type.
    try
    {
        IIterable<int> values;
        copy_from_abi(values, get_abi(param::iterable<int>{}));
        values.First();
        FAIL(L"Previous line should throw");
    }
    catch (xlang_error const& e)
    {
        REQUIRE(xlang_result::invalid_state == e.code());
    }

    SetLastError(ERROR_ACCESS_DENIED);
    REQUIRE_THROWS_AS(throw_last_error(), access_denied_error);

    // Support for HRESULT errors.
    REQUIRE_THROWS_AS(check_hresult(E_HANDLE), invalid_handle_error);

    try
    {
        check_hresult(E_ACCESSDENIED);
        FAIL(L"Previous line should throw");
    }
    catch (xlang_error const& e)
    {
        REQUIRE(e.code() == xlang_result::access_denied);
    }

    try
    {
        check_hresult(E_BOUNDS);
        FAIL(L"Previous line should throw");
    }
    catch (out_of_bounds_error const& e)
    {
        REQUIRE(e.code() == xlang_result::bounds);
    }

    try
    {
        check_hresult(E_BOUNDS);
        FAIL(L"Previous line should throw");
    }
    catch (...)
    {
        com_ptr<xlang_error_info> error_info{ to_xlang_error(), take_ownership_from_abi };
        xlang_result error;
        error_info->GetError(&error);
        REQUIRE(error == xlang_result::bounds);
    }

    // Support for com_interop_result errors
    REQUIRE_THROWS_AS(check_com_interop_error(com_interop_result::no_interface), no_interface_error);
    REQUIRE_THROWS_AS(check_com_interop_error(com_interop_result::pointer), pointer_error);

    // Make sure delegates propagate correctly.
    {
        TypedEventHandler<int, int> d = [](int, int)
        {
            throw no_interface_error();
        };

        REQUIRE_THROWS_AS(d(1, 2), no_interface_error);
    }
}