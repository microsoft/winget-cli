#pragma once

#include "../base.h"
#include "../meta_reader/pe.h"
#include <algorithm>
#include <fstream>
#include <stdint.h>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

namespace xlang::meta::writer
{
    struct pe_writer
    {
        pe_writer()
        {
            m_header.resize(sizeof(impl::image_dos_header) + sizeof(impl::image_nt_headers32));
        }

        void add_metadata(std::vector<uint8_t> const& metadata)
        {
            auto& s = get_section(".text");
            s.resize(sizeof(impl::image_cor20_header) + metadata.size());
            auto md_dest = s.as<uint8_t>(sizeof(impl::image_cor20_header));
            std::copy(metadata.begin(), metadata.end(), md_dest);
            auto cli_header = s.as<impl::image_cor20_header>(0);
            cli_header->cb = sizeof(impl::image_cor20_header);
            cli_header->MajorRuntimeVersion = 2;
            cli_header->MinorRuntimeVersion = 5;
            cli_header->MetaData.Size = static_cast<uint32_t>(metadata.size());
            s.defer_rva(&(cli_header->MetaData.VirtualAddress), &s, md_dest);
            cli_header->Flags = 0x1; // COMIMAGE_FLAGS_ILONLY

            auto nt_header = get_nt_header();
            nt_header->OptionalHeader.DataDirectory[com_directory].Size = sizeof(impl::image_cor20_header);
            m_header.defer_rva(&(nt_header->OptionalHeader.DataDirectory[com_directory].VirtualAddress), &s, cli_header);
        }

        void save_to_file(std::filesystem::path const& path)
        {
            std::basic_ofstream<uint8_t> output_file{ path, std::ios::binary };
            auto const output = save_to_memory();
            output_file.write(output.data(), output.size());
        }

        std::vector<uint8_t> save_to_memory()
        {
            resolve();
            update_header();
            uint32_t raw_size = m_sections.back().physical_offset() + static_cast<uint32_t>(m_sections.back().size());
            std::vector<uint8_t> output;

            // Write the top headers
            output.reserve(raw_size);
            {
                auto headers_start = m_header.as<uint8_t>(0);
                auto headers_end = headers_start + m_header.size();
                output.insert(output.end(), headers_start, headers_end);
            }
            
            // Write each section header
            for (auto const& s : m_sections)
            {
                impl::image_section_header header{};
                XLANG_ASSERT(s.name().size() <= 8);
                std::copy(s.name().begin(), s.name().end(), header.Name);
                header.Misc.VirtualSize = static_cast<uint32_t>(s.size());
                header.VirtualAddress = s.virtual_offset();
                header.SizeOfRawData = round_up(header.Misc.VirtualSize, file_alignment);
                header.PointerToRawData = s.physical_offset();
                header.Characteristics = 0x40000020; // IMAGE_SCN_MEM_READ | IMAGE_SCN_CNT_CODE
                
                auto begin = reinterpret_cast<uint8_t*>(&header);
                auto end = begin + sizeof(impl::image_section_header);
                output.insert(output.end(), begin, end);
            }

            // Write the sections
            for (auto const& s : m_sections)
            {
                // Alignment padding
                XLANG_ASSERT(output.size() <= s.physical_offset());
                XLANG_ASSERT((s.physical_offset() & (file_alignment - 1)) == 0);
                output.resize(s.physical_offset());

                auto begin = s.as<uint8_t>(0);
                auto end = begin + s.size();
                output.insert(output.end(), begin, end);
            }

            return output;
        }

        struct section
        {
            section() = delete;
            explicit section(std::string_view const& name)
                : m_name(name)
            {
                XLANG_ASSERT(m_name.size() <= 8);
            }
            section(section const&) = delete;
            section& operator=(section const&) = delete;

            section(section&&) = default;
            section& operator=(section&&) = default;

            std::string const& name() const noexcept
            {
                return m_name;
            }

