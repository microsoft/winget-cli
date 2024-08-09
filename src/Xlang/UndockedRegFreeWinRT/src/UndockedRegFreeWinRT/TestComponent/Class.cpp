#include "pch.h"
#include "Class.h"
#include "ClassBoth.g.cpp"
#include "ClassSta.g.cpp"
#include "ClassMta.g.cpp"

namespace winrt::TestComponent::implementation
{
    int32_t ClassBoth::Apartment()
    {
        APTTYPE aptType;
        APTTYPEQUALIFIER aptQualifier;
        check_hresult(CoGetApartmentType(&aptType, &aptQualifier));
        return aptType;
    }

    void ClassBoth::Apartment(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    int32_t ClassSta::Apartment()
    {
        APTTYPE aptType;
        APTTYPEQUALIFIER aptQualifier;
        check_hresult(CoGetApartmentType(&aptType, &aptQualifier));
        return aptType;
    }

    void ClassSta::Apartment(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }

    int32_t ClassMta::Apartment()
    {
        APTTYPE aptType;
        APTTYPEQUALIFIER aptQualifier;
        check_hresult(CoGetApartmentType(&aptType, &aptQualifier));
        return aptType;
    }

    void ClassMta::Apartment(int32_t /* value */)
    {
        throw hresult_not_implemented();
    }
}
