---
title: Metadata Representation
author: hpierson@microsoft.com
---

# Title: ​XSPEC02 - Metadata Representation

- Author: Harry Pierson (hpierson\@microsoft.com)

- Status: Draft

## Abstract

This document describes how Xlang types are encoded in metadata files.

> _Note_: this document is derived from Windows Runtime specifications and has not yet been updated to use cross-platform terminology. Types referenced in the Xlang.* namespace are placeholders subject to change.

> _Open Issue_: The .xlmeta is a placeholder extension and subject to change.

> _Open Issue_: tdWindowsRuntime flag needs replaced with an Xlang equivalent.

## Overview

Xlang APIs are described in machine readable metadata files with the extension .xlmeta (aka Xlang Metadata). These metadata files are used by tools and language projections in order to enable language projection.

## General Notes

The Xlang runtime provides C-style APIs to aid language projections in the resolution of namespaces and types that need this metadata at runtime.

All public types in a Xlang metadata file must be Xlang types and must carry the tdWindowsRuntime flag (details on type flags to follow). Xlang metadata files may include metadata for non-Xlang types. Any non-Xlang types in a Xlang metadata file must not be public. Semantics for non-Xlang types are implementation defined and outside the scope of this document.

All public interface members (methods, properties, events) on Xlang types must be Xlang interface members. Xlang types may include metadata for non-Xlang interface members. Any non-Xlang interface members may not be public. Semantics for non-Xlang interface members are implementation defined and outside the scope of this document.

## Xlang metadata Files

### Xlang metadata File Format

Xlang metadata files use the same physical file format as CLR assemblies as defined by the ECMA-335 specification. However, while the physical file format is the same, the rules for valid combinations of data are different for Xlang metadata files and CLR assemblies. This document lists the deltas between Xlang metadata files and CLR assemblies.

System provided Xlang metadata files are pure metadata. 3rd party Xlang metadata files may contain code. In particular, managed Xlang metadata files include MSIL code, just like traditional CLR assemblies do.

Every Xlang metadata file contains the definitions of zero or more Xlang types. Empty Xlang metadata files are valid, if not particularly useful

There are no specific Xlang restrictions on PEKind or machine architecture listed in a Xlang metadata

Xlang metadata version String must contain "Xlang runtime 1.2"

### Xlang metadata File Name

The name without extension of a Xlang metadata file must be a case-insensitive match to the name column of the assembly table inside the Xlang metadata file. For example, the "Foo.Bar.xlmeta" file must have "Foo.Bar" in the name column of the assembly table. Because the file system is case insensitive, the case of the file name may differ from the assembly table name column value

All of the Xlang types in a given Xlang metadata file must be under a namespace that matches the name of the Xlang metadata file and the assembly table name column value. Because the file system is case insensitive, the case of the file may differ from the namespace of all the Xlang types in a given Xlang metadata file. The namespace of all the Xlang types in a given Xlang metadata must match the assembly table name column value exactly (i.e. case sensitive). For example, all of the types in the file with "Foo.Bar" in the assembly table's name column must be in the Foo.Bar namespace. The types may be either direct children of this namespace (aka Foo.Bar.MyType) or in subnamespaces of this namespace (aka Foo.Bar.Baz.MyType). The name of the file must be "Foo.Bar.xlmeta", but may vary in case – that is, "foo.bar.xlmeta" and "FOO.BAR.XLMETA would also be permitted as file names for this metadata file.

### Xlang metadata Composition

The metadata for all the types in the system is spread across multiple .xlmeta files. An AppX package can include zero or more .xlmeta files describing 3rd party Xlang components that are included in the application package.

Across all the .xlmeta files provided by the system or included with a given app, every Xlang type's metadata must be stored in the Xlang metadata file with the longest name matching the namespace of the type. All types that are direct children of a given namespace must be located in the same file. For example, if an AppX package includes Foo.xlmeta and Foo.Bar.xlmeta files, the Foo.Bar.Baz.MyType type must be located in the Foo.Bar.xlmeta file, since that is the file with the longest namespace-matching filename to the type in the package.

### TypeDef Redirection

