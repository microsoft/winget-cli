#include "stdafx.h"

using namespace web;
using namespace utility;

namespace tests
{
namespace functional
{
namespace uri_tests
{
// testing resolution against examples from Section 5.4 https://tools.ietf.org/html/rfc3986#section-5.4
SUITE(resolve_uri_tests)
{
    // 5.4.1. Normal Examples https://tools.ietf.org/html/rfc3986#section-5.4.1
    TEST(resolve_uri_normal)
    {
        const uri baseUri = U("http://a/b/c/d;p?q");

        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g:h")), U("g:h"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g")), U("http://a/b/c/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("./g")), U("http://a/b/c/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g/")), U("http://a/b/c/g/"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("/g")), U("http://a/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("//g")), U("http://g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("?y")), U("http://a/b/c/d;p?y"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g?y")), U("http://a/b/c/g?y"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("#s")), U("http://a/b/c/d;p?q#s"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g#s")), U("http://a/b/c/g#s"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g?y#s")), U("http://a/b/c/g?y#s"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U(";x")), U("http://a/b/c/;x"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g;x")), U("http://a/b/c/g;x"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g;x?y#s")), U("http://a/b/c/g;x?y#s"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("")), U("http://a/b/c/d;p?q"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U(".")), U("http://a/b/c/"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("./")), U("http://a/b/c/"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("..")), U("http://a/b/"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("../")), U("http://a/b/"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("../g")), U("http://a/b/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("../..")), U("http://a/"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("../../")), U("http://a/"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("../../g")), U("http://a/g"));
    }
    // 5.4.2. Abnormal Examples https://tools.ietf.org/html/rfc3986#section-5.4.2
    TEST(resolve_uri_abnormal)
    {
        const uri baseUri = U("http://a/b/c/d;p?q");

        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("../../../g")), U("http://a/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("../../../../g")), U("http://a/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("/./g")), U("http://a/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("/../g")), U("http://a/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g.")), U("http://a/b/c/g."));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U(".g")), U("http://a/b/c/.g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g..")), U("http://a/b/c/g.."));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("..g")), U("http://a/b/c/..g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("./../g")), U("http://a/b/g"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("./g/.")), U("http://a/b/c/g/"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g/./h")), U("http://a/b/c/g/h"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g/../h")), U("http://a/b/c/h"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g;x=1/./y")), U("http://a/b/c/g;x=1/y"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g;x=1/../y")), U("http://a/b/c/y"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g?y/./x")), U("http://a/b/c/g?y/./x"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g?y/../x")), U("http://a/b/c/g?y/../x"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g#s/./x")), U("http://a/b/c/g#s/./x"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("g#s/../x")), U("http://a/b/c/g#s/../x"));
        VERIFY_ARE_EQUAL(baseUri.resolve_uri(U("http:g")), U("http:g"));
    }

} // SUITE(resolve_uri_tests)

} // namespace uri_tests
} // namespace functional
} // namespace tests
