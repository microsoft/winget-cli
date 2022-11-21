## What is xlang?

The xlang project is the hub for the constellation of tools that enable development of Windows applications across a variety of programming languages. This includes tooling to process metadata, and tooling to access APIs from various programming languages including C#, C++, Rust, and Python.

Code in this repository provides infrastructure and tooling related to basic metadata handling & app usage. Tooling for each language is (mostly) maintained in separate repositories. Issue and work tracking for each tool or language is tracked in the corresponding repository as well. Issues that span languages or relate to basic metadata issues are tracked here.

### Metadata & Infrastructure

* [Undocked RegFree WinRT](https://github.com/microsoft/xlang/tree/master/src/UndockedRegFreeWinRT) - Provides access to user-defined WinRT types for versions of Windows prior to Windows 10 May 2019 update.

* [WinRT test component](https://github.com/microsoft/TestWinRT) - Provides a compact but thorough test suite for validating projection support for consuming and implementing Windows Runtime-style APIs.

* [Win32 & COM Metadata](https://github.com/microsoft/win32metadata) - Tooling that analyzes Windows C/C++ headers, and provides metadata describing the Win32 & COM API surface of Windows. This metadata is used to provide access to Win32 from other programming languages such as C# & Rust.

### C++

* [C++/WinRT](https://github.com/microsoft/cppwinrt) - A tool that provides C++ support for calling Windows Runtime APIs that has a modern C++ feel.

* [C++ winmd parser](https://github.com/microsoft/winmd) - A C++ parser library for efficiently parsing WinRT metadata.

* [WinRT ABI (C with classes) Header Generation Tool ](https://github.com/microsoft/xlang/tree/master/src/tool/abi) - Tool that generates the C++ representation of the raw "COM-style" interfaces that make WinRT work.

* [C++/Win32](https://github.com/microsoft/cppwin32) - Tooling that leverages the Win32 metadata to provide a modern C++ development experience for Windows APIs.

* [Windows Implementation Libraries](https://github.com/microsoft/wil) - A set of helper libraries that provide RAII and other helpful abstractions for many existing SDK headers & types.

### C#

* [C#/WinRT](https://github.com/microsoft/cswinrt) - Interop support for WinRT APIs and .NET 5.

* [C#/Win32](https://github.com/microsoft/cswin32) - A metadata-based projection of Win32 and COM APIs.

### Rust

* [Rust for Windows](https://github.com/microsoft/windows-rs) - Tooling that creates WinRT, Win32 and COM APIs for the Rust programming language.

### Python

* [Python/WinRT](https://github.com/pywinrt/pywinrt) - Support for calling most (non-XAML) APIs from the Python programming language.

### Cross-platform WinRT

When this repository first created, we used this space to explore what it would take to bring Windows Runtime APIs to other platforms. While that investment has been put on hold for the time being, some of the project code and samples in this repo remain but may be in disrepair.

## Related projects

* [Project Reunion](https://github.com/microsoft/projectreunion)

* [.NET 5](https://github.com/dotnet)


## License

Code licensed under the [MIT License](LICENSE).

## Contributing

This project welcomes contributions and suggestions.  Most contributions require you to agree to a
Contributor License Agreement (CLA) declaring that you have the right to, and actually do, grant us
the rights to use your contribution. For details, visit https://cla.microsoft.com.

When you submit a pull request, a CLA-bot will automatically determine whether you need to provide
a CLA and decorate the PR appropriately (e.g., label, comment). Simply follow the instructions
provided by the bot. You will only need to do this once across all repos using our CLA.

This project has adopted the [Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/).
For more information see the [Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or
contact [opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.
