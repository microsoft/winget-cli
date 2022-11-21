namespace xlang::impl
{
    template <typename T>
    struct AttributesBase
    {
        T value;
    protected:
        constexpr T get_mask(T mask) const noexcept
        {
            return value & mask;
        }
        void set_mask(T arg, T mask) noexcept
        {
            value = (value & ~mask) | (arg & mask);
        }
        template <typename U>
        constexpr U get_enum(T mask) const noexcept
        {
            static_assert(std::is_enum_v<U>);
            static_assert(std::is_same_v<T, std::underlying_type_t<U>>);
            return static_cast<U>(get_mask(mask));
        }
        template <typename U>
        void set_enum(U arg, T mask) noexcept
        {
            static_assert(std::is_enum_v<U>);
            static_assert(std::is_same_v<T, std::underlying_type_t<U>>);
            set_mask(static_cast<T>(arg), mask);
        }
        constexpr bool get_bit(int bit) const noexcept
        {
            return get_mask(1 << bit);
        }
        void set_bit(bool arg, int bit) noexcept
        {
            set_mask(arg << bit, 1 << bit);
        }
    };
}

namespace xlang::meta::reader
{
    struct AssemblyAttributes : impl::AttributesBase<uint32_t>
    {
        constexpr bool WindowsRuntime() const noexcept
        {
            return get_bit(WindowsRuntime_bit);
        }
        void WindowsRuntime(bool arg) noexcept
        {
            set_bit(arg, WindowsRuntime_bit);
        }

    private:
        static constexpr int WindowsRuntime_bit{ 9 };
    };

    struct EventAttributes : impl::AttributesBase<uint16_t>
    {
        constexpr bool SpecialName() const noexcept
        {
            return get_bit(SpecialName_bit);
        }
        void SpecialName(bool arg) noexcept
        {
            set_bit(arg, SpecialName_bit);
        }
        constexpr bool RTSpecialName() const noexcept
        {
            return get_bit(RTSpecialName_bit);
        }
        void RTSpecialName(bool arg) noexcept
        {
            set_bit(arg, RTSpecialName_bit);
        }

    private:
        static constexpr int SpecialName_bit{ 9 };
        static constexpr int RTSpecialName_bit{ 10 };
    };

    struct FieldAttributes : impl::AttributesBase<uint16_t>
    {
        constexpr MemberAccess Access() const noexcept
        {
            return get_enum<MemberAccess>(Access_mask);
        }
        void Access(MemberAccess arg) noexcept
        {
            set_enum(arg, Access_mask);
        }
        constexpr bool Static() const noexcept
        {
            return get_bit(Static_bit);
        }
        constexpr bool InitOnly() const noexcept
        {
            return get_bit(InitOnly_bit);
        }
        constexpr bool Literal() const noexcept
        {
            return get_bit(Literal_bit);
        }
        constexpr bool NotSerialized() const noexcept
        {
            return get_bit(NotSerialized_bit);
        }
        constexpr bool SpecialName() const noexcept
        {
            return get_bit(SpecialName_bit);
        }
        constexpr bool PInvokeImpl() const noexcept
        {
            return get_bit(PInvokeImpl_bit);
        }
        constexpr bool RTSpecialName() const noexcept
        {
            return get_bit(RTSpecialName_bit);
        }
        constexpr bool HasFieldMarshal() const noexcept
        {
            return get_bit(HasFieldMarshal_bit);
        }
        constexpr bool HasDefault() const noexcept
        {
            return get_bit(HasDefault_bit);
        }
        constexpr bool HasFieldRVA() const noexcept
        {
            return get_bit(HasFieldRVA_bit);
        }

    private:
        static constexpr uint16_t Access_mask{ 0x0007 };
        static constexpr int Static_bit{ 4 };
        static constexpr int InitOnly_bit{ 5 };
        static constexpr int Literal_bit{ 6 };
        static constexpr int NotSerialized_bit{ 7 };
        static constexpr int SpecialName_bit{ 9 };
        static constexpr int PInvokeImpl_bit{ 13 };
        static constexpr int RTSpecialName_bit{ 10 };
        static constexpr int HasFieldMarshal_bit{ 12 };
        static constexpr int HasDefault_bit{ 15 };
        static constexpr int HasFieldRVA_bit{ 8 };
    };

