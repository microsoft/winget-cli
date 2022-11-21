#include "pch.h"

using namespace xlang;

TEST_CASE("IXlangObject")
{
    struct Object : implements<Object, Windows::Foundation::IXlangObject>
    {
        
    };

    auto obj = make<Object>();
    REQUIRE(get_TypeName(obj) == "Object");
    REQUIRE(get_StringRepresentation(obj) == "Object");
    REQUIRE(get_ObjectSize(obj) == sizeof(Object));

    {
        auto obj2 = obj;
        REQUIRE(get_HashCode(obj) == get_HashCode(obj2));
    }
}
