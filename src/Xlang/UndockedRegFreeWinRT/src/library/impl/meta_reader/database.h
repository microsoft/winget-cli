
namespace xlang::impl
{
    constexpr uint8_t bits_needed(uint32_t value) noexcept
    {
        --value;
        uint8_t bits{ 1 };

        while (value >>= 1)
        {
            ++bits;
        }

        return bits;
    }

    static_assert(bits_needed(2) == 1);
    static_assert(bits_needed(3) == 2);
    static_assert(bits_needed(4) == 2);
    static_assert(bits_needed(5) == 3);
    static_assert(bits_needed(22) == 5);
}

namespace xlang::meta::reader
{
    struct cache;

    struct database
    {
        database(database&&) = delete;
        database& operator=(database&&) = delete;

        static bool is_database(std::string_view const& path)
        {
            file_view file{ path };

            if (file.size() < sizeof(impl::image_dos_header))
            {
                return false;
            }

            auto dos = file.as<impl::image_dos_header>();

            if (dos.e_signature != 0x5A4D) // IMAGE_DOS_SIGNATURE
            {
                return false;
            }

            if (file.size() < (dos.e_lfanew + sizeof(impl::image_nt_headers32)))
            {
                return false;
            }

            auto pe = file.as<impl::image_nt_headers32>(dos.e_lfanew);

            if (pe.FileHeader.NumberOfSections == 0 || pe.FileHeader.NumberOfSections > 100)
            {
                return false;
            }
            
            impl::image_section_header const* sections{};
            uint32_t com_virtual_address{};
            if (pe.OptionalHeader.Magic == 0x10B) // PE32
            {
                com_virtual_address = pe.OptionalHeader.DataDirectory[14].VirtualAddress; // IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR
                sections = &file.as<impl::image_section_header>(dos.e_lfanew + sizeof(impl::image_nt_headers32));
            }
            else if (pe.OptionalHeader.Magic == 0x20B) // PE32+
            {
                auto pe_plus = file.as<impl::image_nt_headers32plus>(dos.e_lfanew);
                com_virtual_address = pe_plus.OptionalHeader.DataDirectory[14].VirtualAddress; // IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR
                sections = &file.as<impl::image_section_header>(dos.e_lfanew + sizeof(impl::image_nt_headers32plus));
            }
            else
            {
                throw_invalid("Invalid optional header magic value");
            }
            auto sections_end = sections + pe.FileHeader.NumberOfSections;
            auto section = section_from_rva(sections, sections_end, com_virtual_address);

            if (section == sections_end)
            {
                return false;
            }

            auto offset = offset_from_rva(*section, com_virtual_address);

            auto cli = file.as<impl::image_cor20_header>(offset);

            if (cli.cb != sizeof(impl::image_cor20_header))
            {
                return false;
            }

            section = section_from_rva(sections, sections_end, cli.MetaData.VirtualAddress);

            if (section == sections_end)
            {
                return false;
            }

            offset = offset_from_rva(*section, cli.MetaData.VirtualAddress);

            if (file.as<uint32_t>(offset) != 0x424a5342)
            {
                return false;
            }

            return true;
        }

        explicit database(std::vector<uint8_t>&& buffer, cache const* cache = nullptr) : m_buffer{ std::move(buffer) }, m_view{ m_buffer.data(), m_buffer.data() + m_buffer.size() }, m_cache{ cache }
        {
            initialize();
        }

        explicit database(std::string_view const& path, cache const* cache = nullptr) : m_view{ path }, m_path{ path }, m_cache{ cache }
        {
            initialize();
        }