    struct GenericParamAttributes : impl::AttributesBase<uint16_t>
    {
        constexpr GenericParamVariance Variance() const noexcept
        {
            return get_enum<GenericParamVariance>(Variance_mask);
        }
        void Variance(GenericParamVariance arg) noexcept
        {
            set_enum(arg, Variance_mask);
        }
        constexpr GenericParamSpecialConstraint SpecialConstraint() const noexcept
        {
            return get_enum<GenericParamSpecialConstraint>(SpecialConstraint_mask);
        }
        void SpecialConstraint(GenericParamSpecialConstraint arg) noexcept
        {
            set_enum(arg, SpecialConstraint_mask);
        }

    private:
        static constexpr uint16_t Variance_mask{ 0x0003 };
        static constexpr uint16_t SpecialConstraint_mask{ 0x001c };
    };

    struct MethodAttributes : impl::AttributesBase<uint16_t>
    {
        constexpr MemberAccess Access() const noexcept
        {
            return get_enum<MemberAccess>(Access_mask);
        }
        void Access(MemberAccess arg) noexcept
        {
            set_enum(arg, Access_mask);
        }
        constexpr bool Static() const noexcept
        {
            return get_bit(Static_bit);
        }
        void Static(bool arg) noexcept
        {
            set_bit(arg, Static_bit);
        }
        constexpr bool Final() const noexcept
        {
            return get_bit(Final_bit);
        }
        void Final(bool arg) noexcept
        {
            set_bit(arg, Final_bit);
        }
        constexpr bool Virtual() const noexcept
        {
            return get_bit(Virtual_bit);
        }
        void Virtual(bool arg) noexcept
        {
            set_bit(arg, Virtual_bit);
        }
        constexpr bool HideBySig() const noexcept
        {
            return get_bit(HideBySig_bit);
        }
        void HideBySig(bool arg) noexcept
        {
            set_bit(arg, HideBySig_bit);
        }
        constexpr VtableLayout Layout() const noexcept
        {
            return get_enum<VtableLayout>(VtableLayout_mask);
        }
        void Layout(VtableLayout arg) noexcept
        {
            set_enum(arg, VtableLayout_mask);
        }
        constexpr bool Strict() const noexcept
        {
            return get_bit(Strict_bit);
        }
        void Strict(bool arg) noexcept
        {
            set_bit(arg, Strict_bit);
        }
        constexpr bool Abstract() const noexcept
        {
            return get_bit(Abstract_bit);
        }
        void Abstract(bool arg) noexcept
        {
            set_bit(arg, Abstract_bit);
        }
        constexpr bool SpecialName() const noexcept
        {
            return get_bit(SpecialName_bit);
        }
        void SpecialName(bool arg) noexcept
        {
            set_bit(arg, SpecialName_bit);
        }
        constexpr bool PInvokeImpl() const noexcept
        {
            return get_bit(PInvokeImpl_bit);
        }
        void PInvokeImpl(bool arg) noexcept
        {
            set_bit(arg, PInvokeImpl_bit);
        }
        constexpr bool UnmanagedExport() const noexcept
        {
            return get_bit(UnmanagedExport_bit);
        }
        void UnmanagedExport(bool arg) noexcept
        {
            set_bit(arg, UnmanagedExport_bit);
        }
        constexpr bool RTSpecialName() const noexcept
        {
            return get_bit(RTSpecialName_bit);
        }
        void RTSpecialName(bool arg) noexcept
        {
            set_bit(arg, RTSpecialName_bit);
        }
        constexpr bool HasSecurity() const noexcept
        {
            return get_bit(HasSecurity_bit);
        }
        void HasSecurity(bool arg) noexcept
        {
            set_bit(arg, HasSecurity_bit);
        }
        constexpr bool RequireSecObject() const noexcept
        {
            return get_bit(RequireSecObject_bit);
        }
        void RequireSecObject(bool arg) noexcept
        {
            set_bit(arg, RequireSecObject_bit);
        }