Metadata files provided by the system never reference typedefs directly. Even when referencing a type that's defined in the same metadata file, system metadata files always reference a typeref which in turn references the typedef. This is done in order to support CLR type redirection (projection IVector\<T> as IList\<T> for example).

3rd party metadata files may use TypeDef directly or may redirect all type references thru a typeRef similar to how system metadata files do.

## Type System Encoding

All types in this document from the System namespace from the mscorlib assembly are used as markers by Xlang. These types are used to indicate information about types and should never be resolved. This includes but is not limited to System.Object, System.Guid, System.ValueType, System.Enum, System.MulticastDelegate and System.Attribute Note, these names were chosen for compatibility with CLR. CLR's definition of these types is part of their type system and has nothing to do with Xlang.

Note that many of the constructs described here use C# syntax, this is simply because it is convenient to represent certain CLI metadata constructs using C# syntax. The actual constructs will be pure CLI metadata constructs.

### Namespace

Xlang encodes a type's namespace and local name in a single period delimited string. For example, the type defined in the following snippet of code is "Example.Foundation.ISimpleInterface".

```CS
namespace Example
{
  namespace Foundation
  {
    interface ISimpleInterface
    {
      void Method1(int paramOne);
    };
  };
};
```

Presumably for space optimization, the TypeDef table in CLI metadata provides separate columns for type name and namespace name. However, at the API level the TypeDef props exposes only the type name.

### Fundamental Types

All the Xlang fundamental types except Guid have explicit constant values for use in CLI Metadata blobs and other type references. These constant values are described in Partition 2, Section 23.1.16 of the CLI specification

Xlang Type | CLI Element Type Name | CLI Element Type Value
---------- | --------------------- | ----------------------
Int16      | ELEMENT_TYPE_I2       | 0x06
Int32      | ELEMENT_TYPE_I4       | 0x08
Int64      | ELEMENT_TYPE_I8       | 0x0a
UInt8      | ELEMENT_TYPE_U1       | 0x05
UInt16     | ELEMENT_TYPE_U2       | 0x07
UInt32     | ELEMENT_TYPE_U4       | 0x09
UInt64     | ELEMENT_TYPE_U8       | 0x0b
Single     | ELEMENT_TYPE_R4       | 0x0c
Double     | ELEMENT_TYPE_R8       | 0x0d
Char16     | ELEMENT_TYPE_CHAR     | 0x03
Boolean    | ELEMENT_TYPE_BOOL     | 0x02
String     | ELEMENT_TYPE_STRING   | 0x0e

Since it has no explicit ELEMENT_TYPE_* constant value for them, Guids are represented in metadata as TypeRef to the System.Guid type from the mscorlib assembly.

### Enums

Enums are represented as a row in the TypeDef table (ECMA II.22.37) with the columns set as follows:

- Flags – set to Public | Sealed | tdWindowsRuntime (0x4101)

- Name – an index into the string heap that contains the name of the type

- Namespace – an index into the string heap that contains the namespace of the type

- Extends – set to a TypeRef which references the System.Enum class in the mscorlib assembly

- FieldList – An index into the Field table, marking the first of a contiguous run of fields owned by this type.

- MethodList – must be empty

An enum has a single instance field that specifies the underlying integer type for the enum as well as zero or more static fields, one for each enum value defined by the enum type.

The underlying integer type of the enum appears as the first row in the Field table (ECMA II.22.15) associated with the type (i.e. the one referenced in the FieldList column specified above). The columns in the Field table for the enum type are as follows:

- Flags: Private | SpecialName | RTSpecialName (0x601)

- Name: an index into the string heap that contains the name "value__"

- Signature: an index into the blob heap containing a FieldSig blob (ECMA II.23.2.4) where the Type is set to either ELEMENT_TYPE_I4 or ELEMENT_TYPE_U4 as Xlang enum values must be signed or unsigned 32 bit integers.

After the enum value definition comes a field definition for each of the values in the enumeration:

- Flags: public | static | literal | hasdefault (0x8056)

- Name: an index into the string heap that contains the enum value's name

- Signature: an index into the blob heap containing a FieldSig blob (ECMA II.23.3.4) with the Type set to the TypeDef of the enum type.

