// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.
#include "pch.h"
#include "WinGetUtil.h"

#pragma comment(lib, "Cabinet.lib")
#pragma comment(lib, "winsqlite3.lib")

namespace
{
    // -------------------------------------------------------------------------
    // WinGetUtil runtime loading
    // -------------------------------------------------------------------------

    struct WinGetApi
    {
        HMODULE module = nullptr;
        PFN_WinGetSQLiteIndexCreate              Create = nullptr;
        PFN_WinGetSQLiteIndexAddManifest         AddManifest = nullptr;
        PFN_WinGetSQLiteIndexPrepareForPackaging PrepareForPackaging = nullptr;
        PFN_WinGetSQLiteIndexClose               Close = nullptr;

        ~WinGetApi() { if (module) FreeLibrary(module); }

        explicit operator bool() const { return module && Create && AddManifest && PrepareForPackaging && Close; }
    };

    template<typename T>
    T GetProc(HMODULE mod, const char* name)
    {
        return reinterpret_cast<T>(GetProcAddress(mod, name));
    }

    // Loads WinGetUtil.dll from dllPath (if non-empty) or via the standard DLL
    // search order (which includes the application directory first).
    WinGetApi LoadWinGetUtil(const std::filesystem::path& dllPath)
    {
        WinGetApi api;

        if (!dllPath.empty())
        {
            api.module = LoadLibraryExW(dllPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
            if (!api.module)
            {
                std::wcerr << L"Error: could not load WinGetUtil.dll from: " << dllPath << L"\n"
                           << L"       GetLastError: " << GetLastError() << L"\n";
                return api;
            }
        }
        else
        {
            api.module = LoadLibraryW(L"WinGetUtil.dll");
            if (!api.module)
            {
                std::wcerr << L"Error: could not load WinGetUtil.dll.\n"
                           << L"       Place WinGetUtil.dll in the same directory as this executable,\n"
                           << L"       or specify its path with --wingetutil <path>.\n"
                           << L"       GetLastError: " << GetLastError() << L"\n";
                return api;
            }
        }

        api.Create              = GetProc<PFN_WinGetSQLiteIndexCreate>             (api.module, "WinGetSQLiteIndexCreate");
        api.AddManifest         = GetProc<PFN_WinGetSQLiteIndexAddManifest>        (api.module, "WinGetSQLiteIndexAddManifest");
        api.PrepareForPackaging = GetProc<PFN_WinGetSQLiteIndexPrepareForPackaging>(api.module, "WinGetSQLiteIndexPrepareForPackaging");
        api.Close               = GetProc<PFN_WinGetSQLiteIndexClose>              (api.module, "WinGetSQLiteIndexClose");

        if (!api)
        {
            std::wcerr << L"Error: WinGetUtil.dll is missing one or more required exports.\n"
                       << L"       Ensure the DLL matches the expected version.\n";
        }

        return api;
    }

    // -------------------------------------------------------------------------
    // String helpers
    // -------------------------------------------------------------------------

    std::wstring ToUTF16(std::string_view utf8)
    {
        if (utf8.empty()) return {};
        int len = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
        std::wstring result(len, L'\0');
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(), len);
        return result;
    }

    std::string ToUTF8(std::wstring_view utf16)
    {
        if (utf16.empty()) return {};
        int len = WideCharToMultiByte(CP_UTF8, 0, utf16.data(), static_cast<int>(utf16.size()), nullptr, 0, nullptr, nullptr);
        std::string result(len, '\0');
        WideCharToMultiByte(CP_UTF8, 0, utf16.data(), static_cast<int>(utf16.size()), result.data(), len, nullptr, nullptr);
        return result;
    }

    // -------------------------------------------------------------------------
    // Argument parsing
    // -------------------------------------------------------------------------

    struct Args
    {
        std::filesystem::path manifestsDir;
        std::filesystem::path wingetUtilPath; // optional: explicit path to WinGetUtil.dll
        std::optional<std::filesystem::path> outputFile;
        std::optional<std::filesystem::path> baselineFile;
        bool prebuilt = false;
        bool verbose = false;
        bool showHelp = false;
    };

