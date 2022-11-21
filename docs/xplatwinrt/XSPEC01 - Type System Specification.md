---
title: Type System Specification
author: hpierson@microsoft.com
---

# Title: XSPEC01 - Type System Specification

- Author: Harry Pierson (hpierson\@microsoft.com)

- Status: Draft

## Abstract

This document describes the Xlang type system.

> _Note_: This specification was adapted from Windows Runtime specifications and still refers to fundamental interfaces that are part of the Windows Runtime and to various Windows-specific constructs. As equivalent Xlang constructs are defined, this specification needs to be updated to remove Windows-specific concepts & references and instead reflect the Xlang equivalents.

## General Notes

All types must be contained within a namespace. No types in the global namespace are allowed

Except for interfaces, all Xlang types must have public visibility. Xlang interfaces may optionally have private visibility. All other user-defined Xlang types (structs, classes, enums, delegates, attributes) must have public visibility.

Xlang does not support nested types. No Xlang type can enclose another type. No Xlang type can be nested inside another type

Xlang namespaces and type names are case preserving but insensitive. This means that you cannot have namespaces or type names that vary only by case. For example: you cannot have Foo.SomeType and foo.SomeType nor can you have Foo.SomeType and Foo.someType; however, you can have Foo.SomeType and foo.AnotherType, as they are considered part of the same namespace.

Xlang identifiers must conform to the following grammar. Note, only characters defined in Unicode 3.0 and earlier are supported.

```
identifier-or-keyword:
    identifier-start-character identifier-continuation-characters*

identifier-start-character:
    letter-character
    underscore (U+005F)

identifier-continuation-characters:
    identifier-continuation-character
    identifier-continuation-characters identifier-continuation-character

identifier-continuation-character:
    identifier-start-character
    decimal-digit-character
    connecting-character
    combining-character
    formatting-character

letter-character:
    A Unicode 3.0 character of classes Lu, Ll, Lt, Lm, Lo, or Nl

combining-character:
    A Unicode 3.0 character of classes Mn or Mc

decimal-digit-character:
    A Unicode 3.0 character of the class Nd

connecting-character:
    A Unicode 3.0 character of the class Pc

formatting-character:
    Zero Width Non-Joiner (U+200C)
    Zero Width Joiner (U+200D)
```

## Parameterized Types

Xlang supports type parameterization of interfaces and delegates. The parameterized types permit a family of interfaces to be defined that may be processed polymorphically in programming languages that support parametric polymorphism.

A parameterized type instantiation occurs when a parameterized type is specified with an argument list of types in a type context, such as a method parameter position. For example, "HRESULT foo(X\<Y> x)" instantiates the parameterized
type named by "X" with the type “Y" as its first and only type argument.

Unparameterized Xlang interfaces and delegates are assigned GUIDs to uniquely identify the underlying interface from other interfaces on the same object. Parameterized interfaces (eg IVector\<T>) and delegates (eg EventHandler\<T>)
are instead assigned a parameterized interface ID (PIID) - a GUID uniquely
identifying this parameterized type. This is not an IID. This GUID is used in
generating IIDs for parameterized type instances (eg IVector\<Int32>).

### Parameterized Types Arguments

The following Xlang types are permitted to appear in a parameterized type argument list:

- Xlang fundamental types (eg, Boolean, Int32, String, Guid, etc)

- Xlang enums

- Xlang structs

- Xlang interfaces

- Xlang delegates

- Xlang Runtime Classes

- other parameterized type instantiations (eg, IVector\<IVector\<Int32>>)

All other types are forbidden from appearing in a parameterized type argument list, such as:

- arrays

### Parameterized Type Instances

A parameterized type instance can appear in any context that a non-parameterized type can appear. Parameterized interface instances may be used anywhere an interface may be used, such as in the list of interfaces implemented by a runtime class. Parameterized delegate instances may be used anywhere a delegate may be used, such as in the definition of an event.

Parameterized type parameters can appear, in the parameterized interface or delegate definition, any place that a normal type can normally appear. If a parameter appears where only an interface can appear, that parameter is restricted to be an interface; if a parameter appears in a context where any type can appear, that parameter is not restricted; and so forth. Arguments to parameterized type instances must meet all such restrictions.

### Guid Generation for Parameterized Types

The guid generation algorithm must meet the following requirements:

1. When a parameterized type is instantiated twice with the same arguments, both instantiations must be assigned the same interface or delegate body and the same IID.

2. If two different parameterized types are instantiated, even with the same arguments, it must be statistically unlikely that they are assigned the same IID.

3. If a parameterized type is instantiated twice, but with different arguments, it must be statistically unlikely that they are assigned the same IID.

