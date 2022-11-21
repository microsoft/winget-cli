#include "pch.h"

using namespace xlang;

using u8string = std::basic_string<xlang_char8>;
using u8string_view = std::basic_string_view<xlang_char8>;

TEST_CASE("hstring,constructor,default")
{
    {
        hstring s;
        REQUIRE(s.empty());
    }

    {
        hstring s{};
        REQUIRE(s.empty());
    }
}

TEST_CASE("hstring,constructor,copy")
{
    {
        hstring a;
        REQUIRE(a.empty());

        hstring b = a;
        REQUIRE(a.empty());
        REQUIRE(b.empty());
    }

    {
        hstring a{ u8"abc" };
        REQUIRE(u8"abc" == a);

        hstring b = a;
        REQUIRE(u8"abc" == a);
        REQUIRE(u8"abc" == b);
    }

    {
        hstring a{ u"abc" };
        REQUIRE(u"abc" == a);

        hstring b = a;
        REQUIRE(u"abc" == a);
        REQUIRE(u"abc" == b);
    }
}

TEST_CASE("hstring,constructor,move")
{
    {
        hstring a;
        REQUIRE(a.empty());

        hstring b = std::move(a);
        REQUIRE(a.empty());
        REQUIRE(b.empty());
    }

    {
        hstring a{ u8"abc" };
        REQUIRE(u8"abc" == a);

        hstring b = std::move(a);
        REQUIRE(a.empty());
        REQUIRE(u8"abc" == b);
    }

    {
        hstring a{ u"abc" };
        REQUIRE(u"abc" == a);

        hstring b = std::move(a);
        REQUIRE(a.empty());
        REQUIRE(u"abc" == b);
    }
}

TEST_CASE("hstring,assign,copy")
{
    {
        hstring a;
        hstring b;
        REQUIRE(a.empty());
        REQUIRE(b.empty());

        b = a;
        REQUIRE(a.empty());
        REQUIRE(b.empty());
    }

    {
        hstring a{ u8"abc" };
        hstring b;
        REQUIRE(u8"abc" == a);
        REQUIRE(b.empty());

        b = a;
        REQUIRE(u8"abc" == a);
        REQUIRE(u8"abc" == b);
    }

    {
        hstring a{ u"abc" };
        hstring b;
        REQUIRE(u"abc" == a);
        REQUIRE(b.empty());

        b = a;
        REQUIRE(u"abc" == a);
        REQUIRE(u"abc" == b);
    }
}

TEST_CASE("hstring,assign,move")
{
    {
        hstring a;
        hstring b;
        REQUIRE(a.empty());
        REQUIRE(b.empty());

        b = std::move(a);
        REQUIRE(a.empty());
        REQUIRE(b.empty());
    }

    {
        hstring a{ u8"abc" };
        hstring b;
        REQUIRE(u8"abc" == a);
        REQUIRE(b.empty());

        b = std::move(a);
        REQUIRE(a.empty());
        REQUIRE(u8"abc" == b);
    }

    {
        hstring a{ u"abc" };
        hstring b;
        REQUIRE(u"abc" == a);
        REQUIRE(b.empty());

        b = std::move(a);
        REQUIRE(a.empty());
        REQUIRE(u"abc" == b);
    }
}

TEST_CASE("hstring,assign,std::basic_string")
{
    {
        hstring a;
        REQUIRE(a.empty());
        u8string b(u8"abc");

        a = b;
        REQUIRE(a == b);
        REQUIRE(a == u8"abc");
    }
}

TEST_CASE("hstring,assign,std::basic_string_view")
{
    {
        hstring a;
        REQUIRE(a.empty());
        u8string_view b(u8"abc");

        a = b;
        REQUIRE(a == b);
        REQUIRE(a == u8"abc");
    }
}

TEST_CASE("hstring,assign,T*")
{
    {
        hstring a;
        REQUIRE(a.empty());
        xlang_char8 const* b = u8"abc";

        a = b;
        REQUIRE(a == b);
        REQUIRE(a == u8"abc");
    }
}

TEST_CASE("hstring,assign,T[]")
{
    {
        hstring a;
        REQUIRE(a.empty());
        xlang_char8 const b[] = u8"abc";

        a = b;
        REQUIRE(a == b);
        REQUIRE(a == u8"abc");
    }
}

TEST_CASE("hstring,assign,initializer_list")
{
    {
        hstring a(u8"abc");
        REQUIRE(a == u8"abc");

        a = { u8'A', u8'B', u8'C' };
        REQUIRE(a == u8"ABC");
    }
    {
        hstring a(u"abc");
        REQUIRE(a == u"abc");

        a = { u'A', u'B', u'C' };
        REQUIRE(a == u"ABC");
    }
}