    private:
        static constexpr uint16_t Access_mask{ 0x0007 };
        static constexpr int Static_bit{ 4 };
        static constexpr int Final_bit{ 5 };
        static constexpr int Virtual_bit{ 6 };
        static constexpr int HideBySig_bit{ 7 };
        static constexpr uint16_t VtableLayout_mask{ 0x0100 };
        static constexpr int Strict_bit{ 9 };
        static constexpr int Abstract_bit{ 10 };
        static constexpr int SpecialName_bit{ 11 };
        static constexpr int PInvokeImpl_bit{ 13 };
        static constexpr int UnmanagedExport_bit{ 3 };
        static constexpr int RTSpecialName_bit{ 12 };
        static constexpr int HasSecurity_bit{ 14 };
        static constexpr int RequireSecObject_bit{ 15 };
    };

    struct MethodImplAttributes : impl::AttributesBase<uint16_t>
    {
        constexpr CodeType CodeType() const noexcept
        {
            return get_enum<reader::CodeType>(CodeType_mask);
        }
        void CodeType(reader::CodeType arg) noexcept
        {
            set_enum(arg, CodeType_mask);
        }
        constexpr Managed Managed() const noexcept
        {
            return get_enum<reader::Managed>(Managed_mask);
        }
        void Managed(reader::Managed arg) noexcept
        {
            set_enum(arg, Managed_mask);
        }
        constexpr bool ForwardRef() const noexcept
        {
            return get_bit(ForwardRef_bit);
        }
        void ForwardRef(bool arg) noexcept
        {
            set_bit(arg, ForwardRef_bit);
        }
        constexpr bool PreserveSig() const noexcept
        {
            return get_bit(PreserveSig_bit);
        }
        void PreserveSig(bool arg) noexcept
        {
            set_bit(arg, PreserveSig_bit);
        }
        constexpr bool InternalCall() const noexcept
        {
            return get_bit(InternalCall_bit);
        }
        void InternalCall(bool arg) noexcept
        {
            set_bit(arg, InternalCall_bit);
        }
        constexpr bool Synchronized() const noexcept
        {
            return get_bit(Synchronized_bit);
        }
        void Synchronized(bool arg) noexcept
        {
            set_bit(arg, Synchronized_bit);
        }
        constexpr bool NoInlining() const noexcept
        {
            return get_bit(NoInlining_bit);
        }
        void NoInlining(bool arg) noexcept
        {
            set_bit(arg, NoInlining_bit);
        }
        constexpr bool NoOptimization() const noexcept
        {
            return get_bit(NoOptimization_bit);
        }
        void NoOptimization(bool arg) noexcept
        {
            set_bit(arg, NoOptimization_bit);
        }

    private:
        static constexpr uint16_t CodeType_mask{ 0x0003 };
        static constexpr uint16_t Managed_mask{ 0x0004 };
        static constexpr int ForwardRef_bit{ 4 }; // Method is defined; used primarily in merge scenarios
        static constexpr int PreserveSig_bit{ 7 }; // Reserved
        static constexpr int InternalCall_bit{ 12 }; // Reserved
        static constexpr int Synchronized_bit{ 5 }; // Method is single threaded through the body
        static constexpr int NoInlining_bit{ 3 }; // Method cannot be inlined
        static constexpr int NoOptimization_bit{ 6 }; // Method will not be optimized when generatinv native code
        static constexpr uint16_t MaxMethodImplVal{ 0xffff }; // Range check value
    };