Remark: It is not permitted for a parameterized type to be instantiated with the same instantiation as an argument, or as an argument to any other parameterized types that appears in the argument list – ie, circular instantiation. For example, if X = Foo\<Y>, and Y = Foo\<X>, non-circularity has been violated. However, if X = Foo\<Y> and Y = Foo\<Int32>, non-circularity has been maintained.

The instantiation algorithm is as follows:

1. Each parameterized type is assigned a parameterized interface ID by its author – abbreviated PIID. (Remark: PIIDs are not IIDs, are not passed to QI as arguments, etc). The author must ensure that the PIID is unique to that p-type.

2. The type signature for a base type is an ASCII octet string, table appearing bellow. Eg, Int32 is "i4".

3. The type signature for an interface that is not a pinterface instance is its IID encoded in ascii in dashed form and surrounded in curly braces, eg: "{00000000-0000-0000-0000-000000000000}"

4. The type signature for a delegate that is not a p-delegate instance is the string "delegate" and the IID as with interfaces. Detailed grammar follows.

5. The guid for a parameterized type is computed via UUID rfc 4122, using a string generated via the following grammar:

```
signature_octets => guid_to_octets(wrt_pinterface_namespace)  
string_to_utf8_octets(ptype_instance_signature)

wrt_pinterface_namespace => "11f47ad5-7b73-42c0-abae-878b1e16adee"

ptype_instance_signature => pinterface_instance_signature | pdelegate_instance_signature

pinterface_instance_signature => "pinterface(" piid_guid ";" args ")"

pdelegate_instance_signature => "pinterface(" piid_guid ";" args ")"

piid_guid => guid

args => arg | arg ";" args

arg => type_signature

type_signature => base_type_identifer | com_interface_signature | interface_signature | delegate_signature | runtime_class_signature | struct_signature | enum_signature | pinterface_instance_signature | pdelegate_instance_signature

com_interface_signature => "cinterface(IInspectable)"

base_type_identifier is defined bellow

interface_signature => guid

runtime_class_signature => "rc(" runtime_class_name ";" default_interface ")"

default_interface => type_signature

struct_signature => "struct(" struct_name ";" args ")"

enum_signature => "enum(" enum_name ";" enum_underlying_type ")"

enum_underlying_type => type_signature

delegate_signature => "delegate(" guid ")"

guid => "{" dashed_hex "}"

dashed_hex is the format uuidgen writes in when passed no arguments:
    dashed_hex => hex{8} "-" hex{4} "-" hex{4} "-" hex{4} "-" hex{12}

hex => [0-9a-f]
```

6. When a p-type instantiation is passed as an argument to anther pinterface, the signature computed by step 3.a is used as the type signature in grammar element 'pinterface_instance_signature' or 'pdelegate_instance_signature', as appropriate

The following names are used for base types when they appear:

- UInt8 maps to "u1"

- Int32 maps to "i4"

- UInt32 maps to "u4"

- Int64 maps to "i8"

- UInt64 maps to "u8"

- Single maps to "f4"

- Double maps to "f8"

- Boolean maps to "b1"

- Char16 maps to "c2"

- String maps to "string"

- Guid maps to "g16"

The above names are case sensitive. Other than for String, the type name uses a single character to suggest the kind of data, followed by its size in bytes. These names where chosen to be concise (to avoid large sizes in struct type signatures), not appear confusingly similar to either the Xlang name, RIDL name, or language projection name for any type, and still remain roughly human readable.

For enum_type_signature, the only valid 'underlying_type' value is that of Int32 or UInt32.

For struct_type_signature, args is an in-order list of type_signatures for the fields of the struct. These may be base types or other struct types.

Struct_name and enum_name are namespace qualified, using period "." as delimiters. Eg, "namespace X { struct A{ int; }; }" becomes "struct(X.A;i4)"

Default_interface must be the interface, pinterface instance, delegate, or pdelegate instance that was specified as default in the Runtime Class or Interface Group body, using the IDL \[default] attribute.

Note that custom attributes are ignored, presumed to have no effect on marshaling.

## Versioning

All Xlang types except fundamental types must have a version attribute. Language projections use the version attribute info to enable back compatibility and light up scenarios. 3rd party types must include version attribute, but it must be ignored by language projections. 3rd party Xlang components are exclusively packaged in the app, so can never change versions independently of the app itself.

The version attribute may optionally be applied to interface members (methods, properties and events). This is intended for high-level class authoring in projections. Version attributes on interface members must be ignored at runtime by language projections.

