
namespace xlang::meta::reader
{
    struct Module;
    struct TypeRef;
    struct TypeDef;
    struct Field;
    struct MethodDef;
    struct Param;
    struct InterfaceImpl;
    struct MemberRef;
    struct Constant;
    struct CustomAttribute;
    struct FieldMarshal;
    struct DeclSecurity;
    struct ClassLayout;
    struct FieldLayout;
    struct StandAloneSig;
    struct EventMap;
    struct Event;
    struct PropertyMap;
    struct Property;
    struct MethodSemantics;
    struct MethodImpl;
    struct ModuleRef;
    struct TypeSpec;
    struct ImplMap;
    struct FieldRVA;
    struct Assembly;
    struct AssemblyProcessor;
    struct AssemblyOS;
    struct AssemblyRef;
    struct AssemblyRefProcessor;
    struct AssemblyRefOS;
    struct File;
    struct ExportedType;
    struct ManifestResource;
    struct NestedClass;
    struct GenericParam;
    struct MethodSpec;
    struct GenericParamConstraint;


    template <typename Index, typename Row>
    struct index_tag;

    template <typename Index, typename Row>
    inline constexpr auto index_tag_v = index_tag<Index, Row>::value;

    template<>
    struct index_tag<TypeDefOrRef, TypeDef> : std::integral_constant<TypeDefOrRef, TypeDefOrRef::TypeDef> {};
    template<>
    struct index_tag<TypeDefOrRef, TypeRef> : std::integral_constant<TypeDefOrRef, TypeDefOrRef::TypeRef> {};
    template<>
    struct index_tag<TypeDefOrRef, TypeSpec> : std::integral_constant<TypeDefOrRef, TypeDefOrRef::TypeSpec> {};

    template<>
    struct index_tag<HasConstant, Field> : std::integral_constant<HasConstant, HasConstant::Field> {};
    template<>
    struct index_tag<HasConstant, Param> : std::integral_constant<HasConstant, HasConstant::Param> {};
    template<>
    struct index_tag<HasConstant, Property> : std::integral_constant<HasConstant, HasConstant::Property> {};

    template<>
    struct index_tag<HasCustomAttribute, MethodDef> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::MethodDef> {};
    template<>
    struct index_tag<HasCustomAttribute, Field> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::Field> {};
    template<>
    struct index_tag<HasCustomAttribute, TypeRef> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::TypeRef> {};
    template<>
    struct index_tag<HasCustomAttribute, TypeDef> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::TypeDef> {};
    template<>
    struct index_tag<HasCustomAttribute, Param> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::Param> {};
    template<>
    struct index_tag<HasCustomAttribute, InterfaceImpl> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::InterfaceImpl> {};
    template<>
    struct index_tag<HasCustomAttribute, MemberRef> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::MemberRef> {};
    template<>
    struct index_tag<HasCustomAttribute, Module> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::Module> {};
    template<>
    struct index_tag<HasCustomAttribute, Property> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::Property> {};
    template<>
    struct index_tag<HasCustomAttribute, Event> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::Event> {};
    template<>
    struct index_tag<HasCustomAttribute, StandAloneSig> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::StandAloneSig> {};
    template<>
    struct index_tag<HasCustomAttribute, ModuleRef> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::ModuleRef> {};
    template<>
    struct index_tag<HasCustomAttribute, TypeSpec> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::TypeSpec> {};
    template<>
    struct index_tag<HasCustomAttribute, Assembly> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::Assembly> {};
    template<>
    struct index_tag<HasCustomAttribute, AssemblyRef> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::AssemblyRef> {};
    template<>
    struct index_tag<HasCustomAttribute, File> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::File> {};
    template<>
    struct index_tag<HasCustomAttribute, ExportedType> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::ExportedType> {};
    template<>
    struct index_tag<HasCustomAttribute, ManifestResource> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::ManifestResource> {};
    template<>
    struct index_tag<HasCustomAttribute, GenericParam> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::GenericParam> {};
    template<>
    struct index_tag<HasCustomAttribute, GenericParamConstraint> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::GenericParamConstraint> {};
    template<>
    struct index_tag<HasCustomAttribute, MethodSpec> : std::integral_constant<HasCustomAttribute, HasCustomAttribute::MethodSpec> {};