For each Enum value definition, there is a corresponding row in the Constant table (ECMA II.22.9) to store the integer value for the enum value.

- Type – one byte to represent the underlying type of the enum, either ELEMENT_TYPE_I4 or ELEMENT_TYPE_U4, followed by one byte padding zero as per the ECMA spec

- Parent: Index into the field table that holds the associated enum value record

- Value: index into the blob table that holds the integer value for the enum value

Additionally, the System.FlagsAttribute must be added to the enumeration TypeDef row for any enums with an underlying UInt32 type. The FlagsAttribute must not be added to the enum TypeDef row for enums with an underlying Int32 type.

For all system provided enums, the VersionAttribute must be added to the enumeration TypeDef row. Optionally, the VersionAttribute may be added to any of the static Field row. If present, the version value from the VersionAttribute on any enum Field rows must be greater than or equal to the value from the VersionAttribute on the enum TypeDef row.

### Structs

Structs are implemented as a row in the TypeDef table (ECMA II.22.37) with the columns set as follows:

- Flags – Public | Sealed | Sequential | tdWindowsRuntime (0x4109)

- Name – an index into the string heap that contains the name of the type

- Namespace – an index into the string heap that contains the namespace of the type

- Extends – set to a TypeRef which references the System.ValueType class in the mscorlib assembly

- FieldList – An index into the Field table, marking the first of a contiguous run of fields owned by this type.

- MethodList – must be empty

Structs have one or more Field table entries.

- Flags: public

- Name: an index into the string heap that contains the field's name

- Signature: an index into the blob heap containing a FieldSig blob (ECMA II.23.2.4) with the Type set to the metadata token for the field type

  - Struct fields must be fundamental types, enums or other structs

For all system provided structs, the VersionAttribute must be added to the struct TypeDef row.

### Delegates

Delegates are implemented as a row in the TypeDef table (ECMA II.22.37) with the columns set as follows:

- Flags: set to Public | Sealed | tdWindowsRuntime (0x4101)

- Name – an index into the string heap that contains the name of the type

- Namespace – an index into the string heap that contains the namespace of the type

- Extends: set to a TypeRef which references the System.MulticastDelegate class in the mscorlib assembly

- FieldList: must be empty

- MethodList: An index into the MethodDef table (ECMA II.22.26), marking the first of a contiguous run of methods owned by this type.

Delegates TypeDef rows must have a GuidAttribute.

Delegates have exactly a two MethodDef table entries. The first defines a constructor. This constructor is a compatibility marker, which is why it uses non Xlang constructs like native int and parameters that are neither in nor out. Xlang Delegates have no such constructor method.

- RVA: 0 (this is an abstract construct)

- ImplFlags: runtime (0x03)

- Flags: private | hidebysig | specialname | RTSpecialName (0x1881)

- Name: an index into the string table containing the name ".ctor"

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) for a method with an object and native int in parameters and no return value

- ParamList: an index into the Param table (ECMA II.22.33) containing the first in a run of Param rows associated with this method. Each row in the Param table will contain the following information

  - Object parameter

    - Sequence 1

    - Name "object"

    - Flags: none (0x00)

  - Native Int parameter

    - Sequence 2

    - Name "method"

    - Flags: none (0x00)

The second MethodDef entry defines the Invoke method:

- RVA: 0 (this is an abstract construct)

- ImplFlags: runtime (0x03)

- Flags: public | Virtual | HideBySig | specialname (0x08C6)

- Name: an index into the string table containing the name "Invoke"

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) that contains the parameter types and return type of the delegate. If the delegate is parameterized, the MethodDefSig blob should reference each of the delegates type parameters via GENERICINST encoded Type (as per ECMA II.23.2.12). Details on parameterized delegates to follow.

- ParamList: an index into the Param table (ECMA II.22.33) containing the first in a run of Param rows associated with this method. Each row in the Param table will contain the following information

  - Flags – in or out as appropriate for the parameter

  - Sequence – sequence order of the parameter. Zero is reserved for the return value of the method

  - Name – index into the string heap containing the name of the parameter

For all system provided delegates, the VersionAttribute must be added to the delegate's TypeDef row.

#### Parameterized Delegates

Parameterized delegates the following additional requirements:

- The name of a parameterized delegate is appended with a back tick and a number representing the number of type parameters the parameterized delegate has. For example, the Xlang.Foundation.EventHandler\

  <t\> type is stored in
  metadata with the name Xlang.Foundation.EventHandler`1</t\>

- Parameterized delegates have one row in the GenericParam table (ECMA II.22.20) for every type parameter with the columns set as follows:

  - Number: the index of the generic parameter, numbered left-to-right, starting at zero

  - Flags: None

  - Owner: Index into the TypeDef table for the row containing the interface

  - Name: Index into the string heap containing the name of the generic parameter

The TypeSpec table (ECMA II.23.2.14) is used to define instances of parameterized delegates. These TypeSpecs can then be used in method signatures similarly to TypeRefs.

### Interfaces

Interfaces are implemented as a row in the TypeDef table (ECMA II.22.37) with the columns set as follows:

- Flags:

  - interface | public | abstract | tdWindowsRuntime (0x40A1) or

  - interface | NotPublic| abstract | tdWindowsRuntime (0x40A0)

- Name: an index into the string table containing the interface name

- Namespace – an index into the string heap that contains the namespace of the type

- Extends: null

- FieldList: must be empty

- MethodList: An index into the MethodDef table, marking the first of a contiguous run of methods owned by this type. Specifics on the contents of the MethodDef table are detailed in the subsections of the current section.

Interfaces TypeDef rows must have a GuidAttribute as well as a VersionAttribute.

Any Xlang interface with private visibility must have a single ExclusiveToAttribute. Any Xlang interface with public visibility must not have an ExclusiveToAttribute. If present, the ExclusiveToAttribute must reference a runtime class.

Required interfaces for an interface are represented by rows in the InterfaceImpl table (ECMA II.22.23) with the columns set as follows:

- Class: an index into the TypeDef table for the row containing the interface

- Interface: an index into the TypeDef, TypeRef or TypeSpec table that specifies the required interface. Note, in system provided metadata files, this will never be a TypeDef even if the required interface is defined in the same metadata file. See the _TypeDef Redirection_ section for further details.

#### Parameterized Interfaces

Parameterized interfaces the following additional requirements:

The name of a parameterized interface is appended with a back tick and a number representing the number of type parameters the parameterized delegate has. For example, the Xlang.Foundation.Collections.IVector\<T> type is stored in metadata with the name Xlang.Foundation.Collections.IVector`1\<T>.

Parameterized interfaces have one row in the GenericParam table (ECMA II.22.20) for every type parameter with the columns set as follows:

- Number: the index of the generic parameter, numbered left-to-right, starting at zero

- Flags: None

- Owner: Index into the TypeDef table for the row containing the interface

- Name: Index into the string heap containing the name of the generic parameter

The TypeSpec table (ECMA II.23.2.14) is used to define instances of parameterized interfaces. These TypeSpecs can then be used in method signatures and interface implementations similarly to TypeRefs.

### Interface Members

#### Array Parameters

When encoding an Array parameter for any interface member type, the array length parameter that immediately precedes the array parameter is omitted from both the MethodDefSig blob as well as the params table.

The direction of the array parameter is directly encoded in metadata. The direction of the array length parameter may be inferred as follows:

- If the array parameter is an in parameter, the array length parameter must also be an in parameter. This represents the _PassArray_ pattern.

- If the array parameter is an out parameter and is not carrying the BYREF marker, the array length is an in parameter. This represents the _FillArray_ pattern.

- If the array parameter is an out parameter and carries the BYREF marker, the array length is an out parameter. This represents the _ReceiveArray_ pattern.

#### Methods

In order to better model the expected projection of methods as well as CLR compatibility, the required HRESULT return value is not encoded in metadata. Rather, the out parameter to be used as the return value is encoded as the return value in the methodDefSig. For methods that do not declare an out parameter to be used as the return value, the methodDefSig must declare the return type to be void (as per ECMA II.23.2.11).

Each method on an interface will be represented as a row in the MethodDef table. Each methoddef row will contain the following information:

- RVA: 0x00

- ImplFlags: 0x00

- Flags: public | Virtual | HideBySig | Abstract | NewSlot | Instance (0x5c6)