The Version attribute includes an unsigned 32-bit integer constructor parameter. For binary compatibility, the value must increase each time new types are added. The specific sequencing is up to the type author. The first version does not need to be 1 or 0, and future versions can be any value greater than the previous as the author deems appropriate.

If a component intends to maintain binary compatibility, structs, delegates and interfaces are immutable once defined. They may never be modified in any subsequent version.

Enums and runtime classes are additively versionable. Enums may add new enum values in subsequent versions. Classes may add new implemented interfaces (including static, activation factory, composition factory, overridable and protected interfaces) in subsequent versions. Further details on additive versioning are included in the sections for enums and runtime classes.

## Namespaces

A namespace is a naming scope used to organize code and avoid naming collisions. All named types in the Xlang type system (enums, structs, delegates, interfaces, and runtime classes) live in a namespace. Namespaces can contain other namespaces.

## Fundamental Types

The Xlang Type system includes a core set of built-in primitive types.

Xlang Type | Type Description
---------- | ----------------------------------------------------------
Int16      | a 16 bit signed integer
Int32      | a 32 bit signed integer
Int64      | a 64 bit signed integer
UInt8      | an 8 bit unsigned integer
UInt16     | a 16 bit unsigned integer
UInt32     | a 32 bit unsigned integer
UInt64     | a 64 bit unsigned integer
Single     | a 32-bit IEEE 754 floating point number
Double     | a 64-bit IEEE 754 floating point number
Char16     | a 16-bit non-numeric value representing a UTF-16 code unit
Boolean    | an 8-bit Boolean value
String     | an immutable sequence of Char16s used to represent text
Guid       | A 128-bit standard Guid

## Enums

An enum type is a distinct value type with a set of named constants.

Each enum type has a corresponding integral type called the underlying type of the enum type. The only legal enum underlying types in Xlang are Int32 and UInt32.

Enums with an underlying type of UInt32 must carry the FlagsAttribute. Enums with an underlying type of Int32 must not carry the FlagsAttribute.

Enums must have public visibility.

### Enum Versioning

Enums are additively versionable. Subsequent versions of a given enum may add values (aka named constants). Pre-existing values may not be removed or changed. Enum values optionally carry the VersionAttribute to distinguish when specific values were added to the enum type. Enum values without a VersionAttribute are considered to have the same version value as the enclosing enum type.

## Structs

Structs are a record type with one or more fields. Structs are always passed and returned by value. Struct fields may only be enums, structs and fundamental types (including strings).

Structs must have public visibility.

Structs must have at least one field. All of a struct's fields must be public.

Structs cannot be generic or parameterized.

## Interfaces

An interface is a contract that consists of a group of related type members whose usage is defined but whose implementation is not. An interface definition specifies the interface's members – methods, properties and events. There is no implementation associated with an interface.

Non parameterized interfaces must have a unique interface ID (aka IID) specified via a GuidAttribute. Parameterized interfaces must have a unique parameterized interface ID (aka PIID) specified via a GuidAttribute. The PIID is used to generate an IID for a specific parameterized interface instance via the algorithm specified above.

Interfaces may have public or private visibility. This reflects the fact that some interfaces represent shared contracts implemented by multiple Xlang classes while other interfaces represent members implemented by a single Xlang class. Private visibility interfaces must specify the Xlang class they are exclusive to via the ExclusiveToAttribute. Private interfaces may only be implemented by the Xlang class specified in the ExclusiveToAttribute.

### IInspectable and IUnknown

All Xlang interfaces must inherit directly from IInspectable, which in turn inherits from IUnknown. IUnknown defines three methods: QueryInterface, AddRef and Release as per traditional COM usage. IInspectable defines three methods in addition to the IUnknown methods: GetIids, GetRuntimeClassName and GetTrustLevel. These three methods allow the object's client to retrieve information about the object. In particular, IInspectable.GetRuntimeClassName enables an object's client to retrieve an Xlang type name that can be resolved in metadata to enable language projection.

### Interface Requires

Interfaces may specify that they require one or more other interfaces that must be implemented on any object that implements the current interface. For example, if IButton required IControl then any class implementing IButton would also need to implement IControl.

Adding new functionality by implementing new interfaces that inherit from existing interfaces (i.e. IFoo2 inherits from IFoo) is not allowed.

### Parameterized Interfaces

Interfaces support type parameterization. A parameterized interface definition specifies a type parameter list in addition to the list of interface members and required interfaces. A required interface of a parameterized interface may share the same type argument list, such that a single type argument is used to specify the parameterized instance of both the interface and the interface it requires (eq IVector\<T> requires IIterable\<T>). The signature of any member (aka method, property or event) of a parameterized interface may reference a type from the parameterized interface’s type arguments list. (eq. IVector\<T>.SetAt(\[in] UInt32 index, \[in] T value)).