    void PrintUsage(const wchar_t* programName)
    {
        std::wcout
            << L"Usage: " << programName << L" <manifests-dir> [options]\n"
            << L"\n"
            << L"Builds a WinGet source index from a directory of YAML manifests and\n"
            << L"measures its raw and compressed size (XPRESS Huffman, approximating MSIX).\n"
            << L"\n"
            << L"Options:\n"
            << L"  --prebuilt           The provided path is a prebuilt index to check\n"
            << L"  --wingetutil <path>  Path to WinGetUtil.dll\n"
            << L"                       (default: searches application directory and PATH)\n"
            << L"  --output <file>      Write results as JSON to <file>\n"
            << L"  --baseline <file>    Compare against a prior JSON result (from --output)\n"
            << L"  --verbose            Show per-table row counts and page usage\n"
            << L"  --help               Show this help\n";
    }

    Args ParseArgs(int argc, wchar_t* argv[])
    {
        Args args;
        for (int i = 1; i < argc; ++i)
        {
            std::wstring_view arg{ argv[i] };
            if (arg == L"--help" || arg == L"-h" || arg == L"-?")
            {
                args.showHelp = true;
            }
            else if (arg == L"--prebuilt" || arg == L"-v")
            {
                args.prebuilt = true;
            }
            else if (arg == L"--verbose" || arg == L"-v")
            {
                args.verbose = true;
            }
            else if ((arg == L"--output" || arg == L"-o") && i + 1 < argc)
            {
                args.outputFile = argv[++i];
            }
            else if ((arg == L"--baseline" || arg == L"-b") && i + 1 < argc)
            {
                args.baselineFile = argv[++i];
            }
            else if (arg == L"--wingetutil" && i + 1 < argc)
            {
                args.wingetUtilPath = argv[++i];
            }
            else if (args.manifestsDir.empty() && !arg.empty() && arg[0] != L'-')
            {
                args.manifestsDir = arg;
            }
        }
        return args;
    }

    // -------------------------------------------------------------------------
    // Manifest discovery
    // -------------------------------------------------------------------------