- Name: an index into the string table containing the name of the method

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) that contains the parameter types and return type of the method. If the interface is parameterized, the MethodDefSig blob should reference each of the interface's type parameters via GENERICINST encoded Type (as per ECMA II.23.2.12). Details on parameterized interfaces to follow.

- ParamList: an index into the Param table (ECMA II.22.33) containing the first in a run of Param rows associated with this method.

Each parameter of the method (plus the return value if specified) will have a corresponding row in the Param table (ECMA II.22.33).

- Flags – none, in or out as appropriate for the parameter.

  - Return values are always none

  - Other parameters are always in or out

- Sequence – sequence order of the parameter.

  - Zero is reserved for the return value of the method

- Name – index into the string heap containing the name of the parameter

Each method may optionally have an OverloadAttribute that carries the unique method name (within the scope of the interface). Each method may optionally have a DefaultOverloadAttribute that indicates which overloaded method of the same arity (aka number of in parameters) should be projected in weakly, dynamically typed languages.

#### Properties

Each property on an interface is defined as rows in the Property (ECMA II.22.34), PropertyMap (ECMA II.22.35), MethodSemantics (ECMA II.22.28) and MethodDef (ECMA II.22.26) tables.

Each interface with one or more properties will be represented as a single row in the PropertyMap table containing the following information:

- Parent: an index into the TypeDef table containing the interface that contains the properties

- PropertyList: an index into the Property table containing the first in a run of rows associated with this type.

Each property will be represented as a single row in the Property table containing the following information:

- Flags: None

- Name: an index into the string heap containing the name of the property

- Type: index into the blob heap containing a PropertySig blob (ECMA II.23.2.5) containing the type information for the property.

Each Property will be represented as one or two rows in the MethodDef table. Read only properties are represented as a single method with the "get_" prefix while read/write properties are represented as two methods, one with the "get_" and the other with the "put_" prefix. The signature for the get method takes no parameters and returns a value of the property's type. The signature for the set method takes a single parameter of the property's type and doesn't return anything.

The MethodDef rows for the property contain the following:

- RVA: 0

- ImplFlags: None

- Flags: public | virtual | HideBySig | newSlot | abstract | specialname (0xDC6)

- Name: an index into the string table containing "get_\<PropertyName>” or
  “put\<PropertyName> as appropriate

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) that contains the parameter types and return type of the method as described above.

- ParamList: an index into the Param table (ECMA II.22.33) containing the first in a run of Param rows associated with this method. Values in the Param table are as specified under methods above.

Each MethodDef row for the property will have an associated row in the MethodSemantics table containing the following information:

- Semantics: Getter or Setter as appropriate

- Method: Index into the MethodDef table containing the getter or setter method

- Association: Index into the Property table containing the property

#### Events

Each event on an interface is defined as rows in the Event (ECMA II.22.13), EventMap (ECMA II.22.12), MethodSemantics (ECMA II.22.28) and MethodDef (ECMA II.22.26) tables.

Each interface with one or more events will be represented as a single row in the EventMap table containing the following information:

- Parent: an index into the TypeDef table containing the interface that contains the events

- EventList: an index into the Event table containing the first in a run of rows associated with this type.

Each Event will be represented as a single row in the Event table containing the following information:

- EventFlags: None

- Name: an index into the string heap containing the name of the property

- EventType: a TypeDefOrRef that indexes into the appropriate table that contains the delegate type of the event.

Each Event will be represented as two rows in the MethodDef table, one with the "add_" prefix for adding event listeners and one with the "remove_" prefix for removing event listeners. The add method takes in a delegate instance and returns a Xlang.Foundation.EventRegistrationToken that represents the event registration. The remove method takes the EventRegistrationToken returned by the add method to unregister the event.

The MethodDef rows for the event contains the following:

- RVA: 0

- ImplFlags: None

- Flags: public | Final | virtual | hidebysig | newslot | specialname (0x09e6)

- Name: an index into the string table containing "add_\<EventName> or
  “remove_\<EventName> as appropriate.

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) that contains the parameter and return types of the method as described below.

  - Add_ method takes a single parameter of the delegate type and returns a Xlang.Foundation.EventRegistrationToken.

  - Remove_ method takes a single Xlang.Foundation.EventRegistrationToken parameter and returns nothing.