## Delegates

Delegates are Xlang types that act as a type-safe function pointer. They are essentially a simple Xlang object that exposes a single interface that inherits from IUnknown and defines of a single method named Invoke. Invoking the delegate in turn invokes the method it references. Delegates are often (but not exclusively) used for defining Xlang events.

Xlang delegates are named types and define a method signature. Delegate method signatures follow the same rules for parameters as interface methods do. The signature and parameter names of the Invoke method must match the definition of the delegate.

Like interfaces, non-parameterized delegates must have a unique interface ID (aka IID) specified via a GuidAttribute. The IID of the delegate is used as the IID of the single method interface used to implement the delegate. Parameterized delegates must have a unique parameterized interface ID (aka PIID) specified via a GuidAttribute. The PIID is used to generate an IID for a specific parameterized delegate instance via the algorithm specified above.

Delegates must have public visibility.

### IUnknown

Note that unlike Xlang interfaces, delegates do not implement IInspectable, only IUnknown. This means that they cannot be inspected for type information at runtime.

### Parameterized Delegates

Delegates support type parameterization. A parameterized delegate definition specifies a type parameter list in addition to the traditional method signature as specified above. In the method signature, any parameter may be specified as one of the types from the parameterized delegates' type arguments list.

## Interface Members

Xlang interfaces support three types of members: methods, properties and events. Interfaces may not have data fields.

### Methods

Xlang interfaces support methods which take zero or more parameters and return an HRESULT indicating the success or failure of the method call. Methods may optionally indicate a single out parameter to be projected as the return value in exception based languages. The return value out parameter, if specified, must be the last parameter in the method signature.

Methods must have public visibility.

Methods may not use variable numbers of arguments. Methods may not have optional parameters or parameters with default values.

Methods may not be parameterized. Parameterized delegates and methods of parameterized interfaces may use type parameters of the containing type in the method signature.

### Parameters

All method parameters except array length parameters (described below) must have a name and a type. Note that return values must specify a name just like parameters do. Method parameter names, including the return type name, must be unique within the scope of the method.

Only parameters for parameterized delegates and members of parameterized interfaces may specify a parameterized type for the parameter type. Methods may not be individually parameterized. Parameters may always specify parameterized type instances (eg IVector\<Int32>) as the parameter type.

All method parameters must be exclusively in or out parameters, In/out parameters are not supported.

While all methods on Xlang interfaces must return an HRESULT, methods may optionally indicate that their final out parameter is intended to be used as the return value when the method is projected into exception based languages. Such parameters are known as \[out,retval] parameters after the MIDL syntax used to declare them. When an \[out, retval] parameter is specified, it must be the last parameter in the method signature.

Other than \[out, retval] position at the end of the parameter list, there are no other ordering requirements for out parameters.

#### Array Parameters

Xlang methods support conformant array parameters. Arrays can never be used except as parameters. They cannot be stand-alone named types and they cannot be used as a struct field type. Array parameters can be used as in, out and retval parameters.

Xlang supports array parameters of most Xlang types including fundamental types (including string and guid), structs, enums, delegates, interfaces and runtime classes. Arrays of other arrays are not allowed.

Because they are conformant, arrays parameters must always be immediately preceded in the parameter list by a parameter for the array size. The array size parameter must be a UInt32\. The array size parameter does not have a name.

Xlang supports three different array passing styles:

- PassArray – this style is used when the caller is providing an array to the method. In this style, both the array size parameter and array parameter are both in parameters

- FillArray – this style is used when the caller is providing an array for the method to fill, up to a maximum array size. In this style, the array size parameter is an in parameter while the array parameter is an out parameter. When using the FillArray style, the array parameter may optionally specify one of the other parameters as the array length parameter. Details below in section _1.10.2.2_.

- ReceiveArray – this style is used when the caller is receiving an array that was allocated by the method. In this style, the array size parameter and the array parameter are both out parameters. Additionally, the array parameter is passed by ref (i.e. ArrayType_* rather than ArrayType_).

- Note, the combination of out array size parameter and in array parameter is not valid in Xlang.

When an array parameter is used as an \[out,retval] parameter, the array length parameter must be an \[out] parameter – that is, only the ReceiveArray style is legal for retval arrays.

#### Array Length Parameter

A FillArray style array parameter may optionally specify another parameter as the array length parameter. Where the required array size parameter specifies the maximum number of elements in an array provided by the caller, the array length parameter specifies the number of elements that were actually filled in by the callee.

Array length parameter is specified with the LengthIs attribute on the array parameter.

### Method Overloading

