## The ABI/WinRT language projection

The Application Binary Interface (ABI) projection generates header files for consuming WinRT APIs that are similar to headers generated from COM .idl files.

**Note: This tool is for legacy codebases and not recommended for new development. C++ developers should instead use https://github.com/microsoft/cppwinrt, which supports standard C++ and the latest C++ language features.**

- Nuget Package: https://www.nuget.org/packages/Microsoft.Windows.AbiWinRT/

## Usage

Similar to the `Microsoft.Windows.CppWinRT`nuget package, you can add a reference to `Microsoft.Windows.AbiWinRT` and the tool will generate the projection headers for your project to consume.

|  Property |  Description |
|-----------|---------------|
| `AbiWinRTAddAbiNamespacePrefix` | Adds the `ABI::` prefix to all generated headers. Defaults to `true`. |
| `AbiWinRTEnsureSDKHeaderCompat` | Passes the `-lowercase-include-guard` switch to `abi.exe` to enable compat with SDK headers. Defaults to `false`. |