            void resize(std::size_t size)
            {
                XLANG_ASSERT(!assigned_rva());
                m_data.resize(size);
            }

            std::size_t size() const noexcept
            {
                return m_data.size();
            }

            template <typename T>
            T* as(std::size_t offset) noexcept
            {
                XLANG_ASSERT(sizeof(T) + offset <= m_data.size());
                return reinterpret_cast<T*>(m_data.data() + offset);
            }

            template <typename T>
            T const* as(std::size_t offset) const noexcept
            {
                XLANG_ASSERT(sizeof(T) + offset <= m_data.size());
                return reinterpret_cast<T const*>(m_data.data() + offset);
            }

            // Store a yet-to-be resolved RVA at location inside this section.
            // It points to target_address in target_section.
            // After sections are resolved and placed at base RVAs, we can then resolve the deferred RVAs stored here
            void defer_rva(void* location, section const* target_section, void const* target_address)
            {
                uint8_t const* const self_base = this->m_data.data();
                uint8_t const* const target_base = target_section->m_data.data();
                XLANG_ASSERT(self_base <= location &&
                    location <= self_base + this->size() - sizeof(uint32_t));
                XLANG_ASSERT(target_base <= target_address &&
                    target_address < target_base + target_section->size());
                std::size_t offset = static_cast<uint8_t const*>(location) - self_base;
                std::size_t target_offset = static_cast<uint8_t const*>(target_address) - target_base;
                m_unresolved_rvas.emplace_back(rva{ target_section, static_cast<uint32_t>(offset), static_cast<uint32_t>(target_offset) });
            }

            uint32_t virtual_offset() const noexcept
            {
                XLANG_ASSERT(assigned_rva());
                return m_base_rva;
            }

            void virtual_offset(uint32_t offset) noexcept
            {
                XLANG_ASSERT(!assigned_rva());
                m_base_rva = offset;
            }

            void resolve_rvas() noexcept
            {
                for (rva const& entry : m_unresolved_rvas)
                {
                    auto addr = as<uint32_t>(entry.m_offset);
                    XLANG_ASSERT(entry.m_target_offset < entry.m_target_section->size());
                    *addr = entry.m_target_offset + entry.m_target_section->virtual_offset();
                }
            }

            void physical_offset(uint32_t offset) noexcept
            {
                XLANG_ASSERT(m_physical_offset == 0xffffffff);
                m_physical_offset = offset;
            }

            uint32_t physical_offset() const noexcept
            {
                XLANG_ASSERT(m_physical_offset != 0xffffffff);
                return m_physical_offset;
            }

        private:
            bool assigned_rva() const noexcept
            {
                return m_base_rva != 0xffffffff;
            }
            std::string m_name;
            std::vector<uint8_t> m_data;

            struct rva
            {
                section const* m_target_section;
                std::uint32_t m_offset;
                std::uint32_t m_target_offset;
            };
            std::vector<rva> m_unresolved_rvas;
            uint32_t m_base_rva{ 0xffffffff };
            uint32_t m_physical_offset{ 0xffffffff };
        };

        section& get_section(std::string_view const& name)
        {
            if (name.empty() || name.size() > 8)
            {
                throw_invalid("Section names are limited to 8 characters and must be non-empty");
            }

            auto iter = std::find_if(m_sections.begin(), m_sections.end(), [&name](section const& s)
                {
                    return s.name() == name;
                });
            if (iter != m_sections.end())
            {
                return *iter;
            }
            else
            {
                m_sections.emplace_back(name);
                return (m_sections.back());
            }
        }

        section& header() noexcept
        {
            return m_header;
        }

    private:
        static constexpr uint32_t dos_header_offset{ 0 };
        static constexpr uint32_t nt_header_offset{ dos_header_offset + sizeof(impl::image_dos_header) };
        static constexpr uint32_t section_headers_offset{ nt_header_offset + sizeof(impl::image_nt_headers32) };

        static constexpr uint32_t com_directory{ 14 }; // IMAGE_DIRECTORY_ENTRY_COM_DESCRIPTOR