TEST_CASE("hstring,constructor,initializer_list")
{
    {
        hstring s{ u8'A', u8'B', u8'C' };
        REQUIRE(s == u8"ABC");
    }
    {
        hstring s{ u'A', u'B', u'C' };
        REQUIRE(s == u"ABC");
    }

    {
        hstring s{};
        REQUIRE(s.empty());
    }
}

TEST_CASE("hstring,constructor,wchar_t,size")
{
    {
        hstring s(u8"", 0);
        REQUIRE(s.empty());
    }

    {
        hstring s(u8"abcde", 3);
        REQUIRE(u8"abc" == s);
    }
}

TEST_CASE("hstring,constructor,T")
{
    {
        hstring s{ u8"" };
        REQUIRE(s.empty());
    }

    {
        hstring s{ u8"abc" };
        REQUIRE(u8"abc" == s);

        s = u8"abcde";
        REQUIRE(u8"abcde" == s);
    }
    {
        hstring s{ u"" };
        REQUIRE(s.empty());
    }

    {
        hstring s{ u"abc" };
        REQUIRE(u"abc" == s);

        s = u"abcde";
        REQUIRE(u"abcde" == s);
    }
}

TEST_CASE("hstring,constructor,std::basic_string")
{
    {
        hstring s{ u8string(u8"") };
        REQUIRE(s.empty());
    }

    {
        hstring s{ u8string(u8"abc") };
        REQUIRE(u8"abc" == s);

        s = u8string(u8"abcde");
        REQUIRE(u8"abcde" == s);
    }
    {
        hstring s{ std::u16string(u"") };
        REQUIRE(s.empty());
    }

    {
        hstring s{ std::u16string(u"abc") };
        REQUIRE(u"abc" == s);

        s = std::u16string(u"abcde");
        REQUIRE(u"abcde" == s);
    }
}

TEST_CASE("hstring,empty,size,clear")
{
    hstring s;

    REQUIRE(s.empty());
    REQUIRE(0 == s.size());

    s = u8"abc";

    REQUIRE(!s.empty());
    REQUIRE(3 == s.size());

    s.clear();

    REQUIRE(s.empty());
    REQUIRE(0 == s.size());

    s = u8"abcde";

    REQUIRE(!s.empty());
    REQUIRE(5 == s.size());

    s = u8"";

    REQUIRE(s.empty());
    REQUIRE(0 == s.size());
}

TEST_CASE("hstring,operator,std::basic_string_view")
{
    hstring hs;
    u8string_view ws = hs;
    REQUIRE(ws.empty());

    hs = u8"abc";
    ws = hs;
    REQUIRE(u8"abc" == ws);

    hs.clear();
    REQUIRE(u8"abc" == ws);
    ws = hs;
    REQUIRE(ws.empty());
}

TEST_CASE("hstring,[N]")
{
    hstring s{ u8"abc" };

    REQUIRE(u8'a' == s[0]);
    REQUIRE(u8'b' == s[1]);
    REQUIRE(u8'c' == s[2]);
}

TEST_CASE("hstring,front,back")
{
    hstring s{ u8"abc" };

    REQUIRE(u8'a' == s.front());
    REQUIRE(u8'c' == s.back());
}

TEST_CASE("hstring,data,c_str,begin,cbegin")
{
    auto const test = u8"abc";
    std::size_t const len = 3;
    hstring s{ u8"abc" };

    REQUIRE(0 == std::char_traits<xlang_char8>::compare(test, s.data(), len));
    REQUIRE(0 == std::char_traits<xlang_char8>::compare(test, s.c_str(), len));
    REQUIRE(0 == std::char_traits<xlang_char8>::compare(test, s.begin(), len));
    REQUIRE(0 == std::char_traits<xlang_char8>::compare(test, s.cbegin(), len));
    REQUIRE(0 == std::char_traits<xlang_char8>::compare(test, std::begin(s), len));
    REQUIRE(0 == std::char_traits<xlang_char8>::compare(test, std::cbegin(s), len));
}

TEST_CASE("hstring,begin,end")
{
    hstring s{ u8"abc" };

    u8string copy(s.begin(), s.end());

    REQUIRE(u8"abc" == copy);
}

TEST_CASE("hstring,cbegin,cend")
{
    hstring s{ u8"abc" };

    u8string copy(s.cbegin(), s.cend());

    REQUIRE(u8"abc" == copy);
}

TEST_CASE("hstring,rbegin,rend")
{
    hstring s{ u8"abc" };

    u8string copy(s.rbegin(), s.rend());

    REQUIRE(u8"cba" == copy);
}

TEST_CASE("hstring,crbegin,crend")
{
    hstring s{ u8"abc" };

    u8string copy(s.crbegin(), s.crend());

    REQUIRE(u8"cba" == copy);
}