    // Returns directories that directly contain at least one .yaml file.
    // Each such directory represents one package version manifest.
    std::vector<std::filesystem::path> FindManifestDirs(const std::filesystem::path& root)
    {
        std::vector<std::filesystem::path> result;
        std::error_code ec;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(
            root, std::filesystem::directory_options::skip_permission_denied, ec))
        {
            if (!entry.is_directory(ec)) continue;
            for (const auto& f : std::filesystem::directory_iterator(entry.path(), ec))
            {
                if (f.is_regular_file(ec) && f.path().extension() == L".yaml")
                {
                    result.push_back(entry.path());
                    break;
                }
            }
        }
        return result;
    }

    // -------------------------------------------------------------------------
    // Index building
    // -------------------------------------------------------------------------

    struct ManifestStats
    {
        uint64_t added = 0;
        uint64_t failed = 0;
    };

    HRESULT AddManifest(
        const WinGetApi&              api,
        WINGET_SQLITE_INDEX_HANDLE    index,
        const std::filesystem::path&  manifestDir,
        const std::filesystem::path&  relPath)
    {
        std::wstring manifestPathW = manifestDir.wstring();
        std::wstring relPathW      = relPath.generic_wstring(); // forward slashes
        return api.AddManifest(index, manifestPathW.c_str(), relPathW.c_str());
    }

    ManifestStats BuildIndex(
        const WinGetApi&             api,
        WINGET_SQLITE_INDEX_HANDLE   index,
        const std::filesystem::path& manifestsDir)
    {
        auto manifestDirs = FindManifestDirs(manifestsDir);
        std::wcout << L"Found " << manifestDirs.size() << L" manifest directories. Adding to index...\n";

        ManifestStats stats;
        std::vector<std::filesystem::path> retryList;

    retry:
        uint64_t processed = 0;
        auto start = std::chrono::steady_clock::now();
        for (const auto& dir : manifestDirs)
        {
            std::filesystem::path relPath = std::filesystem::relative(dir, manifestsDir);
            if (SUCCEEDED(AddManifest(api, index, dir, relPath)))
            {
                ++stats.added;
            }
            else
            {
                retryList.push_back(dir);
            }

            if (++processed % 1000 == 0)
            {
                auto now = std::chrono::steady_clock::now();
                auto duration = now - start;
                start = now;
                std::wcout << L"  " << processed << L" / " << manifestDirs.size() << L" processed (~" << std::chrono::duration_cast<std::chrono::milliseconds>(duration / 1000).count() << L"ms per)...\r";
            }
        }

        // Retry failures until no progress is made
        if (!retryList.empty())
        {
            if (retryList.size() < manifestDirs.size())
            {
                std::wcout << L"\nRetrying " << retryList.size() << L" failed manifests...\n";
                manifestDirs = std::move(retryList);
                retryList.clear();
                goto retry;
            }
            else
            {
                std::wcout << L"\nDropping " << retryList.size() << L" failed manifests...\n";
                stats.failed = retryList.size();
            }
        }

        std::wcout << L"\nAdded: " << stats.added << L"  Failed: " << stats.failed << L"\n";
        return stats;
    }

    // -------------------------------------------------------------------------
    // Compression measurement (XPRESS Huffman, the algorithm used by MSIX)
    // -------------------------------------------------------------------------

    uint64_t MeasureCompressed(const std::filesystem::path& file)
    {
        std::ifstream ifs(file, std::ios::binary | std::ios::ate);
        if (!ifs) throw std::exception{ "Couldn't open file." };

        auto fileSize = static_cast<size_t>(ifs.tellg());
        ifs.seekg(0);
        std::vector<BYTE> data(fileSize);
        ifs.read(reinterpret_cast<char*>(data.data()), fileSize);
        ifs.close();

        COMPRESSOR_HANDLE compressor = nullptr;
        if (!CreateCompressor(COMPRESS_ALGORITHM_XPRESS_HUFF, nullptr, &compressor))
        {
            throw std::exception{ "Couldn't create compressor." };
        }

        SIZE_T compressedSize = 0;
        // First call determines required output buffer size
        Compress(compressor, data.data(), data.size(), nullptr, 0, &compressedSize);

        std::vector<BYTE> compressed(compressedSize);
        if (!Compress(compressor, data.data(), data.size(), compressed.data(), compressedSize, &compressedSize))
        {
            CloseCompressor(compressor);
            throw std::exception{ "Couldn't compress." };
        }

        CloseCompressor(compressor);
        return static_cast<uint64_t>(compressedSize);
    }

    // -------------------------------------------------------------------------
    // Table stats (--verbose)
    // -------------------------------------------------------------------------

    struct TableInfo
    {
        std::string name;
        int64_t rowCount = 0;
        int64_t pageCount = 0;
    };

    std::vector<TableInfo> QueryTableStats(const std::filesystem::path& dbFile)
    {
        std::vector<TableInfo> result;

        sqlite3* db = nullptr;
        if (sqlite3_open_v2(ToUTF8(dbFile.wstring()).c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK)
        {
            return result;
        }

        // Enumerate tables
        std::vector<std::string> tableNames;
        {
            sqlite3_stmt* stmt = nullptr;
            if (sqlite3_prepare_v2(db,
                "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name",
                -1, &stmt, nullptr) == SQLITE_OK)
            {
                while (sqlite3_step(stmt) == SQLITE_ROW)
                {
                    tableNames.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
                }
                sqlite3_finalize(stmt);
            }
        }

        for (const auto& name : tableNames)
        {
            TableInfo info;
            info.name = name;

            // Row count
            {
                sqlite3_stmt* stmt = nullptr;
                std::string q = "SELECT COUNT(*) FROM [" + name + "]";
                if (sqlite3_prepare_v2(db, q.c_str(), -1, &stmt, nullptr) == SQLITE_OK)
                {
                    if (sqlite3_step(stmt) == SQLITE_ROW)
                    {
                        info.rowCount = sqlite3_column_int64(stmt, 0);
                    }
                    sqlite3_finalize(stmt);
                }
            }

            // Page count via dbstat virtual table (may not be available on all builds)
            {
                sqlite3_stmt* stmt = nullptr;
                if (sqlite3_prepare_v2(db,
                    "SELECT COUNT(*) FROM dbstat WHERE name=?",
                    -1, &stmt, nullptr) == SQLITE_OK)
                {
                    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
                    if (sqlite3_step(stmt) == SQLITE_ROW)
                    {
                        info.pageCount = sqlite3_column_int64(stmt, 0);
                    }
                    sqlite3_finalize(stmt);
                }
            }

            result.push_back(std::move(info));
        }

        // Sort by page count descending so the largest tables appear first
        std::sort(result.begin(), result.end(),
            [](const TableInfo& a, const TableInfo& b) { return a.pageCount > b.pageCount; });

        sqlite3_close(db);
        return result;
    }

    // -------------------------------------------------------------------------
    // Output
    // -------------------------------------------------------------------------

    struct Results
    {
        std::filesystem::path manifestsDir;
        uint64_t manifestCount = 0;
        uint64_t failedCount = 0;
        uint64_t rawBytes = 0;
        uint64_t compressedBytes = 0;
        std::vector<TableInfo> tables;

        double CompressionRatio() const
        {
            return rawBytes > 0 ? static_cast<double>(compressedBytes) / rawBytes : 0.0;
        }
    };

    std::string FormatBytes(uint64_t bytes)
    {
        std::ostringstream ss;
        if (bytes >= 1024ULL * 1024)
        {
            ss << std::fixed << std::setprecision(1) << (bytes / (1024.0 * 1024.0)) << " MB";
        }
        else if (bytes >= 1024)
        {
            ss << std::fixed << std::setprecision(1) << (bytes / 1024.0) << " KB";
        }
        else
        {
            ss << bytes << " B";
        }
        ss << "  (" << bytes << " bytes)";
        return ss.str();
    }

    void PrintResults(const Results& r, bool verbose)
    {
        double ratio   = r.CompressionRatio();
        double savings = 1.0 - ratio;

        std::wcout << L"\n";
        std::wcout << L"Index built from:  " << r.manifestsDir.c_str() << L"\n";
        std::wcout << L"Manifests added:   " << r.manifestCount;
        if (r.failedCount > 0) { std::wcout << L"  (" << r.failedCount << L" failed)"; }
        std::wcout << L"\n";
        std::wcout << L"Raw size:          " << ToUTF16(FormatBytes(r.rawBytes)) << L"\n";
        std::wcout << L"Compressed size:   " << ToUTF16(FormatBytes(r.compressedBytes)) << L"  (XPRESS Huffman)\n";
        std::wcout << std::fixed << std::setprecision(1);
        std::wcout << L"Compression ratio: " << (ratio * 100) << L"%"
                   << L"  (" << (savings * 100) << L"% savings)\n";

        if (verbose && !r.tables.empty())
        {
            std::wcout << L"\nTable breakdown (sorted by page count):\n";
            std::wcout << std::left
                       << std::setw(40) << L"Table"
                       << std::setw(12) << L"Rows"
                       << std::setw(8)  << L"Pages"
                       << L"\n"
                       << std::wstring(60, L'-') << L"\n";
            for (const auto& t : r.tables)
            {
                std::wcout << std::setw(40) << ToUTF16(t.name)
                           << std::setw(12) << t.rowCount
                           << std::setw(8)  << t.pageCount
                           << L"\n";
            }
        }
    }

    // -------------------------------------------------------------------------
    // JSON output
    // -------------------------------------------------------------------------

    std::string JsonEscapeString(const std::string& s)
    {
        std::string r;
        r.reserve(s.size() + 2);
        r += '"';
        for (char c : s)
        {
            if (c == '"')  r += "\\\"";
            else if (c == '\\') r += "\\\\";
            else r += c;
        }
        r += '"';
        return r;
    }

    void WriteJson(const Results& r, bool verbose, const std::filesystem::path& outFile)
    {
        std::ofstream ofs(outFile);
        if (!ofs) { throw std::runtime_error("Cannot open output file: " + ToUTF8(outFile.wstring())); }

        ofs << "{\n";
        ofs << "  \"manifestsDir\": "   << JsonEscapeString(ToUTF8(r.manifestsDir.wstring())) << ",\n";
        ofs << "  \"manifestCount\": "  << r.manifestCount  << ",\n";
        ofs << "  \"failedCount\": "    << r.failedCount    << ",\n";
        ofs << "  \"rawBytes\": "       << r.rawBytes       << ",\n";
        ofs << "  \"compressedBytes\": " << r.compressedBytes << ",\n";
        ofs << std::fixed << std::setprecision(4);
        ofs << "  \"compressionRatio\": " << r.CompressionRatio();

        if (verbose && !r.tables.empty())
        {
            ofs << ",\n  \"tables\": [\n";
            for (size_t i = 0; i < r.tables.size(); ++i)
            {
                const auto& t = r.tables[i];
                ofs << "    { \"name\": " << JsonEscapeString(t.name)
                    << ", \"rowCount\": "  << t.rowCount
                    << ", \"pageCount\": " << t.pageCount << " }";
                if (i + 1 < r.tables.size()) ofs << ",";
                ofs << "\n";
            }
            ofs << "  ]\n";
        }
        else
        {
            ofs << "\n";
        }
        ofs << "}\n";
    }

    // -------------------------------------------------------------------------
    // Baseline comparison
    // -------------------------------------------------------------------------

    bool ExtractJsonUint64(const std::string& json, const std::string& key, uint64_t& value)
    {
        std::string searchKey = "\"" + key + "\": ";
        auto pos = json.find(searchKey);
        if (pos == std::string::npos) return false;
        pos += searchKey.size();
        try { value = std::stoull(json.substr(pos)); return true; }
        catch (...) { return false; }
    }

    bool ExtractJsonDouble(const std::string& json, const std::string& key, double& value)
    {
        std::string searchKey = "\"" + key + "\": ";
        auto pos = json.find(searchKey);
        if (pos == std::string::npos) return false;
        pos += searchKey.size();
        try { value = std::stod(json.substr(pos)); return true; }
        catch (...) { return false; }
    }

    void CompareWithBaseline(const Results& current, const std::filesystem::path& baselineFile)
    {
        std::ifstream ifs(baselineFile);
        if (!ifs)
        {
            std::wcerr << L"Warning: cannot open baseline file: " << baselineFile << L"\n";
            return;
        }
        std::string json{ std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>() };

        uint64_t baseRaw = 0, baseCompressed = 0;
        double   baseRatio = 0.0;
        if (!ExtractJsonUint64(json, "rawBytes", baseRaw) ||
            !ExtractJsonUint64(json, "compressedBytes", baseCompressed) ||
            !ExtractJsonDouble(json, "compressionRatio", baseRatio))
        {
            std::wcerr << L"Warning: could not parse required fields from baseline JSON.\n";
            return;
        }

        auto signedBytes = [](int64_t delta) -> std::wstring
        {
            std::wostringstream ss;
            if (delta >= 0) ss << L"+";
            ss << delta << L" bytes";
            return ss.str();
        };
        auto pctChange = [](uint64_t cur, uint64_t base) -> std::wstring
        {
            if (base == 0) return L"N/A";
            double pct = 100.0 * (static_cast<double>(cur) - base) / base;
            std::wostringstream ss;
            if (pct >= 0) ss << L"+";
            ss << std::fixed << std::setprecision(1) << pct << L"%";
            return ss.str();
        };

        int64_t rawDelta   = static_cast<int64_t>(current.rawBytes)        - static_cast<int64_t>(baseRaw);
        int64_t compDelta  = static_cast<int64_t>(current.compressedBytes) - static_cast<int64_t>(baseCompressed);
        double  ratioDelta = current.CompressionRatio() - baseRatio;

        std::wostringstream ratioSS;
        if (ratioDelta >= 0) ratioSS << L"+";
        ratioSS << std::fixed << std::setprecision(2) << (ratioDelta * 100) << L"pp"
                << L"  (was " << std::setprecision(1) << (baseRatio * 100) << L"%)";

        std::wcout << L"\nvs baseline (" << baselineFile.filename().wstring() << L"):\n";
        std::wcout << L"  Raw size:          " << signedBytes(rawDelta)
                   << L"  (" << pctChange(current.rawBytes, baseRaw) << L")\n";
        std::wcout << L"  Compressed size:   " << signedBytes(compDelta)
                   << L"  (" << pctChange(current.compressedBytes, baseCompressed) << L")\n";
        std::wcout << L"  Compression ratio: " << ratioSS.str() << L"\n";
    }

} // anonymous namespace