    struct MethodSemanticsAttributes : impl::AttributesBase<uint16_t>
    {
        constexpr bool Setter() const noexcept
        {
            return get_bit(Setter_bit);
        }
        void Setter(bool arg) noexcept
        {
            set_bit(arg, Setter_bit);
        }
        constexpr bool Getter() const noexcept
        {
            return get_bit(Getter_bit);
        }
        void Getter(bool arg) noexcept
        {
            set_bit(arg, Getter_bit);
        }
        constexpr bool Other() const noexcept
        {
            return get_bit(Other_bit);
        }
        void Other(bool arg) noexcept
        {
            set_bit(arg, Other_bit);
        }
        constexpr bool AddOn() const noexcept
        {
            return get_bit(AddOn_bit);
        }
        void AddOn(bool arg) noexcept
        {
            set_bit(arg, AddOn_bit);
        }
        constexpr bool RemoveOn() const noexcept
        {
            return get_bit(RemoveOn_bit);
        }
        void RemoveOn(bool arg) noexcept
        {
            set_bit(arg, RemoveOn_bit);
        }
        constexpr bool Fire() const noexcept
        {
            return get_bit(Fire_bit);
        }
        void Fire(bool arg) noexcept
        {
            set_bit(arg, Fire_bit);
        }

    private:
        static constexpr int Setter_bit{ 0 };
        static constexpr int Getter_bit{ 1 };
        static constexpr int Other_bit{ 2 };
        static constexpr int AddOn_bit{ 3 };
        static constexpr int RemoveOn_bit{ 4 };
        static constexpr int Fire_bit{ 5 };
    };

    struct ParamAttributes : impl::AttributesBase<uint16_t>
    {
        constexpr bool In() const noexcept
        {
            return get_bit(In_bit);
        }
        void In(bool arg) noexcept
        {
            set_bit(arg, In_bit);
        }
        constexpr bool Out() const noexcept
        {
            return get_bit(Out_bit);
        }
        void Out(bool arg) noexcept
        {
            set_bit(arg, Out_bit);
        }
        constexpr bool Optional() const noexcept
        {
            return get_bit(Optional_bit);
        }
        void Optional(bool arg) noexcept
        {
            set_bit(arg, Optional_bit);
        }
        constexpr bool HasDefault() const noexcept
        {
            return get_bit(HasDefault_bit);
        }
        void HasDefault(bool arg) noexcept
        {
            set_bit(arg, HasDefault_bit);
        }
        constexpr bool HasFieldMarshal() const noexcept
        {
            return get_bit(HasFieldMarshal_bit);
        }
        void HasFieldMarshal(bool arg) noexcept
        {
            set_bit(arg, HasFieldMarshal_bit);
        }

    private:
        static constexpr int In_bit{ 0 };
        static constexpr int Out_bit{ 1 };
        static constexpr int Optional_bit{ 4 };
        static constexpr int HasDefault_bit{ 12 };
        static constexpr int HasFieldMarshal_bit{ 13 };
        static constexpr uint16_t Unused_mask{ 0xcfe0 };
    };

    struct PropertyAttributes : impl::AttributesBase<uint16_t>
    {
        constexpr bool SpecialName() const noexcept
        {
            return get_bit(SpecialName_bit);
        }
        void SpecialName(bool arg) noexcept
        {
            set_bit(arg, SpecialName_bit);
        }
        constexpr bool RTSpecialName() const noexcept
        {
            return get_bit(RTSpecialName_bit);
        }
        void RTSpecialName(bool arg) noexcept
        {
            set_bit(arg, RTSpecialName_bit);
        }
        constexpr bool HasDefault() const noexcept
        {
            return get_bit(HasDefault_bit);
        }
        void HasDefault(bool arg) noexcept
        {
            set_bit(arg, HasDefault_bit);
        }

    private:
        static constexpr int SpecialName_bit{ 9 };
        static constexpr int RTSpecialName_bit{ 10 };
        static constexpr int HasDefault_bit{ 12 };
        static constexpr uint16_t Unused_mask{ 0xe9ff };
    };