        table<TypeRef> TypeRef{ this };
        table<GenericParamConstraint> GenericParamConstraint{ this };
        table<TypeSpec> TypeSpec{ this };
        table<TypeDef> TypeDef{ this };
        table<CustomAttribute> CustomAttribute{ this };
        table<MethodDef> MethodDef{ this };
        table<MemberRef> MemberRef{ this };
        table<Module> Module{ this };
        table<Param> Param{ this };
        table<InterfaceImpl> InterfaceImpl{ this };
        table<Constant> Constant{ this };
        table<Field> Field{ this };
        table<FieldMarshal> FieldMarshal{ this };
        table<DeclSecurity> DeclSecurity{ this };
        table<ClassLayout> ClassLayout{ this };
        table<FieldLayout> FieldLayout{ this };
        table<StandAloneSig> StandAloneSig{ this };
        table<EventMap> EventMap{ this };
        table<Event> Event{ this };
        table<PropertyMap> PropertyMap{ this };
        table<Property> Property{ this };
        table<MethodSemantics> MethodSemantics{ this };
        table<MethodImpl> MethodImpl{ this };
        table<ModuleRef> ModuleRef{ this };
        table<ImplMap> ImplMap{ this };
        table<FieldRVA> FieldRVA{ this };
        table<Assembly> Assembly{ this };
        table<AssemblyProcessor> AssemblyProcessor{ this };
        table<AssemblyOS> AssemblyOS{ this };
        table<AssemblyRef> AssemblyRef{ this };
        table<AssemblyRefProcessor> AssemblyRefProcessor{ this };
        table<AssemblyRefOS> AssemblyRefOS{ this };
        table<File> File{ this };
        table<ExportedType> ExportedType{ this };
        table<ManifestResource> ManifestResource{ this };
        table<NestedClass> NestedClass{ this };
        table<GenericParam> GenericParam{ this };
        table<MethodSpec> MethodSpec{ this };

        template <typename T>
        table<T> const& get_table() const noexcept;

        cache const& get_cache() const noexcept
        {
            return *m_cache;
        }

        std::string const& path() const noexcept
        {
            return m_path;
        }

        std::string_view get_string(uint32_t const index) const
        {
            auto view = m_strings.seek(index);
            auto last = std::find(view.begin(), view.end(), 0);

            if (last == view.end())
            {
                throw_invalid("Missing string terminator");
            }

            return { reinterpret_cast<char const*>(view.begin()), static_cast<uint32_t>(last - view.begin()) };
        }

        byte_view get_blob(uint32_t const index) const
        {
            auto view = m_blobs.seek(index);
            auto initial_byte = view.as<uint8_t>();
            uint32_t blob_size_bytes{};

            switch (initial_byte >> 5)
            {
            case 0:
            case 1:
            case 2:
            case 3:
                blob_size_bytes = 1;
                initial_byte &= 0x7f;
                break;

            case 4:
            case 5:
                blob_size_bytes = 2;
                initial_byte &= 0x3f;
                break;

            case 6:
                blob_size_bytes = 4;
                initial_byte &= 0x1f;
                break;

            default:
                throw_invalid("Invalid blob encoding");
            }

            uint32_t blob_size{ initial_byte };

            for (auto&& byte : view.sub(1, blob_size_bytes - 1))
            {
                blob_size = (blob_size << 8) + byte;
            }

            return { view.sub(blob_size_bytes, blob_size) };
        }