    template<>
    struct index_tag<HasFieldMarshal, Field> : std::integral_constant<HasFieldMarshal, HasFieldMarshal::Field> {};
    template<>
    struct index_tag<HasFieldMarshal, Param> : std::integral_constant<HasFieldMarshal, HasFieldMarshal::Param> {};

    template<>
    struct index_tag<HasDeclSecurity, TypeDef> : std::integral_constant<HasDeclSecurity, HasDeclSecurity::TypeDef> {};
    template<>
    struct index_tag<HasDeclSecurity, MethodDef> : std::integral_constant<HasDeclSecurity, HasDeclSecurity::MethodDef> {};
    template<>
    struct index_tag<HasDeclSecurity, Assembly> : std::integral_constant<HasDeclSecurity, HasDeclSecurity::Assembly> {};

    template<>
    struct index_tag<MemberRefParent, TypeDef> : std::integral_constant<MemberRefParent, MemberRefParent::TypeDef> {};
    template<>
    struct index_tag<MemberRefParent, TypeRef> : std::integral_constant<MemberRefParent, MemberRefParent::TypeRef> {};
    template<>
    struct index_tag<MemberRefParent, ModuleRef> : std::integral_constant<MemberRefParent, MemberRefParent::ModuleRef> {};
    template<>
    struct index_tag<MemberRefParent, MethodDef> : std::integral_constant<MemberRefParent, MemberRefParent::MethodDef> {};
    template<>
    struct index_tag<MemberRefParent, TypeSpec> : std::integral_constant<MemberRefParent, MemberRefParent::TypeSpec> {};

    template<>
    struct index_tag<HasSemantics, Event> : std::integral_constant<HasSemantics, HasSemantics::Event> {};
    template<>
    struct index_tag<HasSemantics, Property> : std::integral_constant<HasSemantics, HasSemantics::Property> {};

    template<>
    struct index_tag<MethodDefOrRef, MethodDef> : std::integral_constant<MethodDefOrRef, MethodDefOrRef::MethodDef> {};
    template<>
    struct index_tag<MethodDefOrRef, MemberRef> : std::integral_constant<MethodDefOrRef, MethodDefOrRef::MemberRef> {};

    template<>
    struct index_tag<MemberForwarded, Field> : std::integral_constant<MemberForwarded, MemberForwarded::Field> {};
    template<>
    struct index_tag<MemberForwarded, MethodDef> : std::integral_constant<MemberForwarded, MemberForwarded::MethodDef> {};

    template<>
    struct index_tag<Implementation, File> : std::integral_constant<Implementation, Implementation::File> {};
    template<>
    struct index_tag<Implementation, AssemblyRef> : std::integral_constant<Implementation, Implementation::AssemblyRef> {};
    template<>
    struct index_tag<Implementation, ExportedType> : std::integral_constant<Implementation, Implementation::ExportedType> {};

    template<>
    struct index_tag<CustomAttributeType, MethodDef> : std::integral_constant<CustomAttributeType, CustomAttributeType::MethodDef> {};
    template<>
    struct index_tag<CustomAttributeType, MemberRef> : std::integral_constant<CustomAttributeType, CustomAttributeType::MemberRef> {};

    template<>
    struct index_tag<ResolutionScope, Module> : std::integral_constant<ResolutionScope, ResolutionScope::Module> {};
    template<>
    struct index_tag<ResolutionScope, ModuleRef> : std::integral_constant<ResolutionScope, ResolutionScope::ModuleRef> {};
    template<>
    struct index_tag<ResolutionScope, AssemblyRef> : std::integral_constant<ResolutionScope, ResolutionScope::AssemblyRef> {};
    template<>
    struct index_tag<ResolutionScope, TypeRef> : std::integral_constant<ResolutionScope, ResolutionScope::TypeRef> {};

    template<>
    struct index_tag<TypeOrMethodDef, TypeDef> : std::integral_constant<TypeOrMethodDef, TypeOrMethodDef::TypeDef> {};
    template<>
    struct index_tag<TypeOrMethodDef, MethodDef> : std::integral_constant<TypeOrMethodDef, TypeOrMethodDef::MethodDef> {};
}