static void test_hstring_put(xlang_string* result)
{
    hstring local{ u8"abc" };
    *result = detach_abi(local);
}

TEST_CASE("hstring,accessors,put")
{
    hstring s;
    test_hstring_put(put_abi(s));

    REQUIRE(u8"abc" == s);
}

TEST_CASE("hstring,accessors,detach,hstring")
{
    hstring s{ u8"abc" };
    REQUIRE(!s.empty());

    xlang_string h = detach_abi(s);
    REQUIRE(s.empty());

    attach_abi(s, h);
    REQUIRE(!s.empty());
    REQUIRE(u8"abc" == s);

    hstring empty;
    h = detach_abi(empty);
    REQUIRE(nullptr == h);

    attach_abi(empty, h);
    REQUIRE(empty.empty());
}

TEST_CASE("hstring,accessors,detach,T")
{
    xlang_string h = detach_abi(u8"abc");

    hstring s;
    attach_abi(s, h);
    REQUIRE(u8"abc" == s);
}

//TEST_CASE("hstring,accessors,detach,std::wstring")
//{
//	xlang_string h = detach_abi(u8string(u8"abc"));
//
//	hstring s;
//	attach_abi(s, h);
//	REQUIRE(u8"abc" == s);
//}

TEST_CASE("hstring,accessors,copy_from")
{
    hstring from{ u8"abc" };
    hstring to;

    copy_from_abi(to, get_abi(from));
    REQUIRE(u8"abc" == to);
}

TEST_CASE("hstring,accessors,copy_to")
{
    hstring from{ u8"abc" };
    xlang_string to = nullptr;
    copy_to_abi(from, to);

    hstring copy;
    attach_abi(copy, to);
    REQUIRE(u8"abc" == copy);
}
//
// Comparisons
//

template <typename Left, typename Right>
static void test_compare()
{
    REQUIRE(Left(u8"abc") == Right(u8"abc"));
    REQUIRE_FALSE(Left(u8"abc") == Right(u8"abcd"));

    REQUIRE(Left(u8"abc") != Right(u8"abcd"));
    REQUIRE_FALSE(Left(u8"abc") != Right(u8"abc"));

    REQUIRE(Left(u8"abcd") > Right(u8"abc"));
    REQUIRE_FALSE(Left(u8"abc") > Right(u8"abcd"));

    REQUIRE(Left(u8"abc") < Right(u8"abcd"));
    REQUIRE_FALSE(Left(u8"abcd") < Right(u8"abc"));

    REQUIRE(Left(u8"abcd") >= Right(u8"abc"));
    REQUIRE_FALSE(Left(u8"abc") >= Right(u8"abcd"));

    REQUIRE(Left(u8"abc") <= Right(u8"abcd"));
    REQUIRE_FALSE(Left(u8"abcd") <= Right(u8"abc"));
}

TEST_CASE("u8string,compare,u8string")
{
    // This tests the test_compare function itself.
    // It is only here to serve as a reference point for the following tests.
    // There is nothing in production code that can affect this test.

    test_compare<u8string, u8string>();
}

TEST_CASE("hstring,compare,hstring")
{
    test_compare<hstring, hstring>();
}

TEST_CASE("hstring,compare,u8string")
{
    test_compare<hstring, u8string>();
}

TEST_CASE("u8string,compare,hstring")
{
    test_compare<u8string, hstring>();
}

TEST_CASE("hstring,compare,char8")
{
    test_compare<hstring, xlang_char8 const *>();
}

TEST_CASE("xlang_char8 const *,compare,hstring")
{
    test_compare<xlang_char8 const *, hstring>();
}

TEST_CASE("hstring,compare,u8string_view")
{
    test_compare<hstring, u8string_view>();
}

TEST_CASE("u8string_view,compare,hstring")
{
    test_compare<u8string_view, hstring>();
}

TEST_CASE("hstring,map")
{
    // Ensures that std::less<winrt::hstring> can be instantiated.

    std::map<hstring, int> m{ { u8"abc", 10 },{ u8"def", 20 } };
    REQUIRE(m[u8"abc"] == 10);
    REQUIRE(m[u8"def"] == 20);
}

TEST_CASE("hstring,unordered_map")
{
    // Ensures that std::hash<winrt::hstring> can be instantiated.

    std::unordered_map<hstring, int> m{ { u8"abc", 10 },{ u8"def", 20 } };
    REQUIRE(m[u8"abc"] == 10);
    REQUIRE(m[u8"def"] == 20);
}

static bool compare_hash(const u8string & value)
{
    return std::hash<u8string>{}(value) == std::hash<hstring>{}(hstring(value));
}