    private:
        void initialize()
        {
            auto dos = m_view.as<impl::image_dos_header>();

            if (dos.e_signature != 0x5A4D) // IMAGE_DOS_SIGNATURE
            {
                throw_invalid("Invalid DOS signature");
            }

            auto pe = m_view.as<impl::image_nt_headers32>(dos.e_lfanew);

            if (pe.FileHeader.NumberOfSections == 0 || pe.FileHeader.NumberOfSections > 100)
            {
                throw_invalid("Invalid PE section count");
            }

            impl::image_section_header const* sections{};
            uint32_t com_virtual_address{};
            if (pe.OptionalHeader.Magic == 0x10B) // PE32
            {
                com_virtual_address = pe.OptionalHeader.DataDirectory[14].VirtualAddress; // IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR
                sections = &m_view.as<impl::image_section_header>(dos.e_lfanew + sizeof(impl::image_nt_headers32));
            }
            else if (pe.OptionalHeader.Magic == 0x20B) // PE32+
            {
                auto pe_plus = m_view.as<impl::image_nt_headers32plus>(dos.e_lfanew);
                com_virtual_address = pe_plus.OptionalHeader.DataDirectory[14].VirtualAddress; // IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR
                sections = &m_view.as<impl::image_section_header>(dos.e_lfanew + sizeof(impl::image_nt_headers32plus));
            }
            else
            {
                throw_invalid("Invalid optional header magic value");
            }
            auto sections_end = sections + pe.FileHeader.NumberOfSections;
            auto section = section_from_rva(sections, sections_end, com_virtual_address);

            if (section == sections_end)
            {
                throw_invalid("PE section containing CLI header not found");
            }

            auto offset = offset_from_rva(*section, com_virtual_address);

            auto cli = m_view.as<impl::image_cor20_header>(offset);

            if (cli.cb != sizeof(impl::image_cor20_header))
            {
                throw_invalid("Invalid CLI header");
            }

            section = section_from_rva(sections, sections_end, cli.MetaData.VirtualAddress);

            if (section == sections_end)
            {
                throw_invalid("PE section containing CLI metadata not found");
            }

            offset = offset_from_rva(*section, cli.MetaData.VirtualAddress);

            if (m_view.as<uint32_t>(offset) != 0x424a5342)
            {
                throw_invalid("CLI metadata magic signature not found");
            }

            auto version_length = m_view.as<uint32_t>(offset + 12);
            auto stream_count = m_view.as<uint16_t>(offset + version_length + 18);
            auto view = m_view.seek(offset + version_length + 20);
            byte_view tables;

            for (uint16_t i{}; i < stream_count; ++i)
            {
                auto stream = view.as<stream_range>();
                auto name = view.as<std::array<char, 12>>(8);

                if (name.data() == "#Strings"sv)
                {
                    m_strings = m_view.sub(offset + stream.offset, stream.size);
                }
                else if (name.data() == "#Blob"sv)
                {
                    m_blobs = m_view.sub(offset + stream.offset, stream.size);
                }
                else if (name.data() == "#GUID"sv)
                {
                    m_guids = m_view.sub(offset + stream.offset, stream.size);
                }
                else if (name.data() == "#~"sv)
                {
                    tables = m_view.sub(offset + stream.offset, stream.size);
                }
                else if (name.data() != "#US"sv)
                {
                    throw_invalid("Unknown metadata stream");
                }

                view = view.seek(stream_offset(name.data()));
            }

            std::bitset<8> const heap_sizes{ tables.as<uint8_t>(6) };
            uint8_t const string_index_size = heap_sizes.test(0) ? 4 : 2;
            uint8_t const guid_index_size = heap_sizes.test(1) ? 4 : 2;
            uint8_t const blob_index_size = heap_sizes.test(2) ? 4 : 2;

            std::bitset<64> const valid_bits{ tables.as<uint64_t>(8) };
            view = tables.seek(24);

            for (uint32_t i{}; i < 64; ++i)
            {
                if (!valid_bits.test(i))
                {
                    continue;
                }

                auto row_count = view.as<uint32_t>();
                view = view.seek(4);

                switch (i)
                {
                case 0x00: Module.set_row_count(row_count); break;
                case 0x01: TypeRef.set_row_count(row_count); break;
                case 0x02: TypeDef.set_row_count(row_count); break;
                case 0x04: Field.set_row_count(row_count); break;
                case 0x06: MethodDef.set_row_count(row_count); break;
                case 0x08: Param.set_row_count(row_count); break;
                case 0x09: InterfaceImpl.set_row_count(row_count); break;
                case 0x0a: MemberRef.set_row_count(row_count); break;
                case 0x0b: Constant.set_row_count(row_count); break;
                case 0x0c: CustomAttribute.set_row_count(row_count); break;
                case 0x0d: FieldMarshal.set_row_count(row_count); break;
                case 0x0e: DeclSecurity.set_row_count(row_count); break;
                case 0x0f: ClassLayout.set_row_count(row_count); break;
                case 0x10: FieldLayout.set_row_count(row_count); break;
                case 0x11: StandAloneSig.set_row_count(row_count); break;
                case 0x12: EventMap.set_row_count(row_count); break;
                case 0x14: Event.set_row_count(row_count); break;
                case 0x15: PropertyMap.set_row_count(row_count); break;
                case 0x17: Property.set_row_count(row_count); break;
                case 0x18: MethodSemantics.set_row_count(row_count); break;
                case 0x19: MethodImpl.set_row_count(row_count); break;
                case 0x1a: ModuleRef.set_row_count(row_count); break;
                case 0x1b: TypeSpec.set_row_count(row_count); break;
                case 0x1c: ImplMap.set_row_count(row_count); break;
                case 0x1d: FieldRVA.set_row_count(row_count); break;
                case 0x20: Assembly.set_row_count(row_count); break;
                case 0x21: AssemblyProcessor.set_row_count(row_count); break;
                case 0x22: AssemblyOS.set_row_count(row_count); break;
                case 0x23: AssemblyRef.set_row_count(row_count); break;
                case 0x24: AssemblyRefProcessor.set_row_count(row_count); break;
                case 0x25: AssemblyRefOS.set_row_count(row_count); break;
                case 0x26: File.set_row_count(row_count); break;
                case 0x27: ExportedType.set_row_count(row_count); break;
                case 0x28: ManifestResource.set_row_count(row_count); break;
                case 0x29: NestedClass.set_row_count(row_count); break;
                case 0x2a: GenericParam.set_row_count(row_count); break;
                case 0x2b: MethodSpec.set_row_count(row_count); break;
                case 0x2c: GenericParamConstraint.set_row_count(row_count); break;
                default: throw_invalid("Unknown metadata table");
                };
            }

            table_base const empty_table{ nullptr };

            auto const TypeDefOrRef = composite_index_size(TypeDef, TypeRef, TypeSpec);
            auto const HasConstant = composite_index_size(Field, Param, Property);
            auto const HasCustomAttribute = composite_index_size(MethodDef, Field, TypeRef, TypeDef, Param, InterfaceImpl, MemberRef, Module, Property, Event, StandAloneSig, ModuleRef, TypeSpec, Assembly, AssemblyRef, File, ExportedType, ManifestResource, GenericParam, GenericParamConstraint, MethodSpec);
            auto const HasFieldMarshal = composite_index_size(Field, Param);
            auto const HasDeclSecurity = composite_index_size(TypeDef, MethodDef, Assembly);
            auto const MemberRefParent = composite_index_size(TypeDef, TypeRef, ModuleRef, MethodDef, TypeSpec);
            auto const HasSemantics = composite_index_size(Event, Property);
            auto const MethodDefOrRef = composite_index_size(MethodDef, MemberRef);
            auto const MemberForwarded = composite_index_size(Field, MethodDef);
            auto const Implementation = composite_index_size(File, AssemblyRef, ExportedType);
            auto const CustomAttributeType = composite_index_size(MethodDef, MemberRef, empty_table, empty_table, empty_table);
            auto const ResolutionScope = composite_index_size(Module, ModuleRef, AssemblyRef, TypeRef);
            auto const TypeOrMethodDef = composite_index_size(TypeDef, MethodDef);

            Assembly.set_columns(4, 8, 4, blob_index_size, string_index_size, string_index_size);
            AssemblyOS.set_columns(4, 4, 4);
            AssemblyProcessor.set_columns(4);
            AssemblyRef.set_columns(8, 4, blob_index_size, string_index_size, string_index_size, blob_index_size);
            AssemblyRefOS.set_columns(4, 4, 4, AssemblyRef.index_size());
            AssemblyRefProcessor.set_columns(4, AssemblyRef.index_size());
            ClassLayout.set_columns(2, 4, TypeDef.index_size());
            Constant.set_columns(2, HasConstant, blob_index_size);
            CustomAttribute.set_columns(HasCustomAttribute, CustomAttributeType, blob_index_size);
            DeclSecurity.set_columns(2, HasDeclSecurity, blob_index_size);
            EventMap.set_columns(TypeDef.index_size(), Event.index_size());
            Event.set_columns(2, string_index_size, TypeDefOrRef);
            ExportedType.set_columns(4, 4, string_index_size, string_index_size, Implementation);
            Field.set_columns(2, string_index_size, blob_index_size);
            FieldLayout.set_columns(4, Field.index_size());
            FieldMarshal.set_columns(HasFieldMarshal, blob_index_size);
            FieldRVA.set_columns(4, Field.index_size());
            File.set_columns(4, string_index_size, blob_index_size);
            GenericParam.set_columns(2, 2, TypeOrMethodDef, string_index_size);
            GenericParamConstraint.set_columns(GenericParam.index_size(), TypeDefOrRef);
            ImplMap.set_columns(2, MemberForwarded, string_index_size, ModuleRef.index_size());
            InterfaceImpl.set_columns(TypeDef.index_size(), TypeDefOrRef);
            ManifestResource.set_columns(4, 4, string_index_size, Implementation);
            MemberRef.set_columns(MemberRefParent, string_index_size, blob_index_size);
            MethodDef.set_columns(4, 2, 2, string_index_size, blob_index_size, Param.index_size());
            MethodImpl.set_columns(TypeDef.index_size(), MethodDefOrRef, MethodDefOrRef);
            MethodSemantics.set_columns(2, MethodDef.index_size(), HasSemantics);
            MethodSpec.set_columns(MethodDefOrRef, blob_index_size);
            Module.set_columns(2, string_index_size, guid_index_size, guid_index_size, guid_index_size);
            ModuleRef.set_columns(string_index_size);
            NestedClass.set_columns(TypeDef.index_size(), TypeDef.index_size());
            Param.set_columns(2, 2, string_index_size);
            Property.set_columns(2, string_index_size, blob_index_size);
            PropertyMap.set_columns(TypeDef.index_size(), Property.index_size());
            StandAloneSig.set_columns(blob_index_size);
            TypeDef.set_columns(4, string_index_size, string_index_size, TypeDefOrRef, Field.index_size(), MethodDef.index_size());
            TypeRef.set_columns(ResolutionScope, string_index_size, string_index_size);
            TypeSpec.set_columns(blob_index_size);

            Module.set_data(view);
            TypeRef.set_data(view);
            TypeDef.set_data(view);
            Field.set_data(view);
            MethodDef.set_data(view);
            Param.set_data(view);
            InterfaceImpl.set_data(view);
            MemberRef.set_data(view);
            Constant.set_data(view);
            CustomAttribute.set_data(view);
            FieldMarshal.set_data(view);
            DeclSecurity.set_data(view);
            ClassLayout.set_data(view);
            FieldLayout.set_data(view);
            StandAloneSig.set_data(view);
            EventMap.set_data(view);
            Event.set_data(view);
            PropertyMap.set_data(view);
            Property.set_data(view);
            MethodSemantics.set_data(view);
            MethodImpl.set_data(view);
            ModuleRef.set_data(view);
            TypeSpec.set_data(view);
            ImplMap.set_data(view);
            FieldRVA.set_data(view);
            Assembly.set_data(view);
            AssemblyProcessor.set_data(view);
            AssemblyOS.set_data(view);
            AssemblyRef.set_data(view);
            AssemblyRefProcessor.set_data(view);
            AssemblyRefOS.set_data(view);
            File.set_data(view);
            ExportedType.set_data(view);
            ManifestResource.set_data(view);
            NestedClass.set_data(view);
            GenericParam.set_data(view);
            MethodSpec.set_data(view);
            GenericParamConstraint.set_data(view);
        }