- ParamList: an index into the Param table (ECMA II.22.33) containing the first in a run of Param rows associated with the method. Values in the Param table are as specified under methods above.

Both MethodDef rows for the event will have an associated row in the MethodSemantics table containing the following information:

- Semantics: AddOn or RemoveOn as appropriate

- Method: Index into the MethodDef table containing the add or remove listener method

- Associateion: Index into the Event table containing the event

### Runtime Classes

Runtime Classes are implemented as a row in the TypeDef table (ECMA II.22.37) with the columns set as follows:

- Flags: all runtime classes must carry the public, auto layout, class and tdWindowsRuntime flags.

  - Static only classes carry the abstract flag. All other classes do not carry the abstract flag

  - Non-composable classes carry the sealed flag. Composable classes do not carry the sealed flag

- Name: an index into the string table containing the class name

- Namespace – an index into the string heap that contains the namespace of the type

- Extends: an index into the TypeRef referencing either a composable class or System.Object in mscorlib

- FieldList: must be empty

- MethodList: An index into the MethodDef table, marking the first of a contiguous run of methods owned by this type. Specifics on the contents of the MethodDef table are detailed below.

For all system provided classes, the VersionAttribute must be added to the classes' TypeDef row.

#### Implemented Interfaces

Interfaces implemented by runtime classes are represented by rows in the InterfaceImpl table (ECMA II.22.23) with the columns set as follows:

- Class: an index into the TypeDef table for the row containing the type

- Interface: an index into the TypeDef, TypeRef or TypeSpec table that specifies the implemented interface. Note, in system provided metadata files, this will never be a TypeDef even if the required interface is defined in the same metadata file. See the _TypeDef Redirection_ section for further details.

Runtime classes must specify the DefaultAttribute on exactly one of their InterfaceImpl rows.

Runtime classes may specify the OverridableAttribute or ProtectedAttribute on any of their InterfaceImpl rows. They may not specify both OverridableAttribute and ProtectedAttribute on the same row.

Optionally, the VersionAttribute may be added to any of the class's interfaceImpl rows. The version value from the VersionAttribute on any class's interfaceImpl rows must be greater than or equal to the value from the VersionAttribute on the class's TypeDef row.

#### Static Interfaces

Runtime classes have zero or more StaticAttribute custom attributes. It is legal to specify more than one StaticAttribute custom attributes, so long as each has different specified parameters. Any StaticAttribute will appear as a row in the CustomAttribute table with the following information:

- Parent: The runtime class the StaticAttribute is associated with

- Type: A reference to StaticAttribute's .ctor

- Value: a custom attribute blob containing the System.Type static interface parameter and the Uint32 version parameter

#### Activation

Runtime classes have zero or more ActivatableAttribute custom attributes. It is legal to specify more than one ActivatableAttribute custom attributes, so long as each has different specified parameters. Any ActivatableAttributes will appear as a row in the CustomAttribute table with the following information:

- Parent: The runtime class the ActivatableAttribute is associated with

- Type: A reference to one of ActivatableAttribute's two .ctors:

  - Direct Activation: the .ctor taking just the Uint32 version parameter

  - Factory activation: the .ctor taking the System.Type factory interface parameter and the Uint32 version parameter

- Value: a custom attribute blob containing the System.Type factory interface parameter (if provided) and the Uint32 version parameter

#### Composition

Runtime classes have zero or more ComposableAttribute custom attributes. It is legal to specify more than one ComposableAttribute custom attributes, so long as each has different specified parameters. Any ComposableAttribute will appear as a row in the CustomAttribute table with the following information:

- Parent: The runtime class the ComposableAttribute is associated with

- Type: A reference to ComposableAttribute's .ctor

- Value: a custom attribute blob containing the System.Type composition factory interface interface parameter, a CompositionType enum value (Public or Protected) and the Uint32 version parameter

#### Class Methods

A runtime class has a row in the MethodDef table for every method on every interface associated with the class. This includes member interfaces (normal, protected and overridable), static interfaces, activation factory interfaces and composable factory interfaces. Additionally, a class that supports direct activation will also have a row in the MethodDef table to indicate this.