Within the scope of a single interface, more than one method may have the same name. Methods with the same name on an interface must have unique signatures. Properties and Events cannot be overloaded

Xlang supports overloading on parameter types but favors overloading on the number of input parameters – aka the method's arity. This is done in order to support dynamic, weakly typed languages (aka JavaScript & Python).

When an interface has multiple methods of the same name and number of input parameters, exactly one of those methods must be marked as the default. Of all the overloaded methods with the same name and number of input parameters, only the method marked as the default will be projected by a dynamic, weakly typed language. If there is only a single overloaded method of a given name and number of input parameters, marking it as the default is supported, but not required.

For the purposes of determining a method's arity, array parameters and their required length parameter are consider a single parameter. The PassArray and FillArray styles are considered a single input parameter while the ReceiveArray style is considered a single output parameter.

When multiple methods on an interface have the same name, a unique name for each colliding method must be stored in an OverloadAttrbiute attached to the method. Default overloaded methods carry the DefaultOverloadAttribute.

#### Operator Overloading

Xlang does not support operator overloading. Methods may not be named using the special operator names such as op_Addition that are specified in the ECMA 335 CLI spec, partition I, section 10.3.

### Properties

Properties are a paired get/set methods with matching name and type that appear in language projections as fields rather than method.

Properties and their get/set methods must have public visibility.

Properties must have a get method. A property getter method has no parameters and returns a value of the property type. Properties with only a get method are called read only properties.

Properties may optionally have a set method. A property setter method has a single parameter of the property type and returns void. Properties with both a get and set method are called read/write.

Properties may not be parameterized. Properties from parameterized interfaces may use type parameters of the containing type as the property type.

### Events

Events are paired add/remove listener methods with matching name and delegate type. They are a mechanism for the interface to notify interested parties when something of interest happens.

Events and their add/remove listener methods must have public visibility.

An event add listener method has a single parameter of the event delegate type and returns an Xlang.Foundation.EventRegistrationToken. An event remove listener method has a single parameter of the Xlang.Foundation.EventRegistrationToken type and returns void.

Events may not be parameterized. Events from parameterized interfaces may use type parameters of the containing type as the event delegate type.

## Runtime Classes