// -------------------------------------------------------------------------
// Entry point
// -------------------------------------------------------------------------

int wmain(int argc, wchar_t* argv[])
{
    Args args = ParseArgs(argc, argv);

    if (args.showHelp || args.manifestsDir.empty())
    {
        PrintUsage(argc > 0 ? argv[0] : L"IndexComparisonTool");
        return args.showHelp ? 0 : 1;
    }

    if (!std::filesystem::is_directory(args.manifestsDir) && !args.prebuilt)
    {
        std::wcerr << L"Error: not a directory: " << args.manifestsDir << L"\n";
        return 1;
    }

    if (std::filesystem::is_directory(args.manifestsDir) && args.prebuilt)
    {
        std::wcerr << L"Error: not a prebuilt file: " << args.manifestsDir << L"\n";
        return 1;
    }

    std::filesystem::path indexPath;
    ManifestStats mstats{};
    if (!args.prebuilt)
    {
        // Load WinGetUtil.dll at runtime
        WinGetApi api = LoadWinGetUtil(args.wingetUtilPath);
        if (!api) { return 1; }

        // Create a temp file for the index
        wchar_t tempDir[MAX_PATH + 1]{};
        wchar_t tempFile[MAX_PATH + 1]{};
        GetTempPathW(MAX_PATH, tempDir);
        GetTempFileNameW(tempDir, L"idx", 0, tempFile);
        indexPath = tempFile;

        // Create the index
        std::wcout << L"Creating index at " << indexPath.c_str() << L"...\n";
        WINGET_SQLITE_INDEX_HANDLE index = nullptr;
        HRESULT hr = api.Create(
            indexPath.c_str(),
            2,
            0,
            &index);

        if (FAILED(hr))
        {
            std::wcerr << L"Error: WinGetSQLiteIndexCreate failed: 0x"
                << std::hex << std::uppercase << hr << L"\n";
            return 1;
        }

        // RAII guard: close index on early return
        bool indexClosed = false;
        struct IndexGuard
        {
            PFN_WinGetSQLiteIndexClose close;
            WINGET_SQLITE_INDEX_HANDLE handle;
            bool& closed;
            ~IndexGuard() { if (!closed && handle) close(handle); }
        } indexGuard{ api.Close, index, indexClosed };

        // Populate the index
        mstats = BuildIndex(api, index, args.manifestsDir);

        // Finalize: VACUUM and drop build-time indices
        std::wcout << L"Preparing for packaging (VACUUM + drop indices)...\n";
        hr = api.PrepareForPackaging(index);
        if (FAILED(hr))
        {
            std::wcerr << L"Error: WinGetSQLiteIndexPrepareForPackaging failed: 0x"
                << std::hex << std::uppercase << hr << L"\n";
            return 1;
        }

        indexClosed = true;
        api.Close(index);
    }
    else
    {
        indexPath = args.manifestsDir;
    }

    // Optionally query table stats from the now-closed database
    std::vector<TableInfo> tables;
    if (args.verbose)
    {
        tables = QueryTableStats(indexPath);
    }

    // Measure sizes
    uint64_t rawBytes        = std::filesystem::file_size(indexPath);
    std::wcout << L"Measuring compression...\n";
    uint64_t compressedBytes = MeasureCompressed(indexPath);

    Results results;
    results.manifestsDir    = std::filesystem::weakly_canonical(args.manifestsDir);
    results.manifestCount   = mstats.added;
    results.failedCount     = mstats.failed;
    results.rawBytes        = rawBytes;
    results.compressedBytes = compressedBytes;
    results.tables          = std::move(tables);

    PrintResults(results, args.verbose);

    if (args.outputFile)
    {
        try
        {
            WriteJson(results, args.verbose, *args.outputFile);
            std::wcout << L"\nResults written to: " << args.outputFile->wstring() << L"\n";
        }
        catch (const std::exception& e)
        {
            std::wcerr << L"Warning: failed to write JSON: " << ToUTF16(e.what()) << L"\n";
        }
    }

    if (args.baselineFile)
    {
        CompareWithBaseline(results, *args.baselineFile);
    }

    return 0;
}
