#pragma once

//A version of CoCreatableCppWinRtClass that lets you pass in a uuid rather than getting it from a class property.
#define CoCreatableClassWithCLSIDWithFactory(className, uniquifier, clsid, factory) \
    InternalWrlCreateCreatorMap(className##uniquifier##_COM, clsid, nullptr, ::Microsoft::WRL::Details::CreateClassFactory<factory>, "minATL$__f")
#define CoCreatableCppWinRtClassWithCLSID(className, uniquifier, clsid) CoCreatableClassWithCLSIDWithFactory(className, uniquifier, clsid, ::wil::wrl_factory_for_winrt_com_class<className>)