    struct TypeAttributes : impl::AttributesBase<uint32_t>
    {
        constexpr TypeVisibility Visibility() const noexcept
        {
            return get_enum<TypeVisibility>(Visibility_mask);
        }
        void Visibility(TypeVisibility arg) noexcept
        {
            set_enum(arg, Visibility_mask);
        }
        constexpr TypeLayout Layout() const noexcept
        {
            return get_enum<TypeLayout>(Layout_mask);
        }
        void Layout(TypeLayout arg) noexcept
        {
            set_enum(arg, Layout_mask);
        }
        constexpr TypeSemantics Semantics() const noexcept
        {
            return get_enum<TypeSemantics>(Semantics_mask);
        }
        void Semantics(TypeSemantics arg) noexcept
        {
            set_enum(arg, Semantics_mask);
        }
        constexpr bool Abstract() const noexcept
        {
            return get_bit(Abstract_bit);
        }
        void Abstract(bool arg) noexcept
        {
            set_bit(arg, Abstract_bit);
        }
        constexpr bool Sealed() const noexcept
        {
            return get_bit(Sealed_bit);
        }
        void Sealed(bool arg) noexcept
        {
            set_bit(arg, Sealed_bit);
        }
        constexpr bool SpecialName() const noexcept
        {
            return get_bit(SpecialName_bit);
        }
        void SpecialName(bool arg) noexcept
        {
            set_bit(arg, SpecialName_bit);
        }
        constexpr bool Import() const noexcept
        {
            return get_bit(Import_bit);
        }
        void Import(bool arg) noexcept
        {
            set_bit(arg, Import_bit);
        }
        constexpr bool Serializable() const noexcept
        {
            return get_bit(Serializable_bit);
        }
        void Serializable(bool arg) noexcept
        {
            set_bit(arg, Serializable_bit);
        }
        constexpr bool WindowsRuntime() const noexcept
        {
            return get_bit(WindowsRuntime_bit);
        }
        void WindowsRuntime(bool arg) noexcept
        {
            set_bit(arg, WindowsRuntime_bit);
        }
        constexpr StringFormat StringFormat() const noexcept
        {
            return get_enum<reader::StringFormat>(StringFormat_mask);
        }
        void StringFormat(reader::StringFormat arg) noexcept
        {
            set_enum(arg, StringFormat_mask);
        }
        constexpr bool BeforeFieldInit() const noexcept
        {
            return get_bit(BeforeFieldInit_bit);
        }
        void BeforeFieldInit(bool arg) noexcept
        {
            set_bit(arg, BeforeFieldInit_bit);
        }
        constexpr bool RTSpecialName() const noexcept
        {
            return get_bit(RTSpecialName_bit);
        }
        void RTSpecialName(bool arg) noexcept
        {
            set_bit(arg, RTSpecialName_bit);
        }
        constexpr bool HasSecurity() const noexcept
        {
            return get_bit(HasSecurity_bit);
        }
        void HasSecurity(bool arg) noexcept
        {
            set_bit(arg, HasSecurity_bit);
        }
        constexpr bool IsTypeForwarder() const noexcept
        {
            return get_bit(IsTypeForwarder_bit);
        }
        void IsTypeForwarder(bool arg) noexcept
        {
            set_bit(arg, IsTypeForwarder_bit);
        }

    private:
        static constexpr uint32_t Visibility_mask{ 0x00000007 };
        static constexpr uint32_t Layout_mask{ 0x00000018 };
        static constexpr uint32_t Semantics_mask{ 0x00000020 };
        static constexpr int Abstract_bit{ 7 };
        static constexpr int Sealed_bit{ 8 };
        static constexpr int SpecialName_bit{ 10 };
        static constexpr int Import_bit{ 12 };
        static constexpr int Serializable_bit{ 13 };
        static constexpr int WindowsRuntime_bit{ 14 };
        static constexpr uint32_t StringFormat_mask{ 0x00030000 };
        static constexpr int BeforeFieldInit_bit{ 20 };
        static constexpr int RTSpecialName_bit{ 11 };
        static constexpr int HasSecurity_bit{ 18 };
        static constexpr int IsTypeForwarder_bit{ 21 };
    };
}
