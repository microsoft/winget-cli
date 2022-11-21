"use strict";

function initializeScript()
{
    return [
        new host.typeSignatureRegistration(__DatabaseVisualizer, "xlang::meta::reader::database"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("undefined"), "xlang::meta::reader::table_base"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("undefined"), "xlang::meta::reader::table<*>"),

        // row_base derived types
        new host.typeSignatureRegistration(__AssemblyVisualizer, "xlang::meta::reader::Assembly"),
        new host.typeSignatureRegistration(__AssemblyVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::Assembly>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__AssemblyVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::Assembly>"),

        new host.typeSignatureRegistration(__AssemblyOSVisualizer, "xlang::meta::reader::AssemblyOS"),
        new host.typeSignatureRegistration(__AssemblyOSVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::AssemblyOS>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__AssemblyOSVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::AssemblyOS>"),

        new host.typeSignatureRegistration(__AssemblyProcessorVisualizer, "xlang::meta::reader::AssemblyProcessor"),
        new host.typeSignatureRegistration(__AssemblyProcessorVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::AssemblyProcessor>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__AssemblyProcessorVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::AssemblyProcessor>"),

        new host.typeSignatureRegistration(__AssemblyRefVisualizer, "xlang::meta::reader::AssemblyRef"),
        new host.typeSignatureRegistration(__AssemblyRefVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::AssemblyRef>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__AssemblyRefVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::AssemblyRef>"),

        new host.typeSignatureRegistration(__AssemblyRefOSVisualizer, "xlang::meta::reader::AssemblyRefOS"),
        new host.typeSignatureRegistration(__AssemblyRefOSVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::AssemblyRefOS>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__AssemblyRefOSVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::AssemblyRefOS>"),

        new host.typeSignatureRegistration(__AssemblyRefProcessorVisualizer, "xlang::meta::reader::AssemblyRefProcessor"),
        new host.typeSignatureRegistration(__AssemblyRefProcessorVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::AssemblyRefProcessor>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__AssemblyRefProcessorVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::AssemblyRefProcessor>"),

        new host.typeSignatureRegistration(__ClassLayoutVisualizer, "xlang::meta::reader::ClassLayout"),
        new host.typeSignatureRegistration(__ClassLayoutVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::ClassLayout>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__ClassLayoutVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::ClassLayout>"),

        new host.typeSignatureRegistration(__ConstantVisualizer, "xlang::meta::reader::Constant"),
        new host.typeSignatureRegistration(__ConstantVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::Constant>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__ConstantVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::Constant>"),

        new host.typeSignatureRegistration(__CustomAttributeVisualizer, "xlang::meta::reader::CustomAttribute"),
        new host.typeSignatureRegistration(__CustomAttributeVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::CustomAttribute>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__CustomAttributeVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::CustomAttribute>"),

        new host.typeSignatureRegistration(__DeclSecurityVisualizer, "xlang::meta::reader::DeclSecurity"),
        new host.typeSignatureRegistration(__DeclSecurityVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::DeclSecurity>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__DeclSecurityVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::DeclSecurity>"),

        new host.typeSignatureRegistration(__EventMapVisualizer, "xlang::meta::reader::EventMap"),
        new host.typeSignatureRegistration(__EventMapVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::EventMap>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__EventMapVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::EventMap>"),

        new host.typeSignatureRegistration(__EventVisualizer, "xlang::meta::reader::Event"),
        new host.typeSignatureRegistration(__EventVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::Event>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__EventVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::Event>"),

        new host.typeSignatureRegistration(__ExportedTypeVisualizer, "xlang::meta::reader::ExportedType"),
        new host.typeSignatureRegistration(__ExportedTypeVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::ExportedType>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__ExportedTypeVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::ExportedType>"),

        new host.typeSignatureRegistration(__FieldVisualizer, "xlang::meta::reader::Field"),
        new host.typeSignatureRegistration(__FieldVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::Field>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__FieldVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::Field>"),

        new host.typeSignatureRegistration(__FieldLayoutVisualizer, "xlang::meta::reader::FieldLayout"),
        new host.typeSignatureRegistration(__FieldLayoutVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::FieldLayout>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__FieldLayoutVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::FieldLayout>"),

        new host.typeSignatureRegistration(__FieldMarshalVisualizer, "xlang::meta::reader::FieldMarshal"),
        new host.typeSignatureRegistration(__FieldMarshalVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::FieldMarshal>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__FieldMarshalVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::FieldMarshal>"),

        new host.typeSignatureRegistration(__FieldRVAVisualizer, "xlang::meta::reader::FieldRVA"),
        new host.typeSignatureRegistration(__FieldRVAVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::FieldRVA>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__FieldRVAVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::FieldRVA>"),

        new host.typeSignatureRegistration(__FileVisualizer, "xlang::meta::reader::File"),
        new host.typeSignatureRegistration(__FileVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::File>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__FileVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::File>"),

        new host.typeSignatureRegistration(__GenericParamVisualizer, "xlang::meta::reader::GenericParam"),
        new host.typeSignatureRegistration(__GenericParamVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::GenericParam>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__GenericParamVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::GenericParam>"),

        new host.typeSignatureRegistration(__GenericParamConstraintVisualizer, "xlang::meta::reader::GenericParamConstraint"),
        new host.typeSignatureRegistration(__GenericParamConstraintVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::GenericParamConstraint>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__GenericParamConstraintVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::GenericParamConstraint>"),

        new host.typeSignatureRegistration(__ImplMapVisualizer, "xlang::meta::reader::ImplMap"),
        new host.typeSignatureRegistration(__ImplMapVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::ImplMap>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__ImplMapVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::ImplMap>"),

        new host.typeSignatureRegistration(__InterfaceImplVisualizer, "xlang::meta::reader::InterfaceImpl"),
        new host.typeSignatureRegistration(__InterfaceImplVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::InterfaceImpl>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__InterfaceImplVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::InterfaceImpl>"),

        new host.typeSignatureRegistration(__ManifestResourceVisualizer, "xlang::meta::reader::ManifestResource"),
        new host.typeSignatureRegistration(__ManifestResourceVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::ManifestResource>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__ManifestResourceVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::ManifestResource>"),

        new host.typeSignatureRegistration(__MemberRefVisualizer, "xlang::meta::reader::MemberRef"),
        new host.typeSignatureRegistration(__MemberRefVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::MemberRef>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__MemberRefVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::MemberRef>"),

        new host.typeSignatureRegistration(__MethodDefVisualizer, "xlang::meta::reader::MethodDef"),
        new host.typeSignatureRegistration(__MethodDefVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::MethodDef>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__MethodDefVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::MethodDef>"),

        new host.typeSignatureRegistration(__MethodImplVisualizer, "xlang::meta::reader::MethodImpl"),
        new host.typeSignatureRegistration(__MethodImplVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::MethodImpl>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__MethodImplVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::MethodImpl>"),

        new host.typeSignatureRegistration(__MethodSemanticsVisualizer, "xlang::meta::reader::MethodSemantics"),
        new host.typeSignatureRegistration(__MethodSemanticsVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::MethodSemantics>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__MethodSemanticsVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::MethodSemantics>"),

        new host.typeSignatureRegistration(__MethodSpecVisualizer, "xlang::meta::reader::MethodSpec"),
        new host.typeSignatureRegistration(__MethodSpecVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::MethodSpec>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__MethodSpecVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::MethodSpec>"),

        new host.typeSignatureRegistration(__ModuleVisualizer, "xlang::meta::reader::Module"),
        new host.typeSignatureRegistration(__ModuleVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::Module>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__ModuleVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::Module>"),

        new host.typeSignatureRegistration(__ModuleRefVisualizer, "xlang::meta::reader::ModuleRef"),
        new host.typeSignatureRegistration(__ModuleRefVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::ModuleRef>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__ModuleRefVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::ModuleRef>"),

        new host.typeSignatureRegistration(__NestedClassVisualizer, "xlang::meta::reader::NestedClass"),
        new host.typeSignatureRegistration(__NestedClassVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::NestedClass>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__NestedClassVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::NestedClass>"),

        new host.typeSignatureRegistration(__ParamVisualizer, "xlang::meta::reader::Param"),
        new host.typeSignatureRegistration(__ParamVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::Param>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__ParamVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::Param>"),

        new host.typeSignatureRegistration(__PropertyVisualizer, "xlang::meta::reader::Property"),
        new host.typeSignatureRegistration(__PropertyVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::Property>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__PropertyVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::Property>"),

        new host.typeSignatureRegistration(__PropertyMapVisualizer, "xlang::meta::reader::PropertyMap"),
        new host.typeSignatureRegistration(__PropertyMapVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::PropertyMap>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__PropertyMapVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::PropertyMap>"),

        new host.typeSignatureRegistration(__StandAloneSigVisualizer, "xlang::meta::reader::StandAloneSig"),
        new host.typeSignatureRegistration(__StandAloneSigVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::StandAloneSig>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__StandAloneSigVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::StandAloneSig>"),

        new host.typeSignatureRegistration(__TypeDefVisualizer, "xlang::meta::reader::TypeDef"),
        new host.typeSignatureRegistration(__TypeDefVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::TypeDef>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__TypeDefVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::TypeDef>"),

        new host.typeSignatureRegistration(__TypeRefVisualizer, "xlang::meta::reader::TypeRef"),
        new host.typeSignatureRegistration(__TypeRefVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::TypeRef>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__TypeRefVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::TypeRef>"),

        new host.typeSignatureRegistration(__TypeSpecVisualizer, "xlang::meta::reader::TypeSpec"),
        new host.typeSignatureRegistration(__TypeSpecVisualizer, "xlang::meta::reader::row_base<xlang::meta::reader::TypeSpec>"),
        new host.typeSignatureRegistration(__MakeTableVisualizer("__TypeSpecVisualizer"), "xlang::meta::reader::table<xlang::meta::reader::TypeSpec>"),

        // coded_index types
        new host.typeSignatureRegistration(__TypeDefOrRefCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::TypeDefOrRef>"),
        new host.typeSignatureRegistration(__HasConstantCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::HasConstant>"),
        new host.typeSignatureRegistration(__HasCustomAttributeCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::HasCustomAttribute>"),
        new host.typeSignatureRegistration(__HasFieldMarshalCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::HasFieldMarshal>"),
        new host.typeSignatureRegistration(__HasDeclSecurityCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::HasDeclSecurity>"),
        new host.typeSignatureRegistration(__MemberRefParentCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::MemberRefParent>"),
        new host.typeSignatureRegistration(__HasSemanticsCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::HasSemantics>"),
        new host.typeSignatureRegistration(__MethodDefOrRefCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::MethodDefOrRef>"),
        new host.typeSignatureRegistration(__MemberForwardedCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::MemberForwarded>"),
        new host.typeSignatureRegistration(__ImplementationCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::Implementation>"),
        new host.typeSignatureRegistration(__CustomAttributeTypeCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::CustomAttributeType>"),
        new host.typeSignatureRegistration(__ResolutionScopeCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::ResolutionScope>"),
        new host.typeSignatureRegistration(__TypeOrMethodDefCodedIndexVisualizer, "xlang::meta::reader::coded_index<xlang::meta::reader::TypeOrMethodDef>"),

        // Signature types
        new host.typeSignatureRegistration(__CustomModSig, "xlang::meta::reader::CustomModSig"),
        new host.typeSignatureRegistration(__GenericTypeInstSig, "xlang::meta::reader::GenericTypeInstSig"),
        new host.typeSignatureRegistration(__GenericTypeIndex, "xlang::meta::reader::GenericTypeIndex"),
        new host.typeSignatureRegistration(__TypeSig, "xlang::meta::reader::TypeSig"),
        new host.typeSignatureRegistration(__ParamSig, "xlang::meta::reader::ParamSig"),
        new host.typeSignatureRegistration(__RetTypeSig, "xlang::meta::reader::RetTypeSig"),
        new host.typeSignatureRegistration(__MethodDefSig, "xlang::meta::reader::MethodDefSig"),
        new host.typeSignatureRegistration(__FieldSig, "xlang::meta::reader::FieldSig"),
        new host.typeSignatureRegistration(__PropertySig, "xlang::meta::reader::PropertySig"),
        new host.typeSignatureRegistration(__TypeSpecSig, "xlang::meta::reader::TypeSpecSig"),
        new host.typeSignatureRegistration(__CustomAttributeSig, "xlang::meta::reader::CustomAttributeSig"),
        new host.typeSignatureRegistration(__FixedArgSig, "xlang::meta::reader::FixedArgSig"),
        new host.typeSignatureRegistration(__NamedArgSig, "xlang::meta::reader::NamedArgSig"),
        new host.typeSignatureRegistration(__ElemSig, "xlang::meta::reader::ElemSig"),

        // Other
        new host.typeSignatureRegistration(__EnumDefinition, "xlang::meta::reader::EnumDefinition"),
        new host.typeSignatureRegistration(__SystemType, "xlang::meta::reader::ElemSig::SystemType"),
        new host.typeSignatureRegistration(__EnumValue, "xlang::meta::reader::ElemSig::EnumValue"),
    ]
}

function __getStdVariantValue(value)
{
    var index = value.index();
    for (var j = 0; j < index; ++j)
    {
        value = value._Tail;
    }

    return value._Head;
}

function __enumToString(value, enumValues)
{
    for (var property in enumValues)
    {
        if (enumValues.hasOwnProperty(property) && (value === enumValues[property]))
        {
            return property;
        }
    }

    return "Unknown (" + value + ")";
}

function __enumFlagsToString(value, enumValues)
{
    if (value === 0)
    {
        return "None";
    }

    var prefix = "";
    var result = "";
    for (var property in enumValues)
    {
        if (enumValues.hasOwnProperty(property) && ((value & enumValues[property]) == enumValues[property]))
        {
            result += prefix + property;
            value = value & ~enumValues[property];
            prefix = " | ";
        }
    }

    if (value != 0)
    {
        result += prefix + value;
    }

    return result;
}

var __TypeDefOrRef = Object.freeze({
    TypeDef: 0,
    TypeRef: 1,
    TypeSpec: 2,

    Values: [
        "TypeDef",
        "TypeRef",
        "TypeSpec",
    ],

    Name: "TypeDefOrRef",
    CodedIndexBits: 2,
});

var __HasConstant = Object.freeze({
    Field: 0,
    Param: 1,
    Property: 2,

    Values: [
        "Field",
        "Param",
        "Property",
    ],

    Name: "HasConstant",
    CodedIndexBits: 2,
});

var __HasCustomAttribute = Object.freeze({
    MethodDef: 0,
    Field: 1,
    TypeRef: 2,
    TypeDef: 3,
    Param: 4,
    InterfaceImpl: 5,
    MemberRef: 6,
    Module: 7,
    Permission: 8,
    Property: 9,
    Event: 10,
    StandAloneSig: 11,
    ModuleRef: 12,
    TypeSpec: 13,
    Assembly: 14,
    AssemblyRef: 15,
    File: 16,
    ExportedType: 17,
    ManifestResource: 18,
    GenericParam: 19,
    GenericParamConstraint: 20,
    MethodSpec: 21,

    Values: [
        "MethodDef",
        "Field",
        "TypeRef",
        "TypeDef",
        "Param",
        "InterfaceImpl",
        "MemberRef",
        "Module",
        "Permission",
        "Property",
        "Event",
        "StandAloneSig",
        "ModuleRef",
        "TypeSpec",
        "Assembly",
        "AssemblyRef",
        "File",
        "ExportedType",
        "ManifestResource",
        "GenericParam",
        "GenericParamConstraint",
        "MethodSpec",
    ],

    Name: "HasCustomAttribute",
    CodedIndexBits: 5,
});

var __HasFieldMarshal = Object.freeze({
    Field: 0,
    Param: 1,

    Values: [
        "Field",
        "Param",
    ],

    Name: "HasFieldMarshal",
    CodedIndexBits: 1,
});

var __HasDeclSecurity = Object.freeze({
    TypeDef: 0,
    MethodDef: 1,
    Assembly: 2,

    Values: [
        "TypeDef",
        "MethodDef",
        "Assembly",
    ],

    Name: "HasDeclSecurity",
    CodedIndexBits: 2,
});

var __MemberRefParent = Object.freeze({
    TypeDef: 0,
    TypeRef: 1,
    ModuleRef: 2,
    MethodDef: 3,
    TypeSpec: 4,

    Values: [
        "TypeDef",
        "TypeRef",
        "ModuleRef",
        "MethodDef",
        "TypeSpec",
    ],

    Name: "MemberRef",
    CodedIndexBits: 3,
});

var __HasSemantics = Object.freeze({
    Event: 0,
    Property: 1,

    Values: [
        "Event",
        "Property",
    ],

    Name: "HasSemantics",
    CodedIndexBits: 1,
});

var __MethodDefOrRef = Object.freeze({
    MethodDef: 0,
    MemberRef: 1,

    Values: [
        "MethodDef",
        "MemberRef",
    ],

    Name: "MethodDefOrRef",
    CodedIndexBits: 1,
});

var __MemberForwarded = Object.freeze({
    Field: 0,
    MethodDef: 1,

    Values: [
        "Field",
        "MethodDef",
    ],

    Name: "MemberForwarded",
    CodedIndexBits: 1,
});

var __Implementation = Object.freeze({
    File: 0,
    AssemblyRef: 1,
    ExportedType: 2,

    Values: [
        "File",
        "AssemblyRef",
        "ExportedType",
    ],

    Name: "Implementation",
    CodedIndexBits: 2,
});


var __CustomAttributeType = Object.freeze({
    MethodDef: 2,
    MemberRef: 3,

    Values: [
        null,
        null,
        "MethodDef",
        "MemberRef",
    ],

    Name: "CustomAttributeType",
    CodedIndexBits: 3,
});

var __ResolutionScope = Object.freeze({
    Module: 0,
    ModuleRef: 1,
    AssemblyRef: 2,
    TypeRef: 3,

    Values: [
        "Module",
        "ModuleRef",
        "AssemblyRef",
        "TypeRef",
    ],

    Name: "ResolutionScope",
    CodedIndexBits: 2,
});

var __TypeOrMethodDef = Object.freeze({
    TypeDef: 0,
    MethodDef: 1,

    Values: [
        "TypeDef",
        "MethodDef",
    ],

    Name: "TypeOrMethodDef",
    CodedIndexBits: 1,
});

function __codedValueType(value, codedEnum)
{
    return value & ((1 << codedEnum.CodedIndexBits) - 1);
}

function __codedValueIndex(value, codedEnum)
{
    return (value >> codedEnum.CodedIndexBits) - 1;
}

function __makeCodedValue(index, type, codedEnum)
{
    return ((index + 1) << codedEnum.CodedIndexBits) | type;
}

function __valueToHex(value, bytes)
{
    var result = value.toString(16);
    while (result.length < (bytes * 2))
    {
        result = "0" + result;
    }

    return result;
}

class __Blob
{
    constructor(data, size)
    {
        this.__data = data;
        this.__size = size;
    }

    toString()
    {
        var data = this.ReadValues(this.__size);
        var result = "";
        var prefix = "";
        for (var value of data)
        {
            result += prefix;
            result += __valueToHex(value, 1);
            prefix = " ";
        }

        return result;
    }

    ReadValue(size, offset)
    {
        return this.ReadValues(1, size, offset)[0];
    }

    ReadValues(count, dataSize, offset)
    {
        dataSize = dataSize || 1;
        offset = offset || 0;
        return host.memory.readMemoryValues(this.__data.add(offset).address, count, dataSize);
    }

    ReadString(size, offset)
    {
        offset = offset || 0;
        size = size || (this.__size - offset);
        return host.memory.readString(this.__data.add(offset).address, size);
    }
}

class __BlobStream
{
    constructor(blob, initialPos)
    {
        this.__blob = blob;
        this.__cursor = initialPos || 0;
    }

    copy(offset)
    {
        return new __BlobStream(this.__blob, this.__cursor + initialPos);
    }

    seek(delta)
    {
        this.__cursor += delta;
    }

    consumeValue(dataSize)
    {
        var result = this.peekValue(dataSize);
        this.__cursor += dataSize;
        return result;
    }

    peekValue(dataSize)
    {
        if (this.__cursor + dataSize > this.__blob.Size)
        {
            throw new RangeError("Not enough data remaining in blob");
        }

        return this.__blob.ReadValue(dataSize, this.__cursor);
    }

    consumeUnsigned()
    {
        var result = this.peekUnsigned();
        this.__cursor += result.length;
        return result.value;
    }

    peekUnsigned()
    {
        var initialByte = this.__blob.ReadValue(1, this.__cursor);
        var length;
        if ((initialByte & 0x80) == 0)
        {
            length = 1;
        }
        else if ((initialByte & 0xC0) == 0x80)
        {
            length = 2;
            initialByte = initialByte & 0x3F;
        }
        else if ((initialByte & 0xE0) == 0xC0)
        {
            length = 4;
            initialByte = initialByte & 0x1F;
        }
        else
        {
            throw new Error("Invalid compressed integer in blob");
        }

        for (var i = 1; i < length; ++i)
        {
            initialByte = (initialByte << 8) | this.__blob.ReadValue(1, this.__cursor + i);
        }

        return {
            value: initialByte,
            length: length,
        }
    }

    consumeString()
    {
        var result = this.peekString();
        this.__cursor += result.length;
        return result.value;
    }

    peekString()
    {
        var lenInfo = this.peekUnsigned();
        var str = this.__blob.ReadString(lenInfo.value, this.__cursor + lenInfo.length);
        return {
            value: str,
            length: lenInfo.length + lenInfo.value
        };
    }
}

class __Guid
{
    constructor(data0, data1, data2, data3)
    {
        this.__data0 = data0;
        this.__data1 = data1;
        this.__data2 = data2;
        this.__data3 = data3;
    }

    toString()
    {
        return __valueToHex(this.__data0, 4) + "-" +
            __valueToHex(this.__data1, 2) + "-" +
            __valueToHex(this.__data2, 2) + "-" +
            __valueToHex(this.__data3[0], 1) + __valueToHex(this.__data3[1], 1) + "-" +
            __valueToHex(this.__data3[2], 1) + __valueToHex(this.__data3[3], 1) + __valueToHex(this.__data3[4], 1) +
            __valueToHex(this.__data3[5], 1) + __valueToHex(this.__data3[6], 1) + __valueToHex(this.__data3[7], 1);
    }
}

class __DatabaseVisualizer
{
    getString(index)
    {
        if (index == 0)
        {
            return null;
        }

        var ptr = this.m_strings.m_first.add(index);
        return host.memory.readString(ptr.address);
    }

    getBlob(index)
    {
        var ptr = this.m_blobs.m_first.add(index);
        var initialByte = host.memory.readMemoryValues(ptr.address, 1, 1)[0];

        // High bits indicate how many bytes are required to represent the blob size
        var blobSizeBytes;
        var blobSize;
        switch (initialByte >> 5)
        {
            case 0:
            case 1:
            case 2:
            case 3:
                blobSizeBytes = 1;
                blobSize = initialByte & 0x7F;
                break;

            case 4:
            case 5:
                blobSizeBytes = 2;
                blobSize = initialByte & 0x3F;
                break;

            case 6:
                blobSizeBytes = 4;
                blobSize = initialByte & 0x1F;
                break;

            default:
                throw new RangeError("Invalid blob encoding");
        }

        for (var i = 1; i < blobSizeBytes; ++i)
        {
            ptr = ptr.add(1);
            blobSize = (blobSize << 8) | host.memory.readMemoryValues(ptr.address, 1, 1)[0]
        }

        return new __Blob(ptr.add(1), blobSize);
    }

    getGuid(index)
    {
        var ptr = this.m_guids.m_first.add(index - 1);
        return new __Guid(
            host.memory.readMemoryValues(ptr.address, 1, 4)[0],
            host.memory.readMemoryValues(ptr.add(4).address, 1, 2)[0],
            host.memory.readMemoryValues(ptr.add(6).address, 1, 2)[0],
            host.memory.readMemoryValues(ptr.add(8).address, 8, 1));
    }

    get TypeRef()
    {
        return this.TypeRef;
    }

    get GenericParamConstraint()
    {
        return this.GenericParamConstraint;
    }

    get TypeSpec()
    {
        return this.TypeSpec;
    }

    get TypeDef()
    {
        return this.TypeDef;
    }

    get CustomAttribute()
    {
        return this.CustomAttribute;
    }

    get MethodDef()
    {
        return this.MethodDef;
    }

    get MemberRef()
    {
        return this.MemberRef;
    }

    get Module()
    {
        return this.Module;
    }

    get Param()
    {
        return this.Param;
    }

    get InterfaceImpl()
    {
        return this.InterfaceImpl;
    }

    get Constant()
    {
        return this.Constant;
    }

    get Field()
    {
        return this.Field;
    }

    get FieldMarshal()
    {
        return this.FieldMarshal;
    }

    get DeclSecurity()
    {
        return this.DeclSecurity;
    }

    get ClassLayout()
    {
        return this.ClassLayout;
    }

    get FieldLayout()
    {
        return this.FieldLayout;
    }

    get StandAloneSig()
    {
        return this.StandAloneSig;
    }

    get EventMap()
    {
        return this.EventMap;
    }

    get Event()
    {
        return this.Event;
    }

    get PropertyMap()
    {
        return this.PropertyMap;
    }

    get Property()
    {
        return this.Property;
    }

    get MethodSemantics()
    {
        return this.MethodSemantics;
    }

    get MethodImpl()
    {
        return this.MethodImpl;
    }

    get ModuleRef()
    {
        return this.ModuleRef;
    }

    get ImplMap()
    {
        return this.ImplMap;
    }

    get FieldRVA()
    {
        return this.FieldRVA;
    }

    get Assembly()
    {
        return this.Assembly;
    }

    get AssemblyProcessor()
    {
        return this.AssemblyProcessor;
    }

    get AssemblyOS()
    {
        return this.AssemblyOS;
    }

    get AssemblyRef()
    {
        return this.AssemblyRef;
    }

    get AssemblyRefProcessor()
    {
        return this.AssemblyRefProcessor;
    }

    get AssemblyRefOS()
    {
        return this.AssemblyRefOS;
    }

    get File()
    {
        return this.File;
    }

    get ExportedType()
    {
        return this.ExportedType;
    }

    get ManifestResource()
    {
        return this.ManifestResource;
    }

    get NestedClass()
    {
        return this.NestedClass;
    }

    get GenericParam()
    {
        return this.GenericParam;
    }

    get MethodSpec()
    {
        return this.MethodSpec;
    }
}

function __MakeTableVisualizer(type)
{
    class __TableVisualizer
    {
        get Database()
        {
            return this.m_database;
        }

        get Size()
        {
            return this.m_row_count;
        }

        getDimensionality()
        {
            return 1;
        }

        getValueAt(index)
        {
            if (type != "undefined")
            {
                return eval("new " + type + "(this, index)");
            }

            // For an untyped table, return values as arrays
            var result = new Array();
            var ptr = this.m_data.add(index * this.m_row_size);
            for (var i = 0; i < this.m_columns.Count(); ++i)
            {
                var dataSize = this.m_columns[index].size;
                var dataPtr = ptr.add(this.m_columns[i].offset);
                var value = host.memory.readMemoryValues(dataPtr.address, 1, dataSize)[0];
                result.push(value);
            }

            return result;
        }

        *[Symbol.iterator]()
        {
            var ptr = this.m_data;
            for (var i = 0; i < this.m_row_count; ++i)
            {
                yield new host.indexedValue(this.getValueAt(i), [i]);
            }
        }

        getValue(row, col)
        {
            if (row >= this.m_row_count)
            {
                throw new RangeError("Row index out of range: " + row);
            }
            else if (col >= this.m_columns.Count())
            {
                throw new RangeError("Column index out of range: " + col);
            }

            var dataSize = this.m_columns[col].size;
            var ptr = this.m_data.add(row * this.m_row_size + this.m_columns[col].offset);
            return host.memory.readMemoryValues(ptr.address, 1, dataSize)[0];
        }

        __lowerBound(col, value, min, max)
        {
            // First value that is not less than value
            min = min || 0;
            max = max || this.Size;

            while (min < max)
            {
                var mid = Math.floor((min + max) / 2);
                var testValue = this.getValue(mid, col);
                if (testValue < value)
                {
                    min = mid + 1;
                }
                else
                {
                    max = mid;
                }
            }

            return min;
        }

        __upperBound(col, value, min, max)
        {
            // First value that is greater than value
            min = min || 0;
            max = max || this.Size;

            while (min < max)
            {
                var mid = Math.floor((min + max) / 2);
                var testValue = this.getValue(mid, col);
                if (testValue <= value)
                {
                    min = mid + 1;
                }
                else
                {
                    max = mid;
                }
            }

            return min;
        }

        __equalRange(col, value, min, max)
        {
            var begin = this.__lowerBound(col, value, min, max);
            var end = this.__upperBound(col, value, begin, max);
            return new __TableRange(this, begin, end);
        }

        __fromCodedIndex(codedIndex, codedEnum)
        {
            var typeIndex = __codedValueType(codedIndex, codedEnum);
            if (typeIndex >= codedEnum.Values.length)
            {
                throw new RangeError("Invalid " + codedEnum.Name + " coded index: " + typeIndex);
            }

            var target = codedEnum.Values[typeIndex];
            if (target === null)
            {
                throw new RangeError("Invalid " + codedEnum.Name + " coded index: " + typeIndex);
            }

            var table = this.Database[target];
            var row = __codedValueIndex(codedIndex, codedEnum);
            if (row == -1)
            {
                return null;
            }

            return eval("new __" + target + "Visualizer(table, row)");
        }

        __getCodedIndex(row, col, codedEnum)
        {
            return this.__fromCodedIndex(this.getValue(row, col), codedEnum);
        }

        __getList(tableName, row, col)
        {
            // Lists are stored as a single index in the current row. This marks the beginning of the list. The next row in
            // the current table is the index of one past our end
            var table = this.Database[tableName];
            var begin = this.getValue(row, col);
            var end = table.Size;
            if (row + 1 < this.Size)
            {
                end = this.getValue(row + 1, col);
            }

            return new __TableRange(table, begin - 1, end - 1);
        }

        __getTargetRow(tableName, row, col)
        {
            return this.Database[tableName][this.getValue(row, col) - 1];
        }

        __getParentRow(tableName, row, col)
        {
            // The parent references the first row in this table that belongs to its list. Thus, we need to look for the
            // first value in the parent's table that references a later value in this table and then choose the previous
            var table = this.Database[tableName];
            var index = table.__upperBound(col, row + 1) - 1;
            return table[index];
        }
    }

    return __TableVisualizer;
}

class __TableRange
{
    constructor(table, begin, end)
    {
        this.__table = table;
        this.__begin = begin;
        this.__end = end;
    }

    get Size()
    {
        return this.__end - this.__begin;
    }

    getDimensionality()
    {
        return 1;
    }

    getValueAt(index)
    {
        var pos = this.__begin + index;
        if ((pos < this.__begin) || (pos >= this.__end))
        {
            throw new RangeError("Index out of range: " + index);
        }

        return this.__table[this.__begin + index];
    }

    *[Symbol.iterator]()
    {
        for (var i = this.__begin; i < this.__end; ++i)
        {
            yield new host.indexedValue(this.__table[i], [i - this.__begin]);
        }
    }
}

class __RowBase
{
    constructor(table, index)
    {
        this.__table = table;
        this.__index = index;
    }

    get TableName()
    {
        var result;
        if (this.targetType)
        {
            // C++ name; may also be a template argument
            result = this.targetType.name;
            var pos = result.lastIndexOf("::");
            if (pos != -1)
            {
                result = result.substr(pos + 2);
            }

            pos = result.indexOf(">");
            if (pos != -1)
            {
                result = result.substr(0, pos);
            }
        }
        else
        {
            result = this.constructor.name;
            result = result.substr(2, result.length - 12);
        }

        return result;
    }

    get __Table()
    {
        return this.m_table || this.__table;
    }

    get __Index()
    {
        return (this.m_index === undefined) ? this.__index : this.m_index;
    }

    __getValue(col)
    {
        return this.__Table.getValue(this.__Index, col);
    }

    __getString(col)
    {
        return this.__Table.Database.getString(this.__getValue(col));
    }

    __getBlob(col)
    {
        return this.__Table.Database.getBlob(this.__getValue(col));
    }

    __getCodedIndex(col, codedEnum)
    {
        return this.__Table.__getCodedIndex(this.__Index, col, codedEnum);
    }

    __getBlob(col)
    {
        return this.__Table.Database.getBlob(this.__getValue(col));
    }

    __getGuid(col)
    {
        return this.__Table.Database.getGuid(this.__getValue(col));
    }

    __equalRange(tableName, col, codedEnum)
    {
        // We're looking for range of elements in a different table that correspond to the row in this table. This is
        // done by inspecting coded index values in one of the table's columns to find the range that corresponds to the
        // current row
        var table = this.__Table.Database[tableName];
        var codedValue;
        if (codedEnum)
        {
            codedValue = __makeCodedValue(this.__Index, codedEnum[this.TableName], codedEnum);
        }
        else
        {
            codedValue = this.__Index + 1;
        }

        return table.__equalRange(col, codedValue);
    }

    __getList(tableName, col)
    {
        return this.__Table.__getList(tableName, this.__Index, col);
    }

    __getTargetRow(tableName, col)
    {
        return this.__Table.__getTargetRow(tableName, this.__Index, col);
    }

    __getParentRow(tableName, col)
    {
        return this.__Table.__getParentRow(tableName, this.__Index, col);
    }

    __findConstant()
    {
        var range = this.__equalRange("Constant", 1, __HasConstant);
        if (range.Size == 0)
        {
            return null;
        }
        else if (range.Size != 1)
        {
            throw new RangeError("Expected only one constant value, found: " + range.Size);
        }

        return range.getValueAt(0);
    }
}



// Table Row Types

var __AssemblyHashAlgorithm = Object.freeze({
    None: 0x0000,
    Reserved_MD5: 0x8003,
    SHA1: 0x8004,
});

var __AssemblyFlags = Object.freeze({
    PublicKey: 0x0001,
    Retargetable: 0x0100,
    WindowsRuntime: 0x0200,
    DisableJITcompileOptimizer: 0x4000,
    EnableJITcompileTracking: 0x8000,
});

class __AssemblyVersion
{
    constructor(value)
    {
        // 64-bit value
        this.MajorVersion = value.bitwiseAnd(0xFFFF).asNumber();
        this.MinorVersion = value.bitwiseShiftRight(16).bitwiseAnd(0xFFFF).asNumber();
        this.BuildNumber = value.bitwiseShiftRight(32).bitwiseAnd(0xFFFF).asNumber();
        this.RevisionNumber = value.bitwiseShiftRight(48).bitwiseAnd(0xFFFF).asNumber();
    }

    toString()
    {
        return this.MajorVersion + "." + this.MinorVersion + "." + this.BuildNumber + "." + this.RevisionNumber;
    }
}

class __AssemblyVisualizer extends __RowBase
{
    toString()
    {
        return this.Name;
    }

    get Name()
    {
        return this.__getString(4);
    }

    get HashAlgId()
    {
        return __enumToString(this.__getValue(0), __AssemblyHashAlgorithm);
    }

    get Version()
    {
        return new __AssemblyVersion(this.__getValue(1));
    }

    get Flags()
    {
        return __enumFlagsToString(this.__getValue(2), __AssemblyFlags);
    }

    get PublicKey()
    {
        this.__getBlob(3);
    }

    get Culture()
    {
        return this.__getString(5);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __AssemblyOSVisualizer extends __RowBase
{
    get OSPlatformId()
    {
        return this.__getValue(0);
    }

    get OSMajorVersion()
    {
        return this.__getValue(1);
    }

    get OSMinorVersion()
    {
        return this.__getValue(2);
    }
}

class __AssemblyProcessorVisualizer extends __RowBase
{
    toString()
    {
        return this.Processor;
    }

    get Processor()
    {
        return this.__getValue(0);
    }
}

class __AssemblyRefVisualizer extends __RowBase
{
    toString()
    {
        return this.Name;
    }

    get Name()
    {
        return this.__getString(3);
    }

    get Flags()
    {
        return __enumFlagsToString(this.__getValue(1), __AssemblyFlags);
    }

    get Version()
    {
        return new __AssemblyVersion(this.__getValue(0));
    }

    get PublicKeyOrToken()
    {
        return this.__getBlob(2);
    }

    get Culture()
    {
        return this.__getString(4);
    }

    get HashValue()
    {
        return this.__getString(5);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __AssemblyRefOSVisualizer extends __RowBase
{
    toString()
    {
        return this.AssemblyRef.toString();
    }

    get OSPlatformId()
    {
        return this.__getValue(0);
    }

    get OSMajorVersion()
    {
        return this.__getValue(1);
    }

    get OSMinorVersion()
    {
        return this.__getValue(2);
    }

    get AssemblyRef()
    {
        return this.__getTargetRow("AssemblyRef", 3);
    }
}

class __AssemblyRefProcessorVisualizer extends __RowBase
{
    toString()
    {
        return this.AssemblyRef.toString();
    }

    get Processor()
    {
        return this.__getValue(0);
    }

    get AssemblyRef()
    {
        return this.__getTargetRow("AssemblyRef", 1);
    }
}

class __ClassLayoutVisualizer extends __RowBase
{
    get PackingSize()
    {
        return this.__getValue(0);
    }

    get ClassSize()
    {
        return this.__getValue(1);
    }

    get Parent()
    {
        return this.__getTargetRow("TypeDef", 2);
    }
};

class __ConstantVisualizer extends __RowBase
{
    toString()
    {
        return this.Type + "(" + this.Value + ")";
    }

    get Type()
    {
        switch (this.__getValue(0))
        {
        case 2: return "Boolean";
        case 3: return "Char";
        case 4: return "Int8";
        case 5: return "UInt8";
        case 6: return "Int16";
        case 7: return "UInt16";
        case 8: return "Int32";
        case 9: return "UInt32";
        case 10: return "Int64";
        case 11: return "UInt64";
        case 12: return "Float32";
        case 13: return "Float64";
        case 14: return "String";
        case 18: return "Class";
        default: throw new Error("Unknown type: " + this.__getValue(0));
        }
    }

    get Value()
    {
        var blob = this.__getBlob(2);
        switch (this.__getValue(0))
        {
            case 2: return blob.ReadValue(1) == 1 ? true : false;
            case 3: return 'a' + blob.ReadValue(2) - 'a';
            case 4:
            case 5: return blob.ReadValue(1);
            case 6:
            case 7: return blob.ReadValue(2);
            case 8:
            case 9: return blob.ReadValue(4);
            case 10:
            case 11: return blob.ReadValue(8);
            case 12:
            case 13: return "Floating Point; not yet supported";
            case 14: return blob.ReadString();
            case 18: return "Class; not yet supported";
            default: throw new Error("Unknown type");
        }
    }

    get Parent()
    {
        return this.__getCodedIndex(1, __HasConstant);
    }
}

class __CustomAttributeVisualizer extends __RowBase
{
    toString()
    {
        var result = this.Type.toString() + "(";
        var args = this.Value.FixedArgs;
        var prefix = "";
        for (var i = 0; i < args.length; ++i)
        {
            result += prefix + args[i].toString();
            prefix = ", ";
        }

        return result + ")";
    }

    get Parent()
    {
        return this.__getCodedIndex(0, __HasCustomAttribute);
    }

    get Type()
    {
        return this.__getCodedIndex(1, __CustomAttributeType);
    }

    get Value()
    {
        var sig;
        var type = this.Type;
        if (type.TableName == "MemberRef")
        {
            sig = type.MethodSignature;
        }
        else
        {
            sig = type.Signature;
        }

        return new __CustomAttributeSig(this.__Table, new __BlobStream(this.__getBlob(2)), sig);
    }
}

var __SecurityAction = Object.freeze({
    Assert: 3,
    Demand: 2,
    Deny: 4,
    InheritanceDemand: 7,
    LinkDemand: 6,
    PermitOnly: 5,
    RequestMinimum: 8,
    RequestOptional: 9,
    RequestRefuse: 10,
});

class __DeclSecurityVisualizer extends __RowBase
{
    get Action()
    {
        return __enumToString(this.__getValue(0), __SecurityAction);
    }

    get Parent()
    {
        return this.__getCodedIndex(1, __HasDeclSecurity);
    }

    get PermissionSet()
    {
        // NOTE: Table seems to be unused by WinMD, so leaving unimplemented for now
        return this.__getBlob(2);
    }
}

class __EventMapVisualizer extends __RowBase
{
    toString()
    {
        return this.Parent.toString();
    }

    get Parent()
    {
        return this.__getTargetRow("TypeDef", 0);
    }

    get EventList()
    {
        return this.__getList("Event", 1);
    }
}

var __EventAttributes = Object.freeze({
    SpecialName: 0x0200,
    RTSpecialName: 0x0400,
});

class __EventVisualizer extends __RowBase
{
    toString()
    {
        return this.Parent.toString() + "::" + this.Name;
    }

    get Name()
    {
        return this.__getString(1);
    }

    get EventFlags()
    {
        return __enumFlagsToString(this.__getValue(0), __EventAttributes);
    }

    get EventType()
    {
        return this.__getCodedIndex(2, __TypeDefOrRef);
    }

    get Parent()
    {
        return this.__getParentRow("EventMap", 1).Parent;
    }

    get MethodSemantic()
    {
        return this.__equalRange("MethodSemantics", 2, __HasSemantics);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

var __MemberAccess = Object.freeze({
    // FieldAccessMask: 0x0007 - These 3 bits contain one of the following values:
    CompilerControlled: 0x0000,
    Private: 0x0001,
    FamANDAssem: 0x0002,
    Assembly: 0x0003,
    Family: 0x0004,
    FamORAssem: 0x0005,
    Public: 0x0006,
});

var __FieldAttributesFlags = Object.freeze({
    Static: 0x0010,
    InitOnly: 0x0020,
    Literal: 0x0040,
    NotSerialized: 0x0080,
    SpecialName: 0x0200,

    // Interop Attributes
    PInvokeImpl: 0x2000,

    // Additional flags
    RTSpecialName: 0x0400,
    HasFieldMarshal: 0x1000,
    HasDefault: 0x8000,
    HasFieldRVA: 0x0100,
});

class __FieldAttributes
{
    constructor(value)
    {
        this.__value = value;
    }

    toString()
    {
        var accessPart = this.__value & 0x0007;
        var flagsPart = this.__value & ~0x0007;

        var result = __enumToString(accessPart, __MemberAccess);
        if (flagsPart != 0)
        {
            result += " | " + __enumFlagsToString(flagsPart, __FieldAttributesFlags);
        }

        return result;
    }

    get Value()
    {
        return this.__value;
    }
}

class __ExportedTypeVisualizer extends __RowBase
{
    toString()
    {
        return this.TypeNamespace + "." + this.TypeName;
    }

    get Flags()
    {
        return new __FieldAttributes(this.__getValue(0));
    }

    get TypeDefId()
    {
        return this.__getValue(1);
    }

    get TypeName()
    {
        return this.__getString(2);
    }

    get TypeNamespace()
    {
        return this.__getString(3);
    }

    get Implementation()
    {
        return this.__getCodedIndex(4, __Implementation);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __FieldVisualizer extends __RowBase
{
    toString()
    {
        if (this.Constant == null)
        {
            return this.Name;
        }
        else
        {
            return this.Name + " = " + this.Constant.toString();
        }
    }

    get Name()
    {
        return this.__getString(1);
    }

    get Constant()
    {
        return this.__findConstant();
    }

    get Flags()
    {
        return new __FieldAttributes(this.__getValue(0));
    }

    get Signature()
    {
        return new __FieldSig(this.__Table, new __BlobStream(this.__getBlob(2)));
    }

    get Parent()
    {
        return this.__getParentRow("TypeDef", 4);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __FieldLayoutVisualizer extends __RowBase
{
    toString()
    {
        return this.Field.toString();
    }

    get Offset()
    {
        return this.__getValue(0);
    }

    get Field()
    {
        return this.__getTargetRow("Field", 1);
    }
}

class __FieldMarshalVisualizer extends __RowBase
{
    get Parent()
    {
        return this.__getCodedIndex(0, __HasFieldMarshal);
    }

    get NativeType()
    {
        // NOTE: Table seems to be unused by WinMD, so leaving unimplemented for now
        return this.__getBlob(1);
    }
}

class __FieldRVAVisualizer extends __RowBase
{
    toString()
    {
        return this.Field.toString();
    }

    get RVA()
    {
        return this.__getValue(0);
    }

    get Field()
    {
        return this.__getTargetRow("Field", 1);
    }
}

var __FileAttributes = Object.freeze({
    ContainsMetaData: 0x0000,
    ContainsNoMetadata: 0x0001,
});

class __FileVisualizer extends __RowBase
{
    toString()
    {
        return this.Name;
    }

    get Flags()
    {
        return __enumToString(this.__getValue(0), __FileAttributes);
    }

    get Name()
    {
        return this.__getString(1);
    }

    get HashValue()
    {
        return this.__getBlob(2);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

var __GenericParamAttributes = Object.freeze({
    Covariant: 0x0001,
    Contravariant: 0x0002,
    ReferenceTypeConstraint: 0x0004,
    NotNullableValueTypeConstraint: 0x0008,
    DefaultConstructorConstraint: 0x0010,
});

class __GenericParamVisualizer extends __RowBase
{
    toString()
    {
        return this.Name;
    }

    get Name()
    {
        return this.__getString(3);
    }

    get Number()
    {
        return this.__getValue(0);
    }

    get Flags()
    {
        return __enumFlagsToString(this.__getValue(1), __GenericParamAttributes);
    }

    get Owner()
    {
        return this.__getCodedIndex(2, __TypeOrMethodDef);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __GenericParamConstraintVisualizer extends __RowBase
{
    toString()
    {
        return "where " + this.Owner.toString() + " : " + this.Constraint.toString();
    }

    get Owner()
    {
        return this.__getTargetRow("GenericParam", 0);
    }

    get Constraint()
    {
        return this.__getCodedIndex(1, __TypeDefOrRef);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

var __CharSet = Object.freeze({
    CharSetNotSpec: 0x0000,
    CharSetAnsi: 0x0002,
    CharSetUnicode: 0x0004,
    CharSetAuto: 0x0006,
});

var __CallConv = Object.freeze({
    CallConvPlatformapi: 0x0100,
    CallConvCdecl: 0x0200,
    CallConvStdcall: 0x0300,
    CallConvThiscall: 0x0400,
    CallConvFastcall: 0x0500,
});

var __PInvokeAttributesFlags = Object.freeze({
    NoMangle: 0x0001,
    SupportsLastError: 0x0040,
});

class __PInvokeAttributes
{
    constructor(value)
    {
        this.__value = value;
    }

    toString()
    {
        var charSetPart = this.__value & 0x0006;
        var callConvPart = this.__value & 0x0700;
        var flagsPart = this.__value & ~0x0706;
        var result = __enumToString(charSetPart, __CharSet);
        result += " | " + __enumToString(callConvPart, __CallConv);
        if (flagsPart != 0)
        {
            result += " | " + __enumFlagsToString(flagsPart, __PInvokeAttributesFlags);
        }

        return result;
    }

    get Value()
    {
        return this.__value;
    }
}

class __ImplMapVisualizer extends __RowBase
{
    toString()
    {
        return this.ImportName;
    }

    get MappingFlags()
    {
        return new __PInvokeAttributes(this.__getValue(0));
    }

    get MemberForwarded()
    {
        return this.__getCodedIndex(1, __MemberForwarded);
    }

    get ImportName()
    {
        return this.__getString(2);
    }

    get ImportScope()
    {
        return this.__getTargetRow("ModuleRef", 3);
    }
}

class __InterfaceImplVisualizer extends __RowBase
{
    toString()
    {
        return this.Interface.toString();
    }

    get Class()
    {
        return this.__getTargetRow("TypeDef", 0);
    }

    get Interface()
    {
        return this.__getCodedIndex(1, __TypeDefOrRef);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

var __ManifestResourceAttributes = Object.freeze({
    Public: 0x0001,
    Private: 0x0002,
});

class __ManifestResourceVisualizer extends __RowBase
{
    toString()
    {
        return this.Name;
    }

    get Offset()
    {
        return this.__getValue(0);
    }

    get Flags()
    {
        return __enumToString(this.__getValue(1), __ManifestResourceAttributes);
    }

    get Name()
    {
        return this.__getString(2);
    }

    get Implementation()
    {
        return this.__getCodedIndex(3, __Implementation);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __MemberRefVisualizer extends __RowBase
{
    toString()
    {
        return this.Class.toString() + "::" + this.Name;
    }

    get Name()
    {
        return this.__getString(1);
    }

    get Class()
    {
        return this.__getCodedIndex(0, __MemberRefParent);
    }

    get MethodSignature()
    {
        return new __MethodDefSig(this.__Table, new __BlobStream(this.__getBlob(2)));
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

var __CodeType = Object.freeze({
    IL: 0x0000,
    Native: 0x0001,
    OPTIL: 0x0002,
    Runtime: 0x0003,
});

var __ManagedType = Object.freeze({
    Managed: 0x0000,
    Unmanaged: 0x0004,
});

var __MethodImplAttributesFlags = Object.freeze({
    ForwardRef: 0x0010,
    PreserveSig: 0x0080,
    InternalCall: 0x1000,
    Synchronized: 0x0020,
    NoInlining: 0x0008,
    NoOptimization: 0x0040,
});

class __MethodImplAttributes
{
    constructor(value)
    {
        this.__value = value;
    }

    toString()
    {
        var codeTypePart = this.__value & 0x0003;
        var managedTypePart = this.__value & 0x0004;
        var flagsPart = this.__value & ~0x0007;
        var result = __enumToString(codeTypePart, __CodeType);
        result += " | " + __enumToString(managedTypePart, __ManagedType);
        if (flagsPart != 0)
        {
            result += " | " + __enumFlagsToString(flagsPart, __MethodImplAttributesFlags);
        }

        return result;
    }

    get Value()
    {
        return this.__value;
    }
}

var __VtableLayout = Object.freeze({
    ReuseSlot: 0x0000,
    NewSlot: 0x0100,
});

var __MethodAttributesFlags = Object.freeze({
    Static: 0x0010,
    Final: 0x0020,
    Virtual: 0x0040,
    HideBySig: 0x0080,
    Strict: 0x0200,
    Abstract: 0x0400,
    SpecialName: 0x0800,
    PInvokeImpl: 0x2000,
    UnmanagedExport: 0x0008,
    RTSpecialName: 0x1000,
    HasSecurity: 0x4000,
    RequireSecObject: 0x8000,
});

class __MethodAttributes
{
    constructor(value)
    {
        this.__value = value;
    }

    toString()
    {
        var accessPart = this.__value & 0x0007;
        var vtablePart = this.__value & 0x0100;
        var flagsPart = this.__value & ~0x0107;
        var result = __enumToString(accessPart, __MemberAccess);
        result += " | " + __enumToString(vtablePart, __VtableLayout);
        if (flagsPart != 0)
        {
            result += " | " + __enumFlagsToString(flagsPart, __MethodAttributesFlags);
        }

        return result;
    }

    get Value()
    {
        return this.__value;
    }
}

class __MethodDefVisualizer extends __RowBase
{
    toString()
    {
        return this.Parent.toString() + "::" + this.Name;
    }

    get Name()
    {
        return this.__getString(3);
    }

    get RVA()
    {
        return this.__getValue(0);
    }

    get ImplFlags()
    {
        return new __MethodImplAttributes(this.__getValue(1));
    }

    get Flags()
    {
        return new __MethodAttributes(this.__getValue(2));
    }

    get Signature()
    {
        return new __MethodDefSig(this.__Table, new __BlobStream(this.__getBlob(4)));
    }

    get Parent()
    {
        return this.__getParentRow("TypeDef", 5);
    }

    get ParamList()
    {
        return this.__getList("Param", 5);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __MethodImplVisualizer extends __RowBase
{
    toString()
    {
        return this.MethodBody.toString();
    }

    get Class()
    {
        return this.__getTargetRow("TypeDef", 0);
    }

    get MethodBody()
    {
        return this.__getCodedIndex(1, __MethodDefOrRef);
    }

    get MethodDeclaration()
    {
        return this.__getCodedIndex(2, __MethodDefOrRef);
    }
}

var __MethodSemanticsAttributes = Object.freeze({
    Setter: 0x0001,
    Getter: 0x0002,
    Other: 0x0004,
    AddOn: 0x0008,
    RemoveOn: 0x0010,
    Fire: 0x0020,
});

class __MethodSemanticsVisualizer extends __RowBase
{
    toString()
    {
        return this.Method.toString();
    }

    get Semantic()
    {
        return __enumFlagsToString(this.__getValue(0), __MethodSemanticsAttributes);
    }

    get Method()
    {
        return this.__getTargetRow("MethodDef", 1);
    }

    get Association()
    {
        return this.__getCodedIndex(2, __HasSemantics);
    }
}

class __MethodSpecVisualizer extends __RowBase
{
    toString()
    {
        return this.Method.toString();
    }

    get Method()
    {
        return this.__getCodedIndex(0, __MethodDefOrRef);
    }

    get Instantiation()
    {
        // NOTE: Table seems to be unused by WinMD, so leaving unimplemented for now
        return this.__getBlob(1);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __ModuleVisualizer extends __RowBase
{
    toString()
    {
        return this.Name;
    }

    get Generation()
    {
        return this.__getValue(0);
    }

    get Name()
    {
        return this.__getString(1);
    }

    get Mvid()
    {
        return this.__getGuid(2);
    }

    get EncId()
    {
        return this.__getGuid(3);
    }

    get EncBaseId()
    {
        return this.__getGuid(4);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __ModuleRefVisualizer extends __RowBase
{
    toString()
    {
        return this.Name;
    }

    get Name()
    {
        return this.__getString(0);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __NestedClassVisualizer extends __RowBase
{
    get NestedClass()
    {
        return this.__getTargetRow("TypeDef", 0);
    }

    get EnclosingClass()
    {
        return this.__getTargetRow("TypeDef", 1);
    }
}

var __ParamAttributes = Object.freeze({
    In: 0x0001,
    Out: 0x0002,
    Optional: 0x0010,
    HasDefault: 0x1000,
    HasFieldMarshal: 0x2000,
});

class __ParamVisualizer extends __RowBase
{
    toString()
    {
        return this.Name;
    }

    get Name()
    {
        return this.__getString(2);
    }

    get Flags()
    {
        return __enumFlagsToString(this.__getValue(0), __ParamAttributes);
    }

    get Sequence()
    {
        return this.__getValue(1);
    }

    get Constant()
    {
        return this.__findConstant();
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

var __PropertyAttributes = Object.freeze({
    SpecialName: 0x0200,
    RTSpecialName: 0x0400,
    HasDefault: 0x1000,
});

class __PropertyVisualizer extends __RowBase
{
    toString()
    {
        return this.Parent.toString() + "::" + this.Name;
    }

    get Name()
    {
        return this.__getString(1);
    }

    get Flags()
    {
        return __enumFlagsToString(this.__getValue(0), __PropertyAttributes);
    }

    get Type()
    {
        return new __PropertySig(this.__Table, new __BlobStream(this.__getBlob(2)));
    }

    get Parent()
    {
        return this.__getParentRow("PropertyMap", 1).Parent;
    }

    get Constant()
    {
        return this.__findConstant();
    }

    get MethodSemantic()
    {
        return this.__equalRange("MethodSemantics", 2, __HasSemantics);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

class __PropertyMapVisualizer extends __RowBase
{
    toString()
    {
        return this.Parent.toString();
    }

    get Parent()
    {
        return this.__getTargetRow("TypeDef", 0);
    }

    get PropertyList()
    {
        return this.__getList("Property", 1);
    }
}

class __StandAloneSigVisualizer extends __RowBase
{
    get Signature()
    {
        // NOTE: Table seems to be unused by WinMD, so leaving unimplemented for now
        return this.__getBlob(0);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}

var __TypeVisibility = Object.freeze({
    NotPublic: 0x00000000,
    Public: 0x00000001,
    NestedPublic: 0x00000002,
    NestedPrivate: 0x00000003,
    NestedFamily: 0x00000004,
    NestedAssembly: 0x00000005,
    NestedFamANDAssem: 0x00000006,
    NestedFamORAssem: 0x00000007,
});

var __TypeLayout = Object.freeze({
    AutoLayout: 0x00000000,
    SequentialLayout: 0x00000008,
    ExplicitLayout: 0x00000010,
});

var __TypeSemantics = Object.freeze({
    Class: 0x00000000,
    Interface: 0x00000020,
});

var __StringFormat = Object.freeze({
    AnsiClass: 0x00000000,
    UnicodeClass: 0x00010000,
    AutoClass: 0x00020000,
    CustomFormatClass: 0x00030000,
});

var __TypeAttributesFlags = Object.freeze({
    Abstract: 0x00000080,
    Sealed: 0x00000100,
    SpecialName: 0x00000400,
    Import: 0x00001000,
    Serializable: 0x00002000,
    WindowsRuntime: 0x00004000,

    BeforeFieldInit: 0x00100000,
    RTSpecialName: 0x00000800,
    HasSecurity: 0x00040000,
    IsTypeForwarder: 0x00200000,
});

class __TypeAttributes
{
    constructor(value)
    {
        this.__value = value;
    }

    toString()
    {
        var visibilityPart = this.__value & 0x00000007;
        var layoutPart = this.__value & 0x00000018;
        var semanticsPart = this.__value & 0x00000020;
        var stringFormatPart = this.__value & 0x00030000;
        var flagsPart = this.__value & ~0x0003003F;

        var result = __enumToString(visibilityPart, __TypeVisibility);
        result += " | " + __enumToString(layoutPart, __TypeLayout);
        result += " | " + __enumToString(semanticsPart, __TypeSemantics);
        result += " | " + __enumToString(stringFormatPart, __StringFormat);
        if (flagsPart != 0)
        {
            result += " | " + __enumFlagsToString(flagsPart, __TypeAttributesFlags);
        }

        return result;
    }

    get Value()
    {
        return this.__value;
    }
}

class __TypeDefVisualizer extends __RowBase
{
    toString()
    {
        var result = this.TypeName;

        if (this.TypeNamespace != null)
        {
            result = this.TypeNamespace + "." + result;
        }

        var genericParams = this.GenericParam;
        if (genericParams.Size != 0)
        {
            result += "<";
            var prefix = "";
            for (var i = 0; i < genericParams.Size; ++i)
            {
                result += prefix + genericParams.getValueAt(i).Name;
                prefix = ", ";
            }
            result += ">";
        }

        return result;
    }

    get TypeName()
    {
        return this.__getString(1);
    }

    get TypeNamespace()
    {
        return this.__getString(2);
    }

    get Flags()
    {
        return new __TypeAttributes(this.__getValue(0));
    }

    get TypeName()
    {
        return this.__getString(1);
    }

    get TypeNamespace()
    {
        return this.__getString(2);
    }

    get Extends()
    {
        return this.__getCodedIndex(3, __TypeDefOrRef);
    }

    get FieldList()
    {
        return this.__getList("Field", 4);
    }

    get MethodList()
    {
        return this.__getList("MethodDef", 5);
    }

    get InterfaceImpl()
    {
        return this.__equalRange("InterfaceImpl", 0);
    }

    get GenericParam()
    {
        return this.__equalRange("GenericParam", 2, __TypeOrMethodDef);
    }

    get MethodImplList()
    {
        return this.__equalRange("MethodImpl", 0);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }

    GetPropertyList()
    {
        var table = this.__Table.Database.PropertyMap;
        var index = this.__Index + 1;
        var i = 0;
        for (; i < table.Size; ++i)
        {
            if (table.getValue(i, 0) == index)
            {
                break;
            }
        }

        if (i == table.Size)
        {
            return new __TableRange(table, i, i);
        }

        return table[i].PropertyList;
    }

    GetEventList()
    {
        var table = this.__Table.Database.EventMap;
        var index = this.__Index + 1;
        var i = 0;
        for (; i < table.Size; ++i)
        {
            if (table.getValue(i, 0) == index)
            {
                break;
            }
        }

        if (i == table.Size)
        {
            return new __TableRange(table, i, i);
        }

        return table[i].EventList;
    }
}

class __TypeRefVisualizer extends __RowBase
{
    toString()
    {
        return this.TypeNamespace + "." + this.TypeName;
    }

    get TypeName()
    {
        return this.__getString(1);
    }

    get TypeNamespace()
    {
        return this.__getString(2);
    }

    get ResolutionScope()
    {
        return this.__getCodedIndex(0, __ResolutionScope);
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }

    FindDefinition()
    {
        var searchString = this.TypeNamespace + "." + this.TypeName;

        // Search for a TypeDef instance of the same name
        var table = this.__Table.Database.TypeDef;
        var min = 0;
        var max = table.Size;
        while (min < max)
        {
            var mid = Math.floor((min + max) / 2);
            var def = table.getValueAt(mid);

            var testString = def.TypeNamespace + "." + def.TypeName;
            if (searchString < testString)
            {
                max = mid;
            }
            else if (searchString > testString)
            {
                min = mid + 1;
            }
            else
            {
                return def;
            }
        }

        // Not found
        return null;
    }
}

class __TypeSpecVisualizer extends __RowBase
{
    toString()
    {
        return this.Signature.toString();
    }

    get Signature()
    {
        return new __TypeSpecSig(this.__Table, new __BlobStream(this.__getBlob(0)));
    }

    get CustomAttribute()
    {
        return this.__equalRange("CustomAttribute", 0, __HasCustomAttribute);
    }
}



// coded_index<T> Types
class __CodedIndexBase
{
    toString()
    {
        return this.Type.toString();
    }

    get Type()
    {
        return this.m_table.__fromCodedIndex(this.m_value, this.__Mapping);
    }
}

class __TypeDefOrRefCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __TypeDefOrRef;
    }
}

class __HasConstantCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __HasConstant;
    }
}

class __HasCustomAttributeCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __HasCustomAttribute;
    }
}

class __HasFieldMarshalCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __HasFieldMarshal;
    }
}

class __HasDeclSecurityCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __HasDeclSecurity;
    }
}

class __MemberRefParentCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __MemberRefParent;
    }
}

class __HasSemanticsCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __HasSemantics;
    }
}

class __MethodDefOrRefCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __MethodDefOrRef;
    }
}

class __MemberForwardedCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __MemberForwarded;
    }
}

class __ImplementationCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __Implementation;
    }
}

class __CustomAttributeTypeCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __CustomAttributeType;
    }
}

class __ResolutionScopeCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __ResolutionScope;
    }
}

class __TypeOrMethodDefCodedIndexVisualizer extends __CodedIndexBase
{
    get __Mapping()
    {
        return __TypeOrMethodDef;
    }
}



// Blob Storage Types

// ELEMENT_TYPE enum; one byte in size
var __ElementType = Object.freeze({
    End: 0x00,
    Void: 0x01,
    Boolean: 0x02,
    Char: 0x03,
    I1: 0x04,
    U1: 0x05,
    I2: 0x06,
    U2: 0x07,
    I4: 0x08,
    U4: 0x09,
    I8: 0x0a,
    U8: 0x0b,
    R4: 0x0c,
    R8: 0x0d,
    String: 0x0e,
    Ptr: 0x0f,
    ByRef: 0x10,
    ValueType: 0x11,
    Class: 0x12,
    Var: 0x13,
    Array: 0x14,
    GenericInst: 0x15,
    TypedByRef: 0x16,
    I: 0x18,
    U: 0x19,
    FnPtr: 0x1b,
    Object: 0x1c,
    SZArray: 0x1d,
    MVar: 0x1e,
    CModReqd: 0x1f,
    CModOpt: 0x20,
    Internal: 0x21,
    Modifier: 0x40,
    Sentinel: 0x41,
    Pinned: 0x45,
    Type: 0x50,
    TaggedObject: 0x51,
    Field: 0x53,
    Property: 0x54,
    Enum: 0x55,
});
function __consumeElementType(stream)
{
    return stream.consumeValue(1);
}
function __peekElementType(stream)
{
    return stream.peekValue(1);
}

class __ElementTypeVisualizer
{
    constructor(value)
    {
        this.__value = value;
    }

    toString()
    {
        return __enumToString(this.__value, __ElementType);
    }

    get Value()
    {
        return this.__value;
    }
}

var __CallingConvention = Object.freeze({
    Default: 0x00,
    VarArg: 0x05,
    Field: 0x06,
    LocalSig: 0x07,
    Property: 0x08,
    GenericInst: 0x10,
    Mask: 0x0f,
});

var __CallingConventionFlags = Object.freeze({
    HasThis: 0x20,
    ExplicitThis: 0x40,
    Generic: 0x10,
});

function __consumeCallingConvention(stream)
{
    return stream.consumeValue(1);
}
function __callingConventionToString(value)
{
    var typePart = value & 0x0F;
    var flagsPart = value & ~0x0F;

    var result = __enumToString(typePart, __CallingConvention);
    if (flagsPart != 0)
    {
        result = __enumFlagsToString(flagsPart, __CallingConventionFlags) + " | " + result;
    }

    return result
}

function __consumeCodedIndex(stream, table, codedEnum)
{
    return table.__fromCodedIndex(stream.consumeUnsigned(), codedEnum);
}

function __consumeSzArray(stream)
{
    if (__peekElementType(stream) == __ElementType.SZArray)
    {
        stream.seek(1);
        return true;
    }

    return false;
}

function __consumeIsByRef(stream)
{
    if (__peekElementType(stream) == __ElementType.ByRef)
    {
        stream.seek(1);
        return true;
    }

    return false;
}



class __CustomModSig
{
    constructor(table, stream)
    {
        this.__elementType = __consumeElementType(stream);
        if ((this.__elementType != __ElementType.CModOpt) || (this.__elementType != __ElementType.CModReqd))
        {
            throw new Error("Invalid CustomMod signature element type: " + __enumToString(this.__elementType, __ElementType));
        }

        this.__type = __consumeCodedIndex(stream, table, __TypeDefOrRef);
    }

    get __ElementType()
    {
        return this.m_cmod || this.__elementType;
    }

    toString()
    {
        return this.Type.toString();
    }

    get Type()
    {
        return this.m_type || this.__type;
    }

    get ElementType()
    {
        return __enumToString(this.__ElementType, __ElementType);
    }
}

function __consumeCustomMods(table, stream)
{
    var result = new Array();

    // Continue until we read something other than ELEMENT_TYPE_CMOD_OPT or ELEMENT_TYPE_CMOD_REQD
    for (var type = __peekElementType(stream); (type == __ElementType.CModOpt) || (type == __ElementType.CModReqd); type = __peekElementType(stream))
    {
        result.push(new __CustomModSig(table, stream));
    }

    return result;
}

class __GenericTypeInstSig
{
    constructor(table, stream)
    {
        this.__type = stream.consumeValue(1);
        if ((this.__type != __ElementType.Class) && (this.__type != __ElementType.ValueType))
        {
            throw new Error("Generic type instance signatures must begin with either ELEMENT_TYPE_CLASS or ELEMENT_TYPE_VALUE");
        }

        this.__type = __consumeCodedIndex(stream, table, __TypeDefOrRef);

        var argCount = stream.consumeUnsigned();
        this.__genericArgs = new Array(argCount);
        for (var i = 0; i < argCount; ++i)
        {
            this.__genericArgs[i] = new __TypeSig(table, stream);
        }
    }

    get __ClassOrValue()
    {
        return this.m_class_or_value || this.__type;
    }

    toString()
    {
        var result = this.GenericType.toString() + "<";
        var prefix = "";

        // Handling STL types can get somewhat complicated...
        if (this.m_generic_args)
        {
            for (var i = 0; i < this.m_generic_args.Count(); ++i)
            {
                var type = this.m_generic_args[i].Type;
                var index = type.index();
                for (var j = 0; j < index; ++j)
                {
                    type = type._Tail;
                }

                // For whatever reason, this approach has issues printing out ElementType values...
                if (index == 0)
                {
                    result += prefix + __enumToString(type._Head, __ElementType);
                }
                else
                {
                    result += prefix + type._Head.toString();
                }

                prefix = ", ";
            }
        }
        else
        {
            for (var arg of this.__genericArgs)
            {
                result += prefix + arg.toString();
                prefix = ", ";
            }
        }
        result += ">";

        return result;
    }

    get ClassOrValueType()
    {
        return __enumToString(this.__ClassOrValue, __ElementType);
    }

    get GenericType()
    {
        return this.m_type || this.__type;
    }

    get GenericArgs()
    {
        // NOTE: m_generic_args is std::vector
        return this.m_generic_args || this.__genericArgs;
    }
}

class __GenericTypeIndex
{
    constructor(stream)
    {
        this.__index = stream.consumeUnsigned();
    }

    toString()
    {
        return this.Index.toString();
    }

    get Index()
    {
        return (this.index === undefined) ? this.__index : this.index;
    }
}

class __TypeSig
{
    constructor(table, stream)
    {
        this.__isSzArray = __consumeSzArray(stream);
        this.__customMods = __consumeCustomMods(table, stream);

        var type = __consumeElementType(stream);
        switch (type)
        {
        case __ElementType.Boolean:
        case __ElementType.Char:
        case __ElementType.I1:
        case __ElementType.U1:
        case __ElementType.I2:
        case __ElementType.U2:
        case __ElementType.I4:
        case __ElementType.U4:
        case __ElementType.I8:
        case __ElementType.U8:
        case __ElementType.R4:
        case __ElementType.R8:
        case __ElementType.String:
        case __ElementType.Object:
        case __ElementType.U:
        case __ElementType.I:
            this.__type = new __ElementTypeVisualizer(type);
            break;

        case __ElementType.Class:
        case __ElementType.ValueType:
            this.__type = __consumeCodedIndex(stream, table, __TypeDefOrRef);
            break;

        case __ElementType.GenericInst:
            this.__type = new __GenericTypeInstSig(table, stream);
            break;

        case __ElementType.Var:
            this.__type = new __GenericTypeIndex(stream);
            break;

        default:
            throw new Error("Unknown or invalid ELEMENT_TYPE: " + type);
        }
    }

    toString()
    {
        var result;
        if (this.m_type)
        {
            var type = this.m_type;
            var index = type.index();
            for (var j = 0; j < index; ++j)
            {
                type = type._Tail;
            }

            // For whatever reason, this approach has issues printing out ElementType values...
            if (index == 0)
            {
                result = __enumToString(type._Head, __ElementType);
            }
            else
            {
                result = type._Head.toString();
            }
        }
        else
        {
            result = this.Type.toString()
        }

        return result + (this.IsSzArray ? "[]" : "");
    }

    get IsSzArray()
    {
        return this.m_is_szarray || this.__isSzArray;
    }

    get Type()
    {
        // NOTE: m_type is std::variant<ElementType, coded_index<TypeDefOrRef>, GenericTypeIndex, GenericTypeInstSig>
        return this.m_type || this.__type;
    }

    get CustomMod()
    {
        // NOTE: m_cmod is std::vector
        return this.m_cmod || this.__customMods;
    }
}

class __ParamSig
{
    constructor(table, stream)
    {
        this.__customMods = __consumeCustomMods(table, stream);
        this.__byRef = __consumeIsByRef(stream);
        this.__type = new __TypeSig(table, stream);
    }

    toString()
    {
        var result = this.Type.toString();
        if (this.ByRef)
        {
            result += "&";
        }

        return result;
    }

    get ByRef()
    {
        return this.m_byref || this.__byRef;
    }

    get Type()
    {
        return this.m_type || this.__type;
    }

    get CustomMod()
    {
        return this.m_cmod || this.__customMods;
    }
}

class __RetTypeSig
{
    constructor(table, stream)
    {
        this.__customMods = __consumeCustomMods(table, stream);
        this.__byRef = __consumeIsByRef(stream);

        if (__peekElementType(stream) == __ElementType.Void)
        {
            stream.seek(1);
        }
        else
        {
            this.__type = new __TypeSig(table, stream);
        }
    }

    toString()
    {
        if (this.Type)
        {
            return this.Type.toString();
        }
        else
        {
            return "void";
        }
    }

    get ByRef()
    {
        return this.m_byref || this.__byRef;
    }

    get IsVoid()
    {
        return this.Type == null;
    }

    get Type()
    {
        // NOTE: m_type is std::optional
        return this.m_type || this.__type || null;
    }

    get CustomMod()
    {
        return this.m_cmod || this.__customMods;
    }
}

class __MethodDefSig
{
    constructor(table, stream)
    {
        this.__callingConvention = __consumeCallingConvention(stream);

        this.__genericParamCount = 0;
        if (this.__callingConvention & __CallingConventionFlags.Generic == __CallingConventionFlags.Generic)
        {
            this.__genericParamCount = stream.consumeUnsigned();
        }

        var paramCount = stream.consumeUnsigned();
        this.__retType = new __RetTypeSig(table, stream);

        this.__params = new Array(paramCount);
        for (var i = 0; i < paramCount; ++i)
        {
            this.__params[i] = new __ParamSig(table, stream);
        }
    }

    get __CallingConvention()
    {
        return this.m_calling_convention || this.__callingConvention;
    }

    toString()
    {
        var result = this.ReturnType.toString() + "(";
        var prefix = "";
        for (var param of this.Params)
        {
            result += prefix + param.toString();
            prefix = ", ";
        }
        return result + ")";
    }

    get CallingConvention()
    {
        return __callingConventionToString(this.__CallingConvention);
    }

    get GenericParamCount()
    {
        return this.m_generic_param_count || this.__genericParamCount;
    }

    get ReturnType()
    {
        return this.m_ret_type || this.__retType;
    }

    get Params()
    {
        // NOTE: m_params is std::vector
        return this.m_params || this.__params;
    }
}

class __FieldSig
{
    constructor(table, stream)
    {
        this.__callingConvention = __consumeCallingConvention(stream);
        if (this.__callingConvention & __CallingConvention.Mask != __CallingConvention.Field)
        {
            throw new Error("Invalid Field signature calling convention: " + __callingConventionToString(this.__callingConvention));
        }

        this.__customMods = __consumeCustomMods(table, stream);
        this.__type = new __TypeSig(table, stream);
    }

    get __CallingConvention()
    {
        return this.m_calling_convention || this.__callingConvention;
    }

    toString()
    {
        return this.Type.toString();
    }

    get Type()
    {
        return this.m_type || this.__type;
    }

    get CallingConvention()
    {
        return __callingConventionToString(this.__CallingConvention);
    }

    get CustomMod()
    {
        // NOTE: m_cmod is std::vector
        return this.m_cmod || this.__customMods;
    }
}

class __PropertySig
{
    constructor(table, stream)
    {
        this.__callingConvention = __consumeCallingConvention(stream);
        if (this.__callingConvention & __CallingConvention.Property != __CallingConvention.Property)
        {
            throw new Error("Invalid Property signature calling convention: " + __callingConventionToString(this.__callingConvention));
        }

        var paramCount = stream.consumeUnsigned();
        this.__customMods = __consumeCustomMods(table, stream);
        this.__type = new __TypeSig(table, stream);

        this.__params = new Array(paramCount);
        for (var i = 0; i < paramCount; ++i)
        {
            this.__params[i] = new __ParamSig(table, stream);
        }
    }

    get __CallingConvention()
    {
        return this.m_calling_convention || this.__callingConvention;
    }

    toString()
    {
        return this.Type.toString();
    }

    get Type()
    {
        return this.m_type || this.__type;
    }

    get CallingConvention()
    {
        return __callingConventionToString(this.__CallingConvention);
    }

    get Params()
    {
        return this.m_params || this.__params;
    }

    get CustomMod()
    {
        return this.m_cmod || this.__customMods;
    }
}

class __TypeSpecSig
{
    constructor(table, stream)
    {
        var type = __consumeElementType(stream);
        switch (type)
        {
        case __ElementType.Ptr:
            throw new Error("ELEMENT_TYPE_PTR is not currently supported");

        case __ElementType.FnPtr:
            throw new Error("ELEMENT_TYPE_FNPTR is not currently supported");

        case __ElementType.Array:
            throw new Error("ELEMENT_TYPE_ARRAY is not currently supported");

        case __ElementType.SZArray:
            throw new Error("ELEMENT_TYPE_SZARRAY is not currently supported");

        case __ElementType.GenericInst:
            this.__type = new __GenericTypeInstSig(table, stream);
            break;

        default:
            throw new Error("Unexpected or unknown ELEMENT_TYPE: " + type);
        }
    }

    toString()
    {
        return this.Type.toString();
    }

    get Type()
    {
        return this.m_type || this.__type;
    }
}

class __CustomAttributeSig
{
    constructor(table, stream, methodDefSig)
    {
        var prolog = stream.consumeValue(2);
        if (prolog != 0x0001)
        {
            throw new Error("CustomAttribute blobs must start with prolog of 0x0001");
        }

        this.__fixedArgs = new Array();
        for (var param of methodDefSig.Params)
        {
            this.__fixedArgs.push(new __FixedArgSig(table.Database, param, stream));
        }

        var namedArgsCount = stream.consumeValue(2);
        this.__namedArgs = new Array();
        for (var i = 0; i < namedArgsCount; ++i)
        {
            this.__namedArgs.push(new __NamedArgSig(table.Database, stream));
        }
    }

    get FixedArgs()
    {
        return this.m_fixed_args || this.__fixedArgs;
    }

    get NamedArgs()
    {
        return this.m_named_args || this.__namedArgs;
    }
}

class __FixedArgSig
{
    constructor(database, param, stream)
    {
        if (param.Type.IsSzArray)
        {
            this.__value = new Array();
            var size = stream.consumeValue(4);
            if (size != 0xFFFFFFFF)
            {
                for (var i = 0; i < size; ++i)
                {
                    this.__value.push(__ElemSig.__fromParam(param, stream));
                }
            }
        }
        else
        {
            this.__value = __ElemSig.__fromParam(param, stream);
        }
    }

    toString()
    {
        return this.Value.toString();
    }

    get Value()
    {
        return (this.value || this.__value).Value;
    }
}

class __NamedArgSig
{
    constructor(database, stream)
    {
        // NOTE: Appears unused by WinMD, so left unimplemented for now
    }
}

class __EnumDefinition
{
    constructor(typeDef)
    {
        this.__typeDef = typeDef;
        var fields = typeDef.FieldList;
        for (var i = 0; i < fields.Size; ++i)
        {
            var field = fields.getValueAt(i);
            var isLiteral = (field.Flags.Value & __FieldAttributesFlags.Literal) == __FieldAttributesFlags.Literal;
            var isStatic = (field.Flags.Value & __FieldAttributesFlags.Static) == __FieldAttributesFlags.Static;
            if (!isLiteral && !isStatic)
            {
                this.__underlyingType = field.Signature.Type.Type;
            }
        }
    }

    toString()
    {
        return this.TypeDef.toString();
    }

    get TypeDef()
    {
        return this.m_typedef || this.__typeDef;
    }

    get UnderlyingType()
    {
        return this.m_underlying_type || this.__underlyingType;
    }
}

class __SystemType
{
    constructor(name)
    {
        this.__name = name;
    }

    toString()
    {
        return this.Name.toString();
    }

    get Name()
    {
        return "System.Type(" + (this.name || this.__name) + ")";
    }
}

class __EnumValue
{
    constructor(type, value)
    {
        this.__type = type;
        this.__value = value;
    }

    toString()
    {
        // We ideally want the string value of the enum value
        var type = this.Type.TypeDef;
        var fields = type.FieldList;
        for (var i = 0; i < fields.Size; ++i)
        {
            var field = fields.getValueAt(i);
            if ((field.Flags.Value & __FieldAttributesFlags.Literal) == __FieldAttributesFlags.Literal &&
                (field.Flags.Value & __FieldAttributesFlags.Static) == __FieldAttributesFlags.Static)
            {
                if (field.Constant.Value == this.Value)
                {
                    return this.Type.toString() + "::" + field.Name;
                }
            }
        }

        return this.Type.toString() + "(" + this.Value.toString() + ")";
    }

    get Type()
    {
        return this.type || this.__type;
    }

    get Value()
    {
        return this.value || this.__value;
    }
}

class __ElemSig
{
    static __fromParam(param, stream)
    {
        var type = param.Type.Type;
        if (type.constructor.name == "__ElementTypeVisualizer")
        {
            return __ElemSig.__fromElementType(type.Value, stream);
        }

        if (type.constructor.name == "__TypeRefVisualizer" || type.constructor.name == "__TypeDefVisualizer")
        {
            if (type.TypeNamespace == "System" && type.TypeName == "Type")
            {
                return __ElemSig.__fromSystemType(stream.consumeString());
            }
        }

        // Should be an enum
        return __ElemSig.__fromEnumDefinition(new __EnumDefinition(type.FindDefinition()), stream);
    }

    static __fromSystemType(name)
    {
        return new __ElemSig(new __SystemType(name));
    }

    static __fromEnumDefinition(def, stream)
    {
        var value;
        switch (def.UnderlyingType.Value)
        {
        case __ElementType.Boolean:
            value = stream.consumeValue(1) == 0 ? false : true;
            break;

        case __ElementType.I1:
        case __ElementType.U1:
            value = stream.consumeValue(1);
            break;

        case __ElementType.Char:
        case __ElementType.I2:
        case __ElementType.U2:
            value = stream.consumeValue(2);
            break;

        case __ElementType.I4:
        case __ElementType.U4:
            value = stream.consumeValue(4);
            break;

        case __ElementType.I8:
        case __ElementType.U8:
            value = stream.consumeValue(8);
            break;

        default:
            throw new Error("Invalid underling enum type encountered: " + def.UnderlyingType);
        }

        return new __ElemSig(new __EnumValue(def, value), stream);
    }

    static __fromElementType(type, stream)
    {
        var value;
        switch (type)
        {
        case __ElementType.Boolean:
            value = stream.consumeValue(1) == 0 ? false : true;
            break;

        case __ElementType.I1:
        case __ElementType.U1:
            value = stream.consumeValue(1);
            break;

        case __ElementType.Char:
        case __ElementType.I2:
        case __ElementType.U2:
            value = stream.consumeValue(2);
            break;

        case __ElementType.I4:
        case __ElementType.U4:
            value = stream.consumeValue(4);
            break;

        case __ElementType.I8:
        case __ElementType.U8:
            value = stream.consumeValue(8);
            break;

        case __ElementType.R4:
        case __ElementType.R8:
            throw new Error("Floating point element types not yet supported");

        case __ElementType.String:
            value = stream.consumeString();
            break;

        default:
            throw new Error("Non-primitive type encountered: " + __enumToString(type, __ElementType));
        }

        return new __ElemSig(value);
    }

    constructor(value)
    {
        this.__value = value;
    }

    toString()
    {
        return this.Value.toString();
    }

    get Value()
    {
        return this.value || this.__value;
    }
}
