
namespace xlang::meta::reader
{
    template <> struct typed_index<CustomAttributeType> : index_base<CustomAttributeType>
    {
        using index_base<CustomAttributeType>::index_base;

        auto MemberRef() const;
        auto MethodDef() const;
    };

    template <> struct typed_index<HasConstant> : index_base<HasConstant>
    {
        using index_base<HasConstant>::index_base;

        auto Field() const;
        auto Param() const;
        auto Property() const;
    };

    template <> struct typed_index<HasSemantics> : index_base<HasSemantics>
    {
        using index_base<HasSemantics>::index_base;

        auto Property() const;
        auto Event() const;
    };

    template <> struct typed_index<MethodDefOrRef> : index_base<MethodDefOrRef>
    {
        using index_base<MethodDefOrRef>::index_base;
        
        auto MethodDef() const;
        auto MemberRef() const;
    };

    template <> struct typed_index<ResolutionScope> : index_base<ResolutionScope>
    {
        using index_base<ResolutionScope>::index_base;

        auto Module() const;
        auto ModuleRef() const;
        auto AssemblyRef() const;
        auto TypeRef() const;
    };

    template <> struct typed_index<TypeDefOrRef> : index_base<TypeDefOrRef>
    {
        using index_base<TypeDefOrRef>::index_base;

        TypeDef TypeDef() const;
        TypeRef TypeRef() const;
        TypeSpec TypeSpec() const;
        auto CustomAttribute() const;
    };
}
