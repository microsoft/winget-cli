---
id: XDN03
title: xlang Type System
author: hpierson@microsoft.com
status: draft
---

# Title: XDN03 - xlang Type System

- Author: Harry Pierson (hpierson@microsoft.com)
- Status: draft

## Abstract

This design note describes the xlang's [type system](https://en.wikipedia.org/wiki/Type_system).
The type system is designed to interoperate with a variety of object-oriented programming
languages in a natural and familiar way.

## Philosophy

xlang is designed to interoperate with many different object-oriented programming languages. Some
of the languages xlang is considering at this time include (but is not limited to):

- [C++11](https://isocpp.org)
- [Java](https://www.oracle.com/java) and [Kotlin](https://kotlinlang.org)
- [JavaScript](https://developer.mozilla.org/en-US/docs/Web/JavaScript)  
- [.NET](https://dotnet.microsoft.com), in particular
  [C#](https://docs.microsoft.com/en-us/dotnet/csharp/language-reference/) and
  [Visual Basic](https://docs.microsoft.com/en-us/dotnet/visual-basic/language-reference/)
- [Objective-C](http://en.wikipedia.org/wiki/Objective-C) and [Swift](https://swift.org)
- [Python](https://python.org)
- [TypeScript](https://www.typescriptlang.org/)

These languages represent a wide variety of philosophical differences in type system design.
Strong vs. weak types. Compiled vs interpreted languages. Static vs dynamic typing. Gradual typing.
Designing a single type system that can work in such varied environments inevitably leads to
compromises that an individual language designer would never face.

## Type Categories

### Namespaces

A namespace is a naming scope used to organize code and avoid naming collisions. All type
categories in the xlang type system (enumerations, structs, delegates, interfaces, and runtime classes)
except for fundamental types live in a namespace.

Namespaces may contain other namespaces.

xlang namespaces and type names are case preserving but insensitive. This means that you cannot
have namespaces or type names that vary only by case. For example: you cannot have `Foo.SomeType`
and `foo.SomeType` nor can you have `Foo.SomeType` and `Foo.someType`. However, you can have
`Foo.SomeType` and `foo.AnotherType`. In this case `SomeType` and `AnotherType` would be considered
as belonging to the same namespace.

### Fundamental Types

The xlang Type system includes a core set of built-in primitive types.

xlang Type | Type Description
---------- | ----------------------------------------------------------
Int8       | 8 bit signed integer
Int16      | 16 bit signed integer
Int32      | 32 bit signed integer
Int64      | 64 bit signed integer
UInt8      | 8 bit unsigned integer
UInt16     | 16 bit unsigned integer
UInt32     | 32 bit unsigned integer
UInt64     | 64 bit unsigned integer
Single     | 32-bit IEEE 754 floating point number
Double     | 64-bit IEEE 754 floating point number
Char16     | 16-bit non-numeric value representing a UTF-16 code unit
Boolean    | 8-bit Boolean value
String     | immutable sequence of Char16s used to represent text
Guid       | 128-bit standard [universally unique identifier](https://en.wikipedia.org/wiki/Universally_unique_identifier)
Object     | xlang object of unknown type

> Open Issues
> - String encoding has not been settled. WinRT originally specified UTF-16 character type and strings.
  However, UTF-8 has become the dominant encoding on the web since WinRT was originally designed.
  This work is tracked by [xlang issue #53](https://github.com/Microsoft/xlang/issues/53).
> - xlang should have a pointer sized primitive type, analogous to .NET's
  [IntPtr](https://docs.microsoft.com/en-us/dotnet/api/system.intptr) type.

### Enumerations

An enumeration (or enum) type is a set of named values. Each named value in an enum corresponds to
a constant integer value.

Each enumeration type has an underlying integral type. The only legal underlying integral types for enums
in xlang are Int32 and UInt32.

An enumeration type with an underlying integral type of UInt32 is considered a Flags enum. Flags enums
are intended to be treated as a bit field. Language projections can and should provide logical
operators such as and, or and not for Flags enum values.

Enumerations are additively versionable. Subsequent versions of an enumeration may add new named
values (and associated constant integer values). Pre-existing values may not be removed or changed.

> Open Issues
> - can xlang expand the types usable for enums, as well allowing other unsigned types to be flag
  enums. 64bit bit flags as well as unsigned enums that aren't flags (addresses) have been asked about.
> - versioning in xlang needs its own XDN

### Structs

Structs are a record type with one or more fields. Structs are always passed and returned by value.
Struct fields may only be enums, other structs and fundamental types (including strings).

Structs must have at least one field. All of a struct's fields are public.

Structs cannot be generic or parameterized.

Structs are not versionable. Once they have been defined, they may never be changed.

### Interfaces

An interface is a type that acts as a contract that can have multiple implementations. An
interface specifies the signatures for a group of object members (aka methods, properties and
events) but does not specify an implementation for those object members.

Interfaces can be implemented by multiple different classes. Each class implementation of a given
interface's object members is unrelated to any other class's implementation of the same interface.

All xlang interfaces need an GUID identifier. This identifier has no relevance at the type system
level, it is only used at the ABI level. An interface's GUID can either be explicitly provided or
it can be implicitly generated from the interface's fully namespace qualified name using the
algorithm defined in [RFC4122 section 4.3](https://tools.ietf.org/html/rfc4122#section-4.3).

Interfaces are not versionable. Once they have been defined, they may never be changed.

#### Interface Inheritance

Interfaces may optionally inherit from a single other interface. All of the type members of the
base interface are considered part of the derived interface as well. Any class implementing an
interface that uses inheritance must implement all of the type members specified in all of the
interfaces in the inheritance tree.

Note, WinRT interfaces do not support inheritance. WinRT interfaces may optionally declare that
they require one or more other interfaces to be implemented by any class that implements it. For
example, any type that implements [IStorageFile](https://docs.microsoft.com/en-us/uwp/api/Windows.Storage.IStorageFile)
must also implement [IStorageItem](https://docs.microsoft.com/en-us/uwp/api/windows.storage.istorageitem),
[IInputStreamReference](https://docs.microsoft.com/en-us/uwp/api/windows.storage.streams.iinputstreamreference)
and [IRandomAccessStreamReference](https://docs.microsoft.com/en-us/uwp/api/windows.storage.streams.irandomaccessstreamreference).

WinRT's interface requires approach leads to inefficiencies at the ABI layer - both runtime overhead
from calling QueryInterface as well as memory overhead from having multiple interface virtual memory
tables. By limiting interfaces at the type system level to single inheritance, xlang can implement
them at the ABI layer using a single composite virtual method table, avoiding the runtime and memory
overhead described above.

> Open Issues
> - Given the WinRT IStorageFile example above, can xlang get away with only not supporting
  interface requires?

#### Parameterized Interfaces

xlang interfaces support type parameterization. Parameterized interfaces specify one or more type
parameters that can be used as parameter types or return values of type members. Parameterized
interface members may declare argument and/or return value types in terms of the type parameters
instead of as concrete types.

Like non-parameterized interfaces, parameterized interfaces require a GUID identifier. This GUID can
be explicitly specified or implicitly generated from the type name.

Parameterized interfaces can inherit from both parameterized and non-parameterized interfaces.
When inheriting from a parameterized interface, the type parameters of the base interface must
be specified, though they may be specified in terms of the derived interface's type parameters.

A concrete interface is either a non-parameterized interface or a parameterized interface where all
of the type parameters have been fully specified. Non-parameterized interfaces may inherit from
concrete interfaces, but they may not inherit from a parameterized interface.

For example, [xlang's foundation types](XDN07%20-%20xlang%20Foundation%20Types) includes a set of
parameterized interfaces to represent collections.

- `IIterable<T>` represents a collection that can be enumerated
- `IVector<T>` represents a collection that can be randomly accessed
- `IMap<K,V>` represents a collection that maps from keys to values

`IIterable<T>` is a parameterized interface. `IIterable<String>` is a concrete interface, as all
of the type parameters have been specified.

Both `IVector<T>` and `IMap<K,V>` can be enumerated. `IVector<T>` derives from `IIterable<T>`.
In this case, the same type parameter is used for both `IVector` and `IIterable`. However,
`IMap<K,V>` derives from `IIterable<IKeyValuePair<K,V>>`. In this case, the `T` type parameter of
`IIterable` is instantiated as parameterized type `IKeyValuePair<K,V>`, using the type parameters
defined as part of of `IMap<K,V>`.

Using the same collection interfaces defined above, you could define an non parameterized interface
`IPropertySet` that inherits from `IMap<String, Object>`. Note the use of concrete types in the
declaration of IMap, where in the previous example we used type parameters.

### Delegates

Delegates are xlang types that act as a type-safe function pointer. They are similar conceptually
to an [interface](#interfaces) that declares one and only one method. Defining delegates as a
separate type from interfaces enables better integration with free functions and/or [anonymous
functions](https://en.wikipedia.org/wiki/Anonymous_function) in language projections. Delegates are
often (but not exclusively) used to declare the signature for xlang event handlers.

xlang delegates are named types and define a method signature. Delegate method signatures follow the
same rules for parameters as interface methods do.

Like interfaces, delegates require a GUID identifier. This identifier can be provided explicitly or
generated implicitly from the type's name.

Like interfaces, delegates can be parameterized. Parameterized delegates may declare arguments
and/or their return value type in terms of the type parameters instead of as concrete types.

Delegates are not versionable. Once they have been defined, they may never be changed.

### Classes

Classes are xlang types that expose behavior and encapsulate data.

Classes have zero or more object members (i.e. Methods, Properties and Events).

Classes may be activatable (i.e. directly constructible via a language specific mechanism like
C++ "new"). Activatable classes provide one or more activation methods (i.e. constructors).
Activation methods take zero or more `in` parameters and return a newly activated instance of the
class. Neither `out` parameters nor a different return value are allowed for activation methods.
Multiple activation methods are allowed, providing each method has a unique signature. As described
in the Method Overloading section below, xlang generally prefers arity-based (i.e. number of
parameters) to type based (i.e. types of parameters) overloading. All activation methods for a
given class are considered to have the same name from the perspective of overloading.

Classes may have zero or more instance members as well as zero or more static members. Instance
members are members that are called on an activated instance of the class while static members are
called on the class itself.

Classes can implement zero or more interfaces. As described above, interfaces define a set of type
members. Classes that implement a given interface much implement all the members of the interface.

Classes can not be parameterized. Classes may implement parameterized interfaces, but only if all
type parameters are specified. For example, a `JsonArray` type could not implement `IVector<T>` but
it could implement `IVector<JsonValue>`.

Classes are additively versionable. Subsequent versions of a given enum may add new members.
Pre-existing members may not be removed or modified in any way.

#### Class Inheritance

TBD

## Object Members

Interfaces and Classes can define zero or more members - Methods, Properties and Events.

### Methods

Methods take zero or more parameters and a single optional return type.

Methods must have a concrete number of arguments. Methods may not have optional parameters, nor
parameters with default values.

Methods may not be parameterized. Parameterized delegates and methods on parameterized interfaces
may use the type parameters of the containing type in the method signature.

#### Parameters

All method parameters must have a name and a type. Method parameter names, including the return type
name, must be unique within the scope of the method.

Only parameters for parameterized delegates and members of parameterized interfaces may specify a
parameterized type for the parameter type. Methods may not be individually parameterized.

Method parameters default to being `in` parameters (i.e. they provide information to the method), but
can optionally be marked as `out` parameters (i.e. the act as an additional return value from the method).
`in\out` parameters are not supported.

##### Array Parameters

Object members may optionally support arrays for parameter and return value types. Array parameters
are a block of memory holding an specified number of elements of a given type.

xlang supports array parameters of most xlang types including fundamental types, structs,
enumerations, delegates, interfaces, and classes. Arrays of other arrays are not allowed.

WinRT supports three different array-passing styles.

- `PassArray` style is used when the caller provides an array for the the member read from.
- `FillArray` style is used when the caller provides an array for the member to fill, up to a
  maximum array size.
- `ReceiveArray` style is used when the caller receives an array that was allocated by the member.

For the purposes of determining a method's arity (described in the method overloading section below),
only `PassArray` and `FillArray` are considered input parameters.

Array return values may only use the `ReceiveArray` style.

Arrays may not be used as stand-alone types, struct fields types or as type parameters for
parameterized types.

#### Method Overloading

Within the scope of a single type, more than one method may have the same name. Methods with the
same name on a type must have unique signatures. Only methods can be overloaded.

xlang supports overloading on parameter types, but favors overloading on the number of input
parameters - also known as the method's arity. This is done in order to support dynamic,
weakly-typed languages such as JavaScript.

Classes may not implement combinations of interfaces that create invalid method overload
combinations. For example, assuming the existence of both an `ICowboy` and `IArtist` interface
each with a `Draw()` method, it would not be valid to create a `CowboyArtist` class that
implemented both interfaces since there would be two methods with the same signature named `Draw`.

### Properties

A property is a pair of get/set methods with matching name and type that appear in some language
projections as fields rather than methods.

A property must have a get method. A property getter method has no parameters, and returns a value
of the property type. A property with only a get method is called a read-only property.

A property may optionally have a set method. A property setter method has a single parameter of the
property type, and has no return value. A property with both a get and a set method is called a
read/write property.

Properties may not be parameterized. Properties from parameterized interfaces may use type
parameters of the containing type as the property type.

Properties may not be overloaded.

#### Property Open Issues

- Should xlang support write-only properties? Seems like there's little reason to exclude them.

### Events

An event is a pair of add/remove listener methods with matching name and delegate type. Events are a
mechanism for the type to notify interested parties when something of significance happens.

An event add listener has a single parameter of the event delegate type, and returns an
EventRegistrationToken. An event remove listener method has a single EventRegistrationToken
parameter and has no return value.

For languages that have intrinsic event semantics, like C#, event add/remove methods may be projected
in a language-native manner instead of the signatures listed above.

Events may not be parameterized. Events from parameterized interfaces may use type parameters of the
containing type as the event delegate type.

Events may not be overloaded.

### Intrinsic Operations

All interfaces and runtime classes support the following operations intrinsically. The projection of
these intrinsic operations is language specific. For languages that have a standard projection for
a given intrinsic operation (such as C++ ToString or JavaScript equals), the projection is free to
project the operation in an appropriate manner, if at all.

#### ToString

Returns a string that represents the current object. Typically, this defaults to the object's fully
namespace-qualified type name, but can be customized. For example, an xlang type that represents a
[JSON value](http://json.org/) would return the text representation of the JSON value from ToString.

Projection language examples:
[Java Object.toString](https://docs.oracle.com/javase/7/docs/api/java/lang/Object.html#toString()),
[JavaScript Object.prototype.toString](https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/toString),
[.NET Object.ToString](https://docs.microsoft.com/en-us/dotnet/api/system.object.tostring?view=netframework-4.7.2),
[Objective-C NSObject description](https://developer.apple.com/documentation/objectivec/1418956-nsobject/1418746-description),
and [Python \_\_str__ magic method](https://docs.python.org/3/library/stdtypes.html#str).

#### GetHashCode

Returns an Int32 hash code that can be used to insert and identify an object in a hash-based collection.
Typically, this defaults to a value derived from the object's reference information (such as its
this pointer in C++), but it can be customized. For example, an xlang type that represents a JSON
value would return a hash code generated from the specific contained JSON value. Two xlang JSON value
instances containing the same data would thus return the same value for GetHashCode. Note, however, that
while equal objects should hash equally, equal hash codes does not imply object equality.

xlang types that provide their own implementation of GetHashCode should also provide their own
implementation of Equals (detailed below).

Projection language examples:
[C++ std::hash struct](https://en.cppreference.com/w/cpp/utility/hash),
[Java Object.hashCode](https://docs.oracle.com/javase/7/docs/api/java/lang/Object.html#hashCode()),
[.NET Object.GetHashCode](https://docs.microsoft.com/en-us/dotnet/api/system.object.gethashcode),
[Objective-C NSObject hash](https://developer.apple.com/documentation/objectivec/nsobject/1418561-hash),
and [Python \_\_hash__ magic method](https://docs.python.org/3/reference/datamodel.html#object.__hash__).

> Open Issues
> - Java and .NET intrinsic hash method returns Int32. Python and Objective-C returns a pointer
  sized signed integer (i.e. IntPtr). C++ uses std::size_t, an implementation dependent unsigned
  integer. Should xlang's GetHashCode method be limited to 32 bits to accommodate .NET and Java,
  or should it be pointer sized, forcing .NET and Java to adapt the 64 bit hash value into a 32
  bit one?

#### Equals

Takes an object of any type and returns a boolean indicating if the objects are equal. Typically,
this defaults to reference equality - i.e. is the object parameter the same object instance as the
current object - but it can be customized. For example, an xlang type representing a JSON value would
return true if the object parameter contained the same JSON data as the current object.

xlang types that provide their own implementation of Equals should also provide their own
implementation of GetHashCode (detailed above).

Projection language examples:
[C++ operator==](https://en.cppreference.com/w/cpp/language/operator_comparison),
[Java Object.equals](https://docs.oracle.com/javase/7/docs/api/java/lang/Object.html#equals(java.lang.Object)),
[.NET Object.Equals](https://docs.microsoft.com/en-us/dotnet/api/system.object.equals#System_Object_Equals_System_Object_),
[Objective-C NSObject isEqual](https://developer.apple.com/documentation/objectivec/1418956-nsobject/1418795-isequal),
and [Python \_\_eq__ magic method](https://docs.python.org/3/reference/datamodel.html#object.__eq__).

#### CompareTo

Takes an object of any type and returns an Int32 indicating the relative ordering of the current and
provided objects. A value less than zero indicates the current object precedes the provided object
in the sort order. A value greater than zero indicates the current object follows the provided object.
Zero indicates the two objects are equal.

For most xlang objects, this operation will default to reference comparison - i.e. Compare returns
zero if the provided object is the same object instance as the provided object. If the objects are
not the same instance, their sort order is based on a stable comparison of their reference identity
(such as their ABI object pointers).

xlang objects can provide a custom implementation of CompareTo. For example, an xlang type
representing a JSON value would return a value depending on a lexicographical comparison of the
JSON strings each object represents.

Note, xlang types that provide their own implementation of Compare should also provide their own
implementation of GetHashCode (detailed above).

Projection language examples:
C++ [Proposed "spaceship" operator](http://open-std.org/JTC1/SC22/WG21/docs/papers/2017/p0515r0.pdf)
Java [Java Comparable\<T>](https://docs.oracle.com/javase/7/docs/api/java/lang/Comparable.html)
.NET [IComparable\<T>](https://docs.microsoft.com/en-us/dotnet/api/system.icomparable-1)
Objective-C NSObject [isLessThan](https://developer.apple.com/documentation/objectivec/nsobject/1393841-islessthan)
/ [isGreaterThan](https://developer.apple.com/documentation/objectivec/nsobject/1393885-isgreaterthan),
and [Python Rich Comparison Methods](https://docs.python.org/3/reference/datamodel.html#object.__lt__).
  
> Open Issues
> - Equals vs. CompareTo
>   - Almost all the mainstream languages xlang is likely to project into provide a way to intrinsic
    equality operations on all types. Some, in particular Python and Objective-C, also provide intrinsic
    comparison operators on all types. It has been proposed that xlang should have an intrinsic CompareTo
    operation instead of Equals, since CompareTo can also provide the equivalent of Equals.
>   - It's unclear, however, if this approach would project cleanly into languages like C# and Java that
    have an intrinsic equality operator but not intrinsic comparison operators. C# and Java both have
    generic interfaces that indicate a given type can be compared to another instance of that type for
    sorting purposes. Using CompareTo instead of Equals would likely imply the need for a \[Comparable]
    attribute to indicate a given type has a custom implementation of CompareTo.
> - Comparable vs Comparer
>   - .NET has interfaces for both [IComparable\<T> interface](https://docs.microsoft.com/en-us/dotnet/api/system.icomparable-1)
    and [IComparer\<T> interface](https://docs.microsoft.com/en-us/dotnet/api/system.collections.generic.icomparer-1)
    as well as the [Comparison\<T> delegate](https://docs.microsoft.com/en-us/dotnet/api/system.comparison-1).
    IComparable\<T> is designed to be implemented on instances of the type while IComparer\<T> is
    designed to be implemented on a separate Comparer object. The Comparison delegate behaves similarly
    to the IComparer interface.
>   - The benefit of using a separate comparer object or delegate is that it allows types with value
    semantics to opt-in to comparison operations without requiring additional ABI interfaces on the
    type itself. However, in order to project cleanly into languages with intrinsic comparisons like
    Python and Objective-C, xlang would likely need a mechanism to associate a type with its
    comparer type.