        struct stream_range
        {
            uint32_t offset;
            uint32_t size;
        };

        static bool composite_index_size(uint32_t const row_count, uint8_t const bits)
        {
            return row_count < (1ull << (16 - bits));
        }

        template <typename...Tables>
        static uint8_t composite_index_size(Tables const&... tables)
        {
            return (composite_index_size(tables.size(), impl::bits_needed(sizeof...(tables))) && ...) ? 2 : 4;
        }

        static uint32_t stream_offset(std::string_view const& name) noexcept
        {
            uint32_t padding = 4 - name.size() % 4;

            if (padding == 0)
            {
                padding = 4;
            }

            return static_cast<uint32_t>(8 + name.size() + padding);
        }

        static impl::image_section_header const* section_from_rva(impl::image_section_header const* const first, impl::image_section_header const* const last, uint32_t const rva) noexcept
        {
            return std::find_if(first, last, [rva](auto&& section) noexcept
            {
                return rva >= section.VirtualAddress && rva < section.VirtualAddress + section.Misc.VirtualSize;
            });
        }

        static uint32_t offset_from_rva(impl::image_section_header const& section, uint32_t const rva) noexcept
        {
            return rva - section.VirtualAddress + section.PointerToRawData;
        }

        std::vector<uint8_t> m_buffer;
        file_view m_view;