        static constexpr uint32_t file_alignment{ 0x200 };
        static constexpr uint32_t section_alignment{ 0x1000 };

        static uint32_t round_up(uint32_t size, uint32_t alignment)
        {
            return (size + alignment - 1) & ~(alignment - 1);
        }

        impl::image_dos_header* get_dos_header() noexcept
        {
            return m_header.as<impl::image_dos_header>(dos_header_offset);
        }

        impl::image_nt_headers32* get_nt_header() noexcept
        {
            return m_header.as<impl::image_nt_headers32>(nt_header_offset);
        }

        uint32_t get_raw_end_of_headers() const noexcept
        {
            return section_headers_offset + static_cast<uint32_t>(m_sections.size() * sizeof(impl::image_section_header));
        }

        void resolve()
        {
            m_header.virtual_offset(0);
            uint32_t const start_of_sections = get_raw_end_of_headers();
            
            uint32_t physical_offset = round_up(start_of_sections, file_alignment);
            uint32_t virtual_offset = round_up(start_of_sections, section_alignment);
            for (auto& s : m_sections)
            {
                s.physical_offset(physical_offset);
                s.virtual_offset(virtual_offset);
                uint32_t const size = static_cast<uint32_t>(s.size());
                physical_offset = round_up(physical_offset + size, file_alignment);
                virtual_offset = round_up(virtual_offset + size, section_alignment);
            }

            m_header.resolve_rvas();

            for (auto& s : m_sections)
            {
                s.resolve_rvas();
            }
        }

        void update_header()
        {
            uint32_t const raw_image_size = m_sections.back().physical_offset() + static_cast<uint32_t>(m_sections.back().size());
            uint32_t const raw_header_size = get_raw_end_of_headers();
            {
                auto dos_header = get_dos_header();
                dos_header->e_magic = 0x5a4d; // "MZ
                dos_header->e_lfanew = nt_header_offset;
            }
            {
                auto nt_header = get_nt_header();
                nt_header->Signature = 0x4550; // "PE
                
                auto& file_header = nt_header->FileHeader;
                file_header.Machine = 0x014c; // IMAGE_FILE_MACHINE_I386
                file_header.NumberOfSections = static_cast<uint16_t>(m_sections.size());
                file_header.TimeDateStamp = static_cast<uint32_t>(time(nullptr));
                file_header.SizeOfOptionalHeader = static_cast<uint16_t>(sizeof(impl::image_optional_header32));
                file_header.Characteristics = 0x2102; // IMAGE_FILE_DLL | IMAGE_FILE_32BIT_MACHINE | IMAGE_FILE_EXECUTABLE_IMAGE

                auto& optional_header = nt_header->OptionalHeader;
                optional_header.Magic = 0x010b; // IMAGE_NT_OPTIONAL_HDR32_MAGIC
                optional_header.MajorLinkerVersion = 6;
                optional_header.ImageBase = 0x400000;
                optional_header.SectionAlignment = section_alignment;
                optional_header.FileAlignment = file_alignment;
                optional_header.MajorOperatingSystemVersion = 5;
                optional_header.MajorSubsystemVersion = 5;
                optional_header.SizeOfImage = round_up(raw_image_size, section_alignment);
                optional_header.SizeOfHeaders = round_up(raw_header_size, file_alignment);
                optional_header.Subsystem = 0x3; // IMAGE_SUBSYSTEM_WINDOWS_CUI
                optional_header.DllCharacteristics = 0x540; // IMAGE_DLLCHARACTERISTICS_NO_SEH | IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE
                optional_header.SizeOfStackReserve = 0x100000;
                optional_header.SizeOfStackCommit = 0x1000;
                optional_header.SizeOfHeapReserve = 0x100000;
                optional_header.SizeOfHeapCommit = 0x1000;
                optional_header.NumberOfRvaAndSizes = 16;
            }
        }

        section m_header{ "" };
        std::vector<section> m_sections;
    };
}
