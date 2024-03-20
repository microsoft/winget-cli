<h1 align="center">bit7z</h1>

<h3 align="center">A C++ static library offering a clean and simple interface to the 7-zip shared libraries</h3>

<!-- navbar -->
<p align="center">
  <a href="#-supported-features" title="List of Features Supported by the Library">Supported Features</a> •
  <a href="#-getting-started-library-usage" title="Basic Source Code Examples">Getting Started</a> •
  <a href="#-download" title="Download Pre-compiled Packages">Download</a> •
  <a href="#-requirements" title="Usage Requirements">Requirements</a> •
  <a href="#%EF%B8%8F-building-and-using-bit7z" title="Building the Library">Building & Using</a> •
  <a href="#%EF%B8%8F-donate" title="Support the Project">Donate</a> •
  <a href="#-license" title="Project License">License</a>
</p>
<!-- navbar -->

<div align="center">
  <a href="https://github.com/rikyoz/bit7z/releases" title="Latest Stable GitHub Release"><img src="https://img.shields.io/github/release/rikyoz/bit7z/all.svg?style=flat&logo=github&logoColor=white&colorB=blue&label=" alt="GitHub release"></a>&thinsp;<img src="https://img.shields.io/badge/-C++14-3F63B3.svg?style=flat&logo=C%2B%2B&logoColor=white" alt="C++14" title="C++ Standards Used: C++14">&thinsp;<img src="https://img.shields.io/badge/-Windows-6E46A2.svg?style=flat&logo=windows-11&logoColor=white" alt="Windows" title="Supported Platform: Windows">&thinsp;<img src="https://img.shields.io/badge/-Linux-9C2A91.svg?style=flat&logo=linux&logoColor=white" alt="Linux" title="Supported Platform: Linux">&thinsp;<img src="https://img.shields.io/badge/-macOS-red.svg?style=flat&logo=apple&logoColor=white" alt="macOS" title="Supported Platform: macOS">&thinsp;<img src="https://img.shields.io/badge/-x86%20&middot;%20x64%20&middot;%20arm%20&middot;%20arm64-orange.svg?style=flat&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHhtbDpzcGFjZT0icHJlc2VydmUiIHZpZXdCb3g9IjAgMCA5NDIgOTQyIj48cGF0aCBmaWxsPSIjZmZmIiBkPSJNNTc5LjEgODk0YTQ4IDQ4IDAgMCAwIDk2IDB2LTc3LjVoLTk1LjlWODk0aC0uMXpNNTc5LjEgNDh2NzcuNUg2NzVWNDhhNDggNDggMCAwIDAtOTUuOSAwek00MjMgNDh2NzcuNWg5NlY0OGE0OCA0OCAwIDAgMC05NiAwek00MjMgODk0YTQ4IDQ4IDAgMCAwIDk2IDB2LTc3LjVoLTk2Vjg5NHpNMjY3IDQ4djc3LjVoOTUuOVY0OGE0OCA0OCAwIDAgMC05NS45IDB6TTI2NyA4OTRhNDggNDggMCAwIDAgOTYgMHYtNzcuNWgtOTZWODk0ek0wIDYyN2E0OCA0OCAwIDAgMCA0OCA0OGg3Ny41di05NS45SDQ4QTQ4IDQ4IDAgMCAwIDAgNjI3ek04OTQgNTc5LjFoLTc3LjVWNjc1SDg5NGE0OCA0OCAwIDAgMCAwLTk1Ljl6TTAgNDcxYTQ4IDQ4IDAgMCAwIDQ4IDQ4aDc3LjV2LTk2SDQ4YTQ4IDQ4IDAgMCAwLTQ4IDQ4ek04OTQgNDIzaC03Ny41djk2SDg5NGE0OCA0OCAwIDAgMCAwLTk2ek0wIDMxNWE0OCA0OCAwIDAgMCA0OCA0OGg3Ny41di05Nkg0OGE0OCA0OCAwIDAgMC00OCA0OHpNODk0IDI2N2gtNzcuNXY5NS45SDg5NGE0OCA0OCAwIDAgMCAwLTk1Ljl6TTE3MS42IDcyMC40YTUwIDUwIDAgMCAwIDUwIDUwaDQ5OC44YTUwIDUwIDAgMCAwIDUwLTUwVjIyMS42YTUwIDUwIDAgMCAwLTUwLTUwSDIyMS42YTUwIDUwIDAgMCAwLTUwIDUwdjQ5OC44eiIvPjwvc3ZnPg==" alt="x86, x64, arm, arm64" title="Supported CPU Architectures: x86, x64, arm, arm64">&thinsp;<a href="#%EF%B8%8F-donate" title="Donate"><img src="https://img.shields.io/badge/-donate-yellow.svg?style=flat&logo=paypal&logoColor=white" alt="donate"></a>&thinsp;<a href="https://github.com/rikyoz/bit7z/wiki" title="Project Documentation"><img src="https://img.shields.io/badge/-docs-green.svg?style=flat&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIGRhdGEtbmFtZT0iTGF5ZXIgMSIgdmlld0JveD0iMCAwIDEwNS4zIDEyMi45Ij48cGF0aCBmaWxsLXJ1bGU9ImV2ZW5vZGQiIGZpbGw9IiNmZmYiIGQ9Ik0xNy41IDBIMTAydjk0LjJjLS4xIDIuNy0zLjUgMi43LTcuMiAyLjZIMTYuM2E5LjIgOS4yIDAgMCAwIDAgMTguNEg5OHYtOS44aDcuMlYxMThhNC4yIDQuMiAwIDAgMS00LjEgNC4xSDE2LjZDNy41IDEyNS41IDAgMTE4IDAgMTA4LjhWMTcuNUExNy42IDE3LjYgMCAwIDEgMTcuNSAwWm0tMS4zIDEwOGg3NS4yYTEuNCAxLjQgMCAwIDEgMS40IDEuM3YuOGExLjQgMS40IDAgMCAxLTEuNCAxLjRIMTYuMmExLjQgMS40IDAgMCAxLTEuMy0xLjR2LS44YTEuNCAxLjQgMCAwIDEgMS4zLTEuNFptMC03LjJoNzUuMmExLjQgMS40IDAgMCAxIDEuNCAxLjR2LjhhMS40IDEuNCAwIDAgMS0xLjQgMS40SDE2LjJBMS40IDEuNCAwIDAgMSAxNSAxMDN2LS44YTEuNCAxLjQgMCAwIDEgMS4zLTEuNFoiLz48L3N2Zz4=" alt="docs"></a>&thinsp;<a href="https://ci.appveyor.com/project/rikyoz/bit7z" title="AppVeyor CI Build Status"><img src="https://img.shields.io/appveyor/ci/rikyoz/bit7z.svg?style=flat&logo=appveyor&logoColor=white&label=" alt="Build status"></a>
  <br>
  <img src="https://img.shields.io/badge/MSVC%202015+-flag.svg?color=555555&style=flat&logo=visual%20studio&logoColor=white" alt="MSVC 2015+" title="Supported Windows Compiler: MSVC 2015 or later">&thinsp;<img src="https://img.shields.io/badge/MinGW%206.4+%20-flag.svg?color=555555&style=flat&logo=data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHZpZXdCb3g9IjAgMCAxMzIgMTMyIj48cGF0aCBmaWxsPSIjY2NjIiBkPSJtMCA1MiAyMy0yNCAyNCAyNC0yNCAyM3ptMCA1OCAyMy0yNCAyNCAyNC0yNCAyM3oiLz48cGF0aCBmaWxsPSIjN2Q3ZDdkIiBkPSJtMjkgODEgMjMtMjQgMjQgMjQtMjQgMjN6bS0xLTU4TDUyIDBsMjMgMjMtMjMgMjR6Ii8+PHBhdGggZmlsbD0iI2NjYyIgZD0ibTU3IDUyIDI0LTIzIDIzIDIzLTIzIDI0eiIvPjxwYXRoIGZpbGw9IiM3ZDdkN2QiIGQ9Im04NiAyMyAyNC0yMyAyMyAyMy0yMyAyNHoiLz48cGF0aCBmaWxsPSIjY2NjIiBkPSJtNTcgMTA5IDI0LTIzIDIzIDIzLTIzIDIzeiIvPjxwYXRoIGZpbGw9IiM3ZDdkN2QiIGQ9Im04NiA4MSAyMy0yNCAyMyAyNC0yMyAyM3oiLz48L3N2Zz4=&logoColor=white">&thinsp;<img src="https://img.shields.io/badge/GCC%204.9+-flag.svg?color=555555&style=flat&logo=gnu&logoColor=white" alt="GCC 4.9+" title="Supported Unix Compiler: GCC 4.9 or later">&thinsp;<img src="https://img.shields.io/badge/Clang%203.6+-flag.svg?color=555555&style=flat&logo=llvm&logoColor=white" alt="Clang 3.6+" title="Supported Unix Compiler: Clang 3.5 or later">&thinsp;<a href="https://www.codefactor.io/repository/github/rikyoz/bit7z" alt="Codefactor Grade"><img alt="CodeFactor Grade" src="https://img.shields.io/codefactor/grade/github/rikyoz/bit7z?label=Code%20Quality&logo=codefactor&logoColor=white"></a>&thinsp;<a href="https://github.com/rikyoz/bit7z/blob/master/LICENSE" title="Project License: MPLv2"><img src="https://img.shields.io/badge/-MPL--2.0-lightgrey.svg?style=flat&logo=mozilla" alt="License"></a>
