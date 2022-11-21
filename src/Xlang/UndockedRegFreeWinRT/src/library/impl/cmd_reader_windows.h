
#if XLANG_PLATFORM_WINDOWS

namespace xlang::impl
{
    struct registry_key
    {
        HKEY handle{};

        registry_key(registry_key const&) = delete;
        registry_key& operator=(registry_key const&) = delete;

        ~registry_key() noexcept
        {
            if (handle)
            {
                RegCloseKey(handle);
            }
        }
    };

    template <typename T>
    struct com_ptr
    {
        T* ptr{};

        com_ptr(com_ptr const&) = delete;
        com_ptr& operator=(com_ptr const&) = delete;

        com_ptr() noexcept = default;

        ~com_ptr() noexcept
        {
            if (ptr)
            {
                ptr->Release();
            }
        }

        auto operator->() const noexcept
        {
            return ptr;
        }
    };

    static void check_xml(HRESULT result)
    {
        if (result < 0)
        {
            throw_invalid("Could not read the Windows SDK's Platform.xml");
        }
    }

    inline void add_files_from_xml(
        std::set<std::string>& files,
        std::string const& sdk_version,
        std::filesystem::path const& xml_path,
        std::filesystem::path const& sdk_path)
    {
        com_ptr<IStream> stream;

        check_xml(SHCreateStreamOnFileW(
            xml_path.c_str(),
            STGM_READ, &stream.ptr));

        com_ptr<IXmlReader> reader;

        check_xml(CreateXmlReader(
            __uuidof(IXmlReader),
            reinterpret_cast<void**>(&reader.ptr),
            nullptr));

        check_xml(reader->SetInput(stream.ptr));
        XmlNodeType node_type = XmlNodeType_None;

        while (S_OK == reader->Read(&node_type))
        {
            if (node_type != XmlNodeType_Element)
            {
                continue;
            }

            wchar_t const* value{ nullptr };
            check_xml(reader->GetLocalName(&value, nullptr));

            if (0 != wcscmp(value, L"ApiContract"))
            {
                continue;
            }

            auto path = sdk_path;
            path /= L"References";
            path /= sdk_version;

            check_xml(reader->MoveToAttributeByName(L"name", nullptr));
            check_xml(reader->GetValue(&value, nullptr));
            path /= value;

            check_xml(reader->MoveToAttributeByName(L"version", nullptr));
            check_xml(reader->GetValue(&value, nullptr));
            path /= value;

            check_xml(reader->MoveToAttributeByName(L"name", nullptr));
            check_xml(reader->GetValue(&value, nullptr));
            path /= value;

            path += L".winmd";
            files.insert(path.string());
        }
    }

    inline registry_key open_sdk()
    {
        HKEY key;

        if (0 != RegOpenKeyExW(
            HKEY_LOCAL_MACHINE,
            L"SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
            0,
            KEY_READ,
            &key))
        {
            throw_invalid("Could not find the Windows SDK in the registry");
        }

        return { key };
    }

    inline std::filesystem::path get_sdk_path()
    {
        auto key = open_sdk();

        DWORD path_size = 0;

        if (0 != RegQueryValueExW(
            key.handle,
            L"KitsRoot10",
            nullptr,
            nullptr,
            nullptr,
            &path_size))
        {
            throw_invalid("Could not find the Windows SDK path in the registry");
        }

        std::wstring root((path_size / sizeof(wchar_t)) - 1, L'?');

        RegQueryValueExW(
            key.handle,
            L"KitsRoot10",
            nullptr,
            nullptr,
            reinterpret_cast<BYTE*>(root.data()),
            &path_size);

        return root;
    }

    inline std::string get_module_path()
    {
        std::string path(100, '?');
        DWORD actual_size{};

        while (true)
        {
            actual_size = GetModuleFileNameA(nullptr, path.data(), 1 + static_cast<uint32_t>(path.size()));

            if (actual_size < 1 + path.size())
            {
                path.resize(actual_size);
                break;
            }
            else
            {
                path.resize(path.size() * 2, '?');
            }
        }

        return path;
    }

    inline std::string get_sdk_version()
    {
        auto module_path = get_module_path();
        std::regex rx(R"(((\d+)\.(\d+)\.(\d+)\.(\d+)))");
        std::cmatch match;
        auto sdk_path = get_sdk_path();

        if (std::regex_search(module_path.c_str(), match, rx))
        {
            auto path = sdk_path / "Platforms\\UAP" / match[1].str() / "Platform.xml";

            if (std::filesystem::exists(path))
            {
                return match[1].str();
            }
        }

        auto key = open_sdk();
        uint32_t index{};
        std::array<char, 100> subkey;
        std::array<unsigned long, 4> version_parts{};
        std::string result;

        while (0 == RegEnumKeyA(key.handle, index++, subkey.data(), static_cast<uint32_t>(subkey.size())))
        {
            if (!std::regex_match(subkey.data(), match, rx))
            {
                continue;
            }

            auto path = sdk_path / "Platforms\\UAP" / match[1].str() / "Platform.xml";
            if (!std::filesystem::exists(path))
            {
                continue;
            }

            char* next_part = subkey.data();
            bool force_newer = false;

            for (size_t i = 0; ; ++i)
            {
                auto version_part = strtoul(next_part, &next_part, 10);

                if ((version_part < version_parts[i]) && !force_newer)
                {
                    break;
                }
                else if (version_part > version_parts[i])
                {
                    // E.g. ensure something like '2.1' is considered newer than '1.2'
                    force_newer = true;
                }

                version_parts[i] = version_part;

                if (i == std::size(version_parts) - 1)
                {
                    result = subkey.data();
                    break;
                }

                if (!next_part)
                {
                    break;
                }

                ++next_part;
            }
        }

        if (result.empty())
        {
            throw_invalid("Could not find the Windows SDK");
        }

        return result;
    }
}

#endif