        std::string const m_path;
        byte_view m_strings;
        byte_view m_blobs;
        byte_view m_guids;
        cache const* m_cache;
    };

    template <typename Row>
    inline byte_view row_base<Row>::get_blob(uint32_t const column) const
    {
        return get_database().get_blob(m_table->get_value<uint32_t>(m_index, column));
    }

    template <typename Row>
    inline std::string_view row_base<Row>::get_string(uint32_t const column) const
    {
        return get_database().get_string(m_table->get_value<uint32_t>(m_index, column));
    }

    template <>
    inline table<Module> const& database::get_table<Module>() const noexcept { return Module; }
    template <>
    inline table<TypeRef> const& database::get_table<TypeRef>() const noexcept { return TypeRef; }
    template <>
    inline table<TypeDef> const& database::get_table<TypeDef>() const noexcept { return TypeDef; }
    template <>
    inline table<Field> const& database::get_table<Field>() const noexcept { return Field; }
    template <>
    inline table<MethodDef> const& database::get_table<MethodDef>() const noexcept { return MethodDef; }
    template <>
    inline table<Param> const& database::get_table<Param>() const noexcept { return Param; }
    template <>
    inline table<InterfaceImpl> const& database::get_table<InterfaceImpl>() const noexcept { return InterfaceImpl; }
    template <>
    inline table<MemberRef> const& database::get_table<MemberRef>() const noexcept { return MemberRef; }
    template <>
    inline table<Constant> const& database::get_table<Constant>() const noexcept { return Constant; }
    template <>
    inline table<CustomAttribute> const& database::get_table<CustomAttribute>() const noexcept { return CustomAttribute; }
    template <>
    inline table<FieldMarshal> const& database::get_table<FieldMarshal>() const noexcept { return FieldMarshal; }
    template <>
    inline table<DeclSecurity> const& database::get_table<DeclSecurity>() const noexcept { return DeclSecurity; }
    template <>
    inline table<ClassLayout> const& database::get_table<ClassLayout>() const noexcept { return ClassLayout; }
    template <>
    inline table<FieldLayout> const& database::get_table<FieldLayout>() const noexcept { return FieldLayout; }
    template <>
    inline table<StandAloneSig> const& database::get_table<StandAloneSig>() const noexcept { return StandAloneSig; }
    template <>
    inline table<EventMap> const& database::get_table<EventMap>() const noexcept { return EventMap; }
    template <>
    inline table<Event> const& database::get_table<Event>() const noexcept { return Event; }
    template <>
    inline table<PropertyMap> const& database::get_table<PropertyMap>() const noexcept { return PropertyMap; }
    template <>
    inline table<Property> const& database::get_table<Property>() const noexcept { return Property; }
    template <>
    inline table<MethodSemantics> const& database::get_table<MethodSemantics>() const noexcept { return MethodSemantics; }
    template <>
    inline table<MethodImpl> const& database::get_table<MethodImpl>() const noexcept { return MethodImpl; }
    template <>
    inline table<ModuleRef> const& database::get_table<ModuleRef>() const noexcept { return ModuleRef; }
    template <>
    inline table<TypeSpec> const& database::get_table<TypeSpec>() const noexcept { return TypeSpec; }
    template <>
    inline table<ImplMap> const& database::get_table<ImplMap>() const noexcept { return ImplMap; }
    template <>
    inline table<FieldRVA> const& database::get_table<FieldRVA>() const noexcept { return FieldRVA; }
    template <>
    inline table<Assembly> const& database::get_table<Assembly>() const noexcept { return Assembly; }
    template <>
    inline table<AssemblyProcessor> const& database::get_table<AssemblyProcessor>() const noexcept { return AssemblyProcessor; }
    template <>
    inline table<AssemblyOS> const& database::get_table<AssemblyOS>() const noexcept { return AssemblyOS; }
    template <>
    inline table<AssemblyRef> const& database::get_table<AssemblyRef>() const noexcept { return AssemblyRef; }
    template <>
    inline table<AssemblyRefProcessor> const& database::get_table<AssemblyRefProcessor>() const noexcept { return AssemblyRefProcessor; }
    template <>
    inline table<AssemblyRefOS> const& database::get_table<AssemblyRefOS>() const noexcept { return AssemblyRefOS; }
    template <>
    inline table<File> const& database::get_table<File>() const noexcept { return File; }
    template <>
    inline table<ExportedType> const& database::get_table<ExportedType>() const noexcept { return ExportedType; }
    template <>
    inline table<ManifestResource> const& database::get_table<ManifestResource>() const noexcept { return ManifestResource; }
    template <>
    inline table<NestedClass> const& database::get_table<NestedClass>() const noexcept { return NestedClass; }
    template <>
    inline table<GenericParam> const& database::get_table<GenericParam>() const noexcept { return GenericParam; }
    template <>
    inline table<MethodSpec> const& database::get_table<MethodSpec>() const noexcept { return MethodSpec; }
    template <>
    inline table<GenericParamConstraint> const& database::get_table<GenericParamConstraint>() const noexcept { return GenericParamConstraint; }
}