</div>

## ⚡️ Introduction

**bit7z** is a _cross-platform_ C++ static library that allows the _compression/extraction of archive files_ through a _clean_ and _simple_ wrapper interface to the dynamic libraries from the [7-zip](https://www.7-zip.org/ "7-zip Project Homepage") project.<br/>
It supports compression and extraction to and from the filesystem or the memory, reading archives metadata, updating existing ones, creating multi-volume archives, operation progress callbacks, and many other functionalities.

## 🎯 Supported Features

+ **Compression** using the following archive formats: **7z**, XZ, **BZIP2**, **GZIP**, TAR, **ZIP**, and WIM.
+ **Extraction** of many archive formats: **7z**, AR, ARJ, **BZIP2**, CAB, CHM, CPIO, CramFS, DEB, DMG, EXT, FAT, GPT, **GZIP**, HFS, HXS, IHEX, ISO, LZH, LZMA, MBR, MSI, NSIS, NTFS, QCOW2, **RAR**, **RAR5**, RPM, SquashFS, TAR, UDF, UEFI, VDI, VHD, VMDK, WIM, XAR, XZ, Z, and **ZIP**.
+ **Reading metadata** of archives and their content.
+ **Testing** archives for errors.
+ **Updating** existing file archives with new files.
+ **Renaming**, **updating**, or **deleting** old items in existing file archives.
+ **Compression and extraction _to and from_ memory** and **C++ standard streams**.
+ Compression using **custom path aliases** for the items in the output archives.
+ **Selective extraction** of only specified files/folders **using wildcards** and **regular expressions**.
+ Creation of **encrypted archives** (strong AES-256 encryption; only for 7z and ZIP formats).
+ **Archive header encryption** (only for 7z format).
+ Possibility to choose the **compression level** (if supported by the archive format), the **compression method** ([supported methods](https://github.com/rikyoz/bit7z/wiki/Advanced-Usage#compression-methods "Wiki page on bit7z's supported compression methods")), the **dictionary size**, and the **word size**.
+ **Automatic input archive format detection**.
+ **Solid archives** (only for 7z).
+ **Multi-volume archives**.
+ **Operation callbacks** for obtaining real-time information about ongoing operations.
+ **Canceling** or **pausing** the current operation.

### Notes

The presence or not of some of the above features depends on the particular shared library used along with bit7z.<br/>
For example, 7z.dll should support all these features, 7za.dll should work only with the 7z file format, and 7zxa.dll can only extract 7z files. For more information about the 7-zip DLLs, please check this [wiki page](https://github.com/rikyoz/bit7z/wiki/7z-DLLs).

In the end, some other features (e.g., _automatic format detection_ and _selective extraction using regular expressions_) are disabled by default, and macro definitions must be used during compilation to have them available ([wiki](https://github.com/rikyoz/bit7z/wiki/Building-the-library)).

## 🔥 Getting Started (Library Usage)

Below are a few examples that show how to use some of the main features of bit7z.

### 📂 Extracting Files from an Archive

```cpp
#include <bit7z/bitfileextractor.hpp>

try { // bit7z classes can throw BitException objects
    using namespace bit7z;

    Bit7zLibrary lib{ "7za.dll" };
    BitFileExtractor extractor{ lib, BitFormat::SevenZip };

    // Extracting a simple archive
    extractor.extract( "path/to/archive.7z", "out/dir/" );

    // Extracting a specific file inside an archive
    extractor.extractMatching( "path/to/archive.7z", "file.pdf", "out/dir/" );

    // Extracting the first file of an archive to a buffer
    std::vector< byte_t > buffer;
    extractor.extract( "path/to/archive.7z", buffer );

    // Extracting an encrypted archive
    extractor.setPassword( "password" );
    extractor.extract( "path/to/another/archive.7z", "out/dir/" );
} catch ( const bit7z::BitException& ex ) { /* Do something with ex.what()...*/ }
```

Alternatively, if you only need to work on a single archive:

```cpp
#include <bit7z/bitarchivereader.hpp>

try { // bit7z classes can throw BitException objects
    using namespace bit7z;

    Bit7zLibrary lib{ "7z.dll" };

    // Opening the archive
    BitArchiveReader archive{ lib, "path/to/archive.gz", BitFormat::GZip };

    // Testing the archive
    archive.test();

    // Extracting the archive
    archive.extractTo( "out/dir/" );
} catch ( const bit7z::BitException& ex ) { /* Do something with ex.what()...*/ }
```

### 💼 Compressing Files into an Archive

```cpp
#include <bit7z/bitfilecompressor.hpp>

try { // bit7z classes can throw BitException objects
    using namespace bit7z;

    Bit7zLibrary lib{ "7z.dll" };
    BitFileCompressor compressor{ lib, BitFormat::Zip };

    std::vector< std::string > files = { "path/to/file1.jpg", "path/to/file2.pdf" };

    // Creating a simple zip archive
    compressor.compress( files, "output_archive.zip" );

    // Creating a zip archive with a custom directory structure
    std::map< std::string, std::string > files_map = {
        { "path/to/file1.jpg", "alias/path/file1.jpg" },
        { "path/to/file2.pdf", "alias/path/file2.pdf" }
    };
    compressor.compress( files_map, "output_archive2.zip" );

    // Compressing a directory
    compressor.compressDirectory( "dir/path/", "dir_archive.zip" );

    // Creating an encrypted zip archive of two files
    compressor.setPassword( "password" );
    compressor.compressFiles( files, "protected_archive.zip" );

    // Updating an existing zip archive
    compressor.setUpdateMode( UpdateMode::Append );
    compressor.compressFiles( files, "existing_archive.zip" );

    // Compressing a single file into a buffer
    std::vector< bit7z::byte_t > buffer;
    BitFileCompressor compressor2{ lib, BitFormat::BZip2 };
    compressor2.compressFile( files[0], buffer );
} catch ( const bit7z::BitException& ex ) { /* Do something with ex.what()...*/ }
```

Alternatively, if you only need to work on a single archive:

```cpp
#include <bit7z/bitarchivewriter.hpp>

try { // bit7z classes can throw BitException objects
    using namespace bit7z;

    Bit7zLibrary lib{ "7z.dll" };
    BitArchiveWriter archive{ lib, BitFormat::SevenZip };

    // Adding the items to be compressed (no compression is performed here)
    archive.addFile( "path/to/file.txt" );
    archive.addDirectory( "path/to/dir/" );

    // Compressing the added items to the output archive
    archive.compressTo( "output.7z" );
} catch ( const bit7z::BitException& ex ) { /* Do something with ex.what()...*/ }
```

### 📑 Reading Archive Metadata

```cpp
#include <bit7z/bitarchivereader.hpp>

try { // bit7z classes can throw BitException objects
    using namespace bit7z;

    Bit7zLibrary lib{ "7za.dll" };
    BitArchiveReader arc{ lib, "archive.7z", BitFormat::SevenZip };

    // Printing archive metadata
    std::cout << "Archive properties\n";
    std::cout << "  Items count: "   << arc.itemsCount() << '\n';
    std::cout << "  Folders count: " << arc.foldersCount() << '\n';
    std::cout << "  Files count: "   << arc.filesCount() << '\n';
    std::cout << "  Size: "          << arc.size() <<'\n';
    std::cout << "  Packed size: "   << arc.packSize() << "\n\n";

    // Printing the metadata of the archived items
    std::cout << "Archived items";
    for ( const auto& item : arc ) {
        std::cout << '\n';
        std::cout << "  Item index: "    << item.index() << '\n';
        std::cout << "    Name: "        << item.name() << '\n';
        std::cout << "    Extension: "   << item.extension() << '\n';
        std::cout << "    Path: "        << item.path() << '\n';
        std::cout << "    IsDir: "       << item.isDir() << '\n';
        std::cout << "    Size: "        << item.size() << '\n';
        std::cout << "    Packed size: " << item.packSize() << '\n';
        std::cout << "    CRC: " << std::hex << item.crc() << std::dec << '\n';
    }
    std::cout.flush();
} catch ( const bit7z::BitException& ex ) { /* Do something with ex.what()...*/ }
```

A complete _**API reference**_ is available in the [wiki](https://github.com/rikyoz/bit7z/wiki/) section.

### 🚀 Upgrading from bit7z v3 to v4

The newest bit7z v4 introduced some significant breaking changes to the library's API.

<details>
  <summary>Expand for more details!</summary>

+ By default, the project now follows the [UTF-8 Everywhere Manifesto](http://utf8everywhere.org/):
  + The default string type is `std::string` (instead of `std::wstring`), so users can use the library in cross-platform projects more easily (v4 introduced Linux/macOS support too).
  + Input `std::string`s will be considered as UTF-8 encoded.
  + You can still achieve the old behavior on Windows using the `-DBIT7Z_USE_NATIVE_STRING` CMake option.
+ The old `BitExtractor` class is now called `BitFileExtractor`.
  + Now `BitExtractor` is just the name of a template class for all the extraction classes.
+ The old `BitCompressor` class is now called `BitFileCompressor`.
  + Now `BitCompressor` is just the name of a template class for all the compression classes.
+ The `ProgressCallback` now must return a `bool` value indicating whether the current operation can continue (`true`) or not (`false`).
+ The project structure changed:
  + Public API headers moved from `include/` to the `include/bit7z/` folder, so `#include` directives now need to prepend `bit7z/` to the included header name (e.g., `#include <bit7z/bitfileextractor.hpp>`).
    + Even though it is a bit verbose, it is a typical structure for C and C++ libraries, and it makes explicit which third-party library a header file belongs to.
  + By default, the output folder of bit7z is now `lib/<architecture>/`; if the CMake generator is multi-config (e.g., Visual Studio generators), the default output folder is `lib/<architecture>/<build type>/`.
    + Optionally, you can force using the "Visual Studio style" output path by enabling the `BIT7Z_VS_LIBNAME_OUTDIR_STYLE` CMake option.
  + Third-party dependencies are no longer handled using git submodules but are automatically downloaded using [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake) when configuring/using the library via CMake.

</details>

## 💾 Download

<div align="center">
<a href="https://github.com/rikyoz/bit7z/releases"><img alt="GitHub Latest Release" src="https://img.shields.io/github/v/release/rikyoz/bit7z?sort=semver&label=Latest%20Release&style=social" height='36' style='border:0;height:36px;'/></a>
<br/>
<a href="https://github.com/rikyoz/bit7z/releases">
<img alt="GitHub All Releases" src="https://img.shields.io/github/downloads/rikyoz/bit7z/total.svg?style=popout&label=Total%20Downloads&logo=icloud&logoColor=white"/></a>
</div>

Each released package contains:

+ A _pre-compiled version_ of bit7z (both in _debug_ and _release_ mode);
+ The _public API headers_ needed to use the library in your program;

Packages are available for both _x86_ and _x64_ architectures.

You can also clone/download this repository and build the library yourself (please, read the [wiki](https://github.com/rikyoz/bit7z/wiki/Building-the-library)).

## 🧰 Requirements

+ **Operating System:** Windows, Linux, macOS, Android[^1].
+ **Architecture:** x86, x86_64, arm, arm64.
+ **Language Standard:** C++11 (for using the library), C++14 (for building the library).
+ **Compiler:** MSVC 2015 or later[^2], MinGW v6.4 or later, GCC v4.9 or later, Clang 3.6 or later.
+ **Shared Library:** a 7-zip `.dll` library on Windows, a 7-zip/p7zip `.so` library on Unix[^3].

[^1]: On Windows, you should link your program _also_ with _oleaut32_ (e.g., `-lbit7z -loleaut32`).<br/> On Linux and macOS, you should link your program _also_ with _dl_ (e.g., `-lbit7z -ldl`).<br/> If you are using the library via CMake, these dependencies will be linked automatically to your project.

[^2]: MSVC 2010 was supported until v2.x, MSVC 2012/2013 until v3.x.

[^3]: bit7z doesn't ship with the 7-zip shared libraries. You can build them from the source code available at [7-zip.org](http://www.7-zip.org/).

## ⚙️ Building and Using bit7z

For building the library:

```bash
cd <bit7z folder>
mkdir build && cd build
cmake ../ -DCMAKE_BUILD_TYPE=Release
cmake --build . -j --config Release
```

A more detailed guide on how to build this library is available [here](https://github.com/rikyoz/bit7z/wiki/Building-the-library).

You can also directly integrate the library into your project via CMake:

+ Download bit7z and copy it into a sub-directory of your project (e.g., `third_party`), or add it as a git submodule of your repository.
+ Then, use the command `add_subdirectory()` in your `CMakeLists.txt` to include bit7z.
+ Finally, link the `bit7z` library using the `target_link_libraries()` command.

For example:

```cmake
add_subdirectory( ${CMAKE_SOURCE_DIR}/third_party/bit7z )
target_link_libraries( ${YOUR_TARGET} PRIVATE bit7z )
```

The library is highly customizable: for a detailed list of the available build options, please refer to the [wiki](https://github.com/rikyoz/bit7z/wiki/Building-the-library).

### 📑 7-zip Version

While configuring bit7z via CMake, it automatically downloads the latest version of 7-zip currently supported by the library.

Optionally, you can specify a different version of 7-zip via the CMake option `BIT7Z_7ZIP_VERSION` (e.g., `-DBIT7Z_7ZIP_VERSION="22.01"`).

Alternatively, you can specify a custom path containing the 7-zip source code via the option `BIT7Z_CUSTOM_7ZIP_PATH`.

Please note that, in general, it is best to use the same version of 7-zip of the shared libraries that you will use at runtime.

#### Using 7-zip v23.01 on Linux and macOS

<details>
  <summary>Expand for more details!</summary>

_On Linux and macOS_, 7-zip v23.01 introduced breaking changes to the IUnknown interface. If you build bit7z for such a version of 7-zip (the default), it will not support using the shared libraries from previous versions of 7-zip (or from p7zip). Conversely, bit7z made for earlier versions of 7-zip or for p7zip is incompatible with the shared libraries from 7-zip v23.01 and later.

You can build the shared libraries of 7-zip v23.01 in a backward-compatible mode by defining the macro `Z7_USE_VIRTUAL_DESTRUCTOR_IN_IUNKNOWN`. If this is your case, you can build bit7z for v23.01 using the option `BIT7Z_USE_LEGACY_IUNKNOWN` (in this case, bit7z will be compatible also with previous versions of 7-zip/p7zip).

</details>

### 🌐 String Encoding

By default, bit7z follows the [UTF-8 Everywhere Manifesto](http://utf8everywhere.org/) to simplify the use of the library within cross-platform projects.
In short, this means that:

+ The default path string type is `std::string`.
+ Input `std::string`s are considered as UTF-8 encoded; output `std::string`s are UTF-8 encoded.

<details>
  <summary>Expand for more details and for other string encoding options!</summary>

On POSIX systems, `std::string`s are usually already UTF-8 encoded, and no configuration is needed.

The situation is a bit more complex on Windows since, by default, Windows treats `std::string`s as encoded using the system code page, which may not necessarily be UTF-8, like, for example, [Windows-1252](https://en.wikipedia.org/wiki/Windows-1252).

If your program deals exclusively with ASCII-only strings, you should be fine with the default bit7z settings (as ASCII characters are also UTF-8).

However, if you need to handle non-ASCII/Unicode characters, as it is likely, you have the following options:

+ Enforcing using the UTF-8 code page for your whole application, as explained by Microsoft [here](https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page):
  + _**Recommended**_, but supported only since Windows 10 1903 and later.
+ Manually ensuring the encoding of the `std::string`s passed to bit7z:
  + You can use some string encoding library or C++11's UTF-8 string literals for input strings.
  + User-input strings (e.g., the password of an archive) can be handled as explained [here](https://nullprogram.com/blog/2020/05/04/); in short: read the input as an UTF-16 wide string (e.g., via `ReadConsoleW`), and convert it to UTF-8 (bit7z provides a utility function for this, `bit7z::to_tstring`).
  + You can correctly print the UTF-8 output strings from bit7z (e.g., the path/name metadata of a file in an archive) to the console by calling [`SetConsoleOutputCP(CP_UTF8)`](https://learn.microsoft.com/en-us/windows/console/setconsoleoutputcp) before.
+ Configuring bit7z to use UTF-16 encoded wide strings (i.e., `std::wstring`) by enabling the `BIT7Z_USE_NATIVE_STRING` option via CMake.
  + If your program is Windows-only, or you already use wide strings on Windows, this might be the best choice since it will avoid any internal string conversions (7-zip always uses wide strings).
  + This option makes developing cross-platform applications slightly inconvenient since you'll still have to use `std::string` on POSIX systems.
  + The library provides a type alias `bit7z::tstring` and a macro function `BIT7Z_STRING` for defining wide string variables and literals on Windows and narrow ones on other platforms.
  + You must programmatically set the standard input and output encoding to UTF-16 to correctly read and print Unicode characters:

    ```cpp
    #include <fcntl.h> //for _O_U16TEXT
    #include <io.h>  //for _setmode

    _setmode(_fileno(stdout), _O_U16TEXT); // setting the stdout encoding to UTF16
    _setmode(_fileno(stdin), _O_U16TEXT); // setting the stdin encoding to UTF16
    ```

+ Configuring bit7z to use the system code page encoding for `std::string` by enabling the `BIT7Z_USE_SYSTEM_CODEPAGE` option via CMake.
  + _Not recommended_: using this option, your program will be limited in the set of characters it can pass to and read from bit7z.

</details>

## ☕️ Donate

If you have found this project helpful, please consider supporting me with a small donation so that I can keep improving it!
Thank you! :pray:

<div align="center">
<a href='https://github.com/sponsors/rikyoz' target='_blank' title="Support the Maintainer via GitHub Sponsors"><img height='24' style='border:0px;height:24px;' src='https://img.shields.io/badge/-Sponsor%20the%20Maintainer-fafbfc?logo=GitHub%20Sponsors' border='0' alt='Sponsor me on GitHub' /></a>&thinsp;<a href='https://ko-fi.com/G2G1LS1P' target='_blank' title="Support this project via Ko-Fi"><img height='24' style='border:0px;height:24px;' src='https://img.shields.io/badge/-Buy%20Me%20a%20Coffee-red?logo=ko-fi&logoColor=white' border='0' alt='Buy Me a Coffee at ko-fi.com' /></a>&thinsp;<a href="https://www.paypal.com/cgi-bin/webscr?cmd=_s-xclick&hosted_button_id=NTZF5G7LRXDRC" title="Support this project via PayPal"><img src="https://img.shields.io/badge/-Donate%20on%20PayPal-yellow.svg?logo=paypal&logoColor=white" alt="Donations" height='24' style='border:0px;height:24px;'></a>
</div>

## 📜 License

This project is licensed under the terms of the [Mozilla Public License v2.0](https://www.mozilla.org/en-US/MPL/2.0/).<br/>
For more details, please check:

+ The [LICENSE](./LICENSE) file.
+ [Mozilla's MPL-2.0 FAQ](https://www.mozilla.org/en-US/MPL/2.0/FAQ/)

Older versions (v3.x and earlier) of bit7z were released under the [GNU General Public License v2](https://www.gnu.org/licenses/old-licenses/gpl-2.0.en.html).

<br/>
<div align="center">
Copyright &copy; 2014 - 2023 Riccardo Ostani (<a href="https://github.com/rikyoz">@rikyoz</a>)
</div>
<br/>