##### Member Interface Members

Each method from a member interface (including protected and overridable interfaces) is represented by a row in the class's MethodDef table. The Class's methodDef table contains an exact copy of the MethodDef information from the original declaring interface, including Param table rows and custom attributes, with the following exceptions:

- Runtime classes may specify alternative names for methods defined on member interfaces

- Methods on runtime classes do not get the Abstract flag

- Methods on runtime classes get the Runtime MethodImpl flag

- Methods from non-overridable interfaces additionally get the Final flag. Methods from overridable interfaces do not get the Final flag

Each row in the MethodDef table of a class from a member interface is connected back to the interface method that originally defined the method via an entry in the MethodImpl table (ECMA II.22.27) with values as follows:

- Class – an index into the TypeDef table which references the class carrying the method (note, this index is not subject to _TypeDef Redirection_)

- MethodBody – An index into the MethodDef table which references the class method

- MethodDeclaration – an index into the MethodDef or MemberRef table which references the originally declared interface method

##### Static Interface Members

Each method from a static interface is represented by a row in the class's MethodDef table. The Class's methodDef table contains an exact copy of the MethodDef information from the original declaring interface, including Param table rows and custom attributes, with the following exceptions:

- Static members do not get the Virtual, Abstract, NewSlot and Instance flags.

- Static members do get the Static and Class flags

- Static Methods on runtime classes get the Runtime MethodImpl flag

##### Activation Members

Classes that support direct, parameterless activation have a constructor row in the class's MethodDef table with the following column values:

- RVA: 0x00

- ImplFlags: Runtime

- Flags: public | HideBySig | SpecialName | RTSpecialName | Instance

- Name: an index into the string table containing ".ctor"

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) that contains no parameters and returns null

- ParamList: must be empty

Classes that support factory activation have a constructor row in the class's MethodDef table for every method in every implemented factory interface with the following column values:

- RVA: 0x00

- ImplFlags: Runtime

- Flags: public | HideBySig | SpecialName | RTSpecialName | Instance

- Name: an index into the string table containing ".ctor"

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) that contains the input parameters and returns null

- ParamList: pointer into the Params table with a row for every parameter, copied exactly from the params table for the originally declaring factory method.

##### Composition Members

Classes that support composition factory activation have a constructor row in the class's MethodDef table for every method in every implemented factory interface with the following column values:

- RVA: 0x00

- ImplFlags: Runtime

- Flags: public | HideBySig | SpecialName | RTSpecialName | Instance

- Name: an index into the string table containing ".ctor"

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) that contains the custom input parameters and returns null. The controlling IInspectable \[in] parameter and the non-delegating IInspectable* \[out] parameter are not reflected in the method signature

- ParamList: pointer into the Params table with a row for every parameter except the controlling IInspectable \[in] parameter and the non-delegating IInspectable* \[out] parameter, copied exactly from the params table for the originally declaring factory method.

### Custom Attributes

Custom attributes have zero or more constructor methods, each with zero or more parameters where the parameter type is limited to the fundamental types, enums and System.Type. Each constructor in the custom attribute appears as a row in the MethodDef with the following information

- RVA (aka Relative Virtual Address): null

- ImplFlags: None

- Flags: public | HideBySig | specalname | RTSpecialName (0x1886)

- Name: an index into the string table containing the name ".ctor"

- Signature: an index into the blob heap containing a MethodDefSig blob (ECMA II.23.2.1) that contains the parameter types and return type of the method

- ParamList: an index into the Param table (ECMA II.22.33) containing the first in a run of Param rows associated with this method.

Custom attributes on metadata constructs are stored as rows in the CustomAttribute table (ECMA II.22.10) with the columns set as follows:

- Parent: index into the metadata table the custom attribute is attached to

- Type: index into the MethodDef or MemberRef table that contains a reference to the constructor of the attribute type

- Value: index into the blob heap that contains positional and named attribute parameters (ECMA II.23.2). Note, since Xlang custom attributes are not allowed to have properties, the custom attribute blob will never contain PROPERTY style named arguments.