Xlang allows you to define a class. A class must implement one or more interfaces. A class cannot implement type members directly (i.e. they can't define their own methods, properties or events). A class must provide an implementation all the members of all the interfaces it implements.

There are several different types of interfaces, which will be described in detail below.

- Member interfaces (including protected and overridable interfaces)

- Static Interfaces

- Activation Factory Interfaces

- Composition Factory Interfaces

Runtime classes cannot be parameterized. A runtime class may implement a parameterized interface instance (aka a parameterized interface with all its type parameters specified) anywhere it would typically accept a non-parameterized interface.

Runtime classes must have public visibility.

Runtime classes may only implement interfaces that are not exclusive (i.e. don't carry the exclusiveTo attribute) or that are exclusive to the runtime class in question. Runtime classes may not implement interfaces that are exclusive to a different runtime class. There is one exception to this rule – composable classes may implement interfaces that are exclusive to a class in their derivation chain that is marked as overridable. Details on overridable interfaces to follow.

### Member Interface

Runtime classes may implement zero or more member interfaces. Member interfaces enable classes to expose functionality that is associated with instances of the class. Runtime classes specify a list of the member interfaces they implement. Entries in the list of member interfaces implemented by a runtime class may optionally carry version information. Details on runtime class versioning to follow.

Member interfaces are implemented directly on instances of the runtime class.

Runtime classes that implement one or more member interfaces must specify one of the member interfaces to be the default interface. Runtime classes that implement zero member interfaces do not specify a default interface.

### Static Interfaces

Xlang classes may implement specify zero or more static interfaces. Static interfaces enable classes to expose functionality that's associated with the class itself rather than with specific instances of the class.

A class must specify at least one member or static interface. A class with no member and no static interfaces is invalid.

Static interfaces are specified via a StaticAttribute associated with the runtime class. StaticAttribute carry a reference the static interface reference as well as version information. Details on runtime class versioning to follow.

While static interfaces are declared as part of the runtime class, they are actually not implemented on class instances themselves. Rather, they are implemented on the class's activation factory. Details on activation factories to follow.

### Activation

Runtime classes optionally support activation – the ability of the system to produce instances of a specified class. Classes must implement at least one member interface in order to support activation.

Xlang defines three activation mechanisms: direct activation (with no constructor parameters) and factory activation (with one or more constructor parameters) and Composition activation. Non composable classes may support either direct and/or factory activation. Composable classes only support composable activation. Details on composition and composable activation to follow.

Classes that support direct activation are activated by calling the IActivationFactory.ActivateInstance method on the class's activation factory. This method takes no parameters and returns a newly activated instance of the runtime class. Details on activation factories to follow.

Classes that support factory activation define one or more factory interfaces which each in turn define one or more factory methods. These factory interfaces are implemented on the class's activation factory. Details on activation factories to follow.

Factory methods take one or more \[in] parameters and must return a newly activated instance of the runtime class. Other out parameters beyond the newly activated class instance are not allowed. Factory methods must take one or more parameters – parameterless factory activation is not allowed. Direct activation must be used for parameterless activation.

Classes that support either direct or factory activation are marked with the ActivatableAttribute. The ActivationAttribute carries version information (details on runtime class versioning to follow) as well as an optional reference to the factory interface. Classes can be marked with multiple ActivatableAttributes – at most one for default activation plus one for every factory interface implemented by the classes' activation factory. Classes marked with the activatableAttribute may not also be marked with the ComposableAttribute. Details on composition to follow.

### Composition

Runtime classes optionally support composition – the ability for multiple class instances to be combined into what appears to be a single object from the outside. Xlang uses composition as a form of runtime class inheritance.

Xlang classes can optionally compose a single composable base class, which in turn may compose a single composable base class, etc. A class does not itself need to be composable in order to compose a composable base class. Classes may only compose with a composable class as a base class. A composable class is not required to compose another composable class (i.e. it may be the root of the hierarchy). Circular graphs of composition (aka A composes B which composes A) are not allowed.

At runtime, a composing class is an aggregation of Xlang objects – one for each object in the composition chain. These aggregated objects delegate identity and lifetime to the originally activated object in the composition chain (called the controlling object). Every object in the chain holds a non-delegating IInspectable pointer to the class it composes in order to call methods on composed base class interfaces, including methods on protected interfaces. Every object in the chain has a pointer to the controlling class for delegating lifetime and identity as well as in order to call methods on overridable interfaces. Details on protected and overridable interfaces to follow.

For example, assume Button composes Control, which in turn composes UIElement. An instance of Button would aggregate a Control instance which in turn would aggregate a UIElement instance. All three objects would have a reference to the Button object for controlling lifetime and identity as well as for querying for overridable interfaces. Each object would have an IInspectable pointer to the object it composes (Button holds a pointer to Control, Control holds a pointer to UIElement) in order to be able to call methods on interfaces implemented on composed classes, including protected interfaces.

Classes may not implement any interfaces defined on the class it composes or any class in the composition chain unless the interface is marked as overridable in the composable class. Details on overridable interfaces to follow.

Composable classes must be marked with one or more ComposableAttributes. The ComposableAttribute carries a reference to the composition factory interface, whether the factory methods on the composition factory interface can be used for controlling object activation or not as well as version information. Details on composition factory interfaces and versioning to follow. Classes can be marked with multiple ComposableAttributes – one for every composition factory interface implemented by the classes' activation factory.

#### Composable Activation

Composable classes must define one or more composition factory interfaces, which in turn implement one or more composition factory methods. Composition factory interfaces are implemented on the class's activation factory. Details on activation factories to follow.

Composition factory interfaces are used to create composable instances of the class. Composable factory interfaces declare zero or more composable factory methods that can be used to activate instances of the class for composition purposes. Note that it is legal to have a composable factory interface with zero factory methods. This implies that the class can be used for composition, but that 3rd parties may not directly compose the class – the method(s) to create instances are internal only.

Composable classes declare if the factory methods on a given composition factory interface can be used to activate the class directly as a controlling object or not. Composable factory interfaces marked as public may be used to directly activate a class as a controlling object as well as indirectly to activate a class as a composed object. Composable factory interfaces marked protected may only be used to indirectly activate a class as a composed object. Composable classes can always be activated as composed objects.

Composition factory interfaces must be exclusive to the runtime class they are implemented by.

Like activation factory methods, composition factory methods must return an instance of the composable class. Additionally, composition factory methods have two additional parameters: the controlling IInspectable \[in] parameter and the non-delegating IInspectable* \[out] parameter. Composition factory methods may optionally have additional in parameters. If specified, the additional in parameters must occur at the beginning of the method signature, before the mandated parameters listed previously. Composition factory methods may not have any additional out parameters beyond the non-delegating IInspectable and the return value parameters.

When a composable class is activated for composition (i.e. Control or UIElement when a Button instance is activated), a pointer to the object that controls identity and lifetime is passed in via the controlling IInspectable \[in] parameter. The composable factory method returns the newly activated instance as the return value. This instance delegates all identity and lifetime management functionality to the controlling IInspectable that was provided. Additionally, the composable factory method returns a pointer to a non-delegating IInspectable* that the composing class can use to invoke methods on A composed class.

When a composable class is activated as a controlling class (i.e. Button in our previous example), the same composable factory methods are used as for activation for composition. When activating a composable class directly, null is passed for the controlling IInspectable parameter. This is an indicator to the composable class that it is being activated as a controlling class. When a controlling class creates the instance of the class it composes, it passes a reference to itself as the controlling IInspectable parameter. The composable factory method returns the controlling class instance as the return value. The non-delegating IInspectable* \[out] parameter is ignored by client code when activating a controlling composable class.

Building on the earlier Button -> Control -> UIElement example, the button class would be activating by calling one of its composition factory methods and passing null for the outer parameter. Button would in turn activate a control instance, passing a reference to itself as the outer parameter. Control would in turn activate a UIElement instance, passing the outer reference it received as the outer parameter. The UIElement factory method would return to Control the newly created UIElement in the instance parameter as well as a reference to UIElement's non-delegating IInspectable in the inner parameter. The Control factory method would return to Button the newly created Control in the instance parameter as well as a reference to Control's non-delegating IInspectable in the inner parameter. The Button control factory would return to the calling code the newly created Button in the instance parameter and null for the inner parameter.

Note, it is possible for a class to be sometime activated for composition and other times activated as the controlling class. For example, if RadioButton composed Button, then Button would be activated for composition when a RadioButton was activated, but activated as the controlling class when Button was activated directly. In either case, the Control class that Button composes would be activated for composition.

#### Protected Interfaces

Composable classes may declare zero or more of its member interfaces to be protected. Non composable classes may not declare member interfaces to be protected. Only code in classes that compose a composable class (directly or indirectly) may query for and use interfaces that the composable class declares as protected. Code from outside the composition chain may not query for or use interfaces that the composable class declares as protected.

Example: If UIElement declares a protected interface IUIElementProtected, then only classes that compose UIELement, including both direct (Control) and indirect (Button) composition may query for and use the IUIElementProtected interface.

#### Overridable Interfaces

Composable classes may declare zero or more of its member interfaces to be overridable. Overridable interfaces may only be queried for and used within a composition chain – similar to the rules about accessing protected interfaces detailed previously. However, where protected interfaces may only be implemented by the class that originally declared it implemented the interface, overridable interfaces may be re-implemented by classes that compose the class that declared it implemented the overridable interface.

At runtime, any composable class code that leverages the overridable interface must QI for the interface via the controlling IInspectable* pointer that is used for identity and lifetime delegation. This pointer will return the implementation of the overridable interface earliest in the composition chain (i.e. closest to the controlling class instance). Classes that wish to access the overridable interfaces of the class they compose may do so via the non-delegating reference that composable classes hold to their composed class.

Example: If UIElement declares an overridable interface IUIElementOverridable, then classes that derive from UIELement, including both direct (Control) and indirect (Button) derivation, would be allowed to implement it. If code in UIElement needed to access functionality in IUIElementOverridable, then UIElement would query the controlling IInspectable to get the earliest implementation in the composition chain. If Control and Button both implemented IUIElementOverridable, then the Button implementation would be returned when the controlling IINspectable was queried. If Button wants to access its composed class functionality, it can use the non-delegating IInspectable that was returned from the composition factory method to query the base class for that interface.

### Activation Factories

Runtime classes optionally have an activation factory. Runtime classes must have an activation factory if the class is activatable, composable or has static interfaces. The activation factory for a class can be retrieved from the system at runtime via the Xlang PAL.

Activation Factories must implement the IActivationFactory interface. However, only classes that support direct activation provide an implementation of IActivationFactory's single method ActivateInstance. Classes that don't support direct activation must return E_NOTIMPL from IActivationFactory.ActivateInstance.

The activation factory must implement all activation factory interfaces, composition factory interfaces and static interfaces defined on the runtime class.

There is no guarantee that language projections maintain a single activation factory instance for the lifetime of the factory. Xlang class authors that need to save long-lived information for static member access need to store it somewhere outside of the activation factory.

### Class Based Projection

While Xlang is primarily an interface based programming model under the hood, runtime classes provide a class based programming model that is better aligned to modern, mainstream, OO programming languages. Language projections are expected to project a runtime class as a single entity, rather than as a bag of interfaces the developer has to deal with separately.

To achieve this class based model, language projections are expected to project type members from a class's member interfaces as direct class members. Language projections are expected to project type members from a class's static interfaces as static class members. Finally, language projections are expected to project activation methods (direct activation as well as interfaces from factory and composable factory interfaces) as class constructors.

To assist in this class based projection, metadata for runtime classes specify a class member for all the methods, properties and events from every interface they implement. Every class member is explicitly tied back to the interface member where it was originally defined. This allows language projections to expose the class as a single entity, handling all the interface querying and reference counting under the covers on behalf of the developer.

By default, every implemented interface member is projected as a class member. However, because runtime classes can implement multiple independent interfaces and version over time (versioning details to follow), it is possible for there to be name collisions for members defined on different interfaces implemented by a single runtime class.

When collisions occur, default class member projection is impossible. If collisions occur across interfaces added in separate versions, the colliding member from the earliest version is projected as a class member. When Collisions occur across interfaces added in the same version, none of the colliding members are projected as class members. Note, methods with colliding names are permitted so long as all the versions are of different arity as described in _Method Overloading_.

Interface members that are not projected as class members must be made available to developers. Typically, this is a done by a casting or dynamic lookup operator allowing the developer to specify the specific interface and method they want to invoke.

In order to resolve method name collisions, runtime classes may specify alternative names for methods on the member and static interfaces they implement. This alternative name is used by the language projection to provide disambiguated access to colliding method names from a class instance. While the runtime class may provide an alternative method name, the method signature, parameters and any attributes attached to the method or its attributes must still match the original interface definition exactly.

Since direct activation, factory methods and composition factory methods are projected as class constructors, they are all projected on the runtime class as if they have the same name. All methods across all factory interfaces must have unique signatures, should favor arity based overloading over type based overloading, and must use the DefaultOverloadAttribute to disambiguate factory methods of the same arity.

### Class Versioning

Runtime classes are additively versionable. Subsequent versions of a given runtime class may specify additional interfaces of all types, with further details on individual interface types below. Pre-existing interfaces specified by a class may never be removed or changed without breaking backwards compatibility.

#### Member Interface Versioning

Member interfaces on runtime classes are additively versionable. Subsequent versions of a given runtime class may implement additional member interfaces, even if the class had never implemented member interfaces previously. Subsequent versions of a given composable runtime class may implement additional protected and overridable interfaces.

Interfaces implemented by a runtime class optionally carry the VersionAttribute to distinguish when specific interfaces were added to the runtime class type. Interface implementation values without a VersionAttribute are considered to have the same version value as the enclosing runtime class type.

#### Static Interface Versioning

Static interfaces on runtime classes are additively versionable. Subsequent versions of a given runtime class may implement additional static interfaces, even if the class had never implemented static interfaces previously.

The StaticAttribute includes a UInt32 parameter for version number, which defines the version that added that activation support.

#### Activation Versioning

Activation support for runtime classes is additively versionable. Subsequent versions of a given runtime class may implement additional activation mechanisms, even if the class had never implemented an activation mechanism. Note, composable classes are not activatable and thus may not add activation support.

Note, classes that support direct activation may only add new factory activation interfaces. Classes that previously only supported factory activation may add direct activation support as well as new factory activation interfaces.

The ActivatableAttribute includes a UInt32 parameter for version number. The version number for the ActivatableAttribute defines the version that added that activation support.

#### Composition Versioning

Composition support for runtime classes is additively versionable. Subsequent versions of a given composable runtime class may implement additional composition mechanisms, provided the class was defined as composable when it was created. Composable classes may not add activation support.

The ComposableAttribute includes a UInt32 parameter for version number. The version number for the ComposableAttribute defines the version that added that composition support.

## Custom Attributes

Xlang supports the definition of custom metadata attributes. All constructs in the Xlang type system can carry custom metadata attributes. This includes all named types (enums, structs, delegates, interfaces, classes, etc.) as well as individual elements contained within type constructs (such as methods, parameters, etc).

Custom attributes are named like other Xlang types. However, they are not activatable. They are a purely data construct.

Custom attributes define a data schema of either positional parameters or named fields. A custom attribute may not use both positional parameters and named fields – they must choose one or the other. The types of a custom attribute's parameters and fields are limited to the Xlang fundamental types, enums and references to other Xlang types. No other parameter or field type is allowed.

Custom attributes that use positional parameters must define one or more valid sets of positional parameters. Each set must specify zero or more positional parameters. Instances of the custom attribute must specify a single set of positional parameters as well as data for each positional parameter in the selected set.

Custom attributes that use named fields specify zero fields with names and types. Instances of the custom attribute must specify the name/value pairs for the fields they wish to specify. Instances may specify values for all, some or none of the name/value pairs

An attribute may have neither positional parameters nor named fields.

Custom attributes must have public visibility.

Attributes may specify the types of Xlang type constructs they may be associated with via the AttributeUsageAttribute.
