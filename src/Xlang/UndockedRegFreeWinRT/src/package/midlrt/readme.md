## The Windows Runtime (WinRT) API Generation tool

The Microsoft.Windows.MidlRT package provides targets for running the midlrt.exe tool that ships in the Windows SDK. This enables you to build WinRT APIs that generate a .winmd file using either the MIDL2.0 or MIDL3.0 syntax.

- Nuget Package: https://www.nuget.org/packages/Microsoft.Windows.MidlRT/

For more information on MIDL3.0 and all the benefits it brings, including improved compilation times, see the official documentation: https://docs.microsoft.com/en-us/uwp/midl-3/

## Migrating from MIDL2.0 to MIDL3

See the official documentation for how to migrate from MIDL2.0 syntax to MIDL3.0: https://docs.microsoft.com/en-us/uwp/midl-3/from-midlrt

## Usage

The MidlRT package is **production only**, which means it enables you to generate your own .winmd file. If you are producing a .winmd, it's likely that you'll need to consume other WinRT API's as well, which can be done through the consumption technology of your choice:

- Microsoft.Windows.CppWinRT: https://www.nuget.org/packages/Microsoft.Windows.CppWinRT/
- Microsoft.Windows.AbiWinRT: https://www.nuget.org/packages/Microsoft.Windows.AbiWinRT/
  - **Note: Microsoft.Windows.AbiWinRT is only intended for legacy code bases,**
       **and doesn't provide support for the latest C++ language features that Microsoft.Windows.CppWinRT does.**

If you are using `Microsoft.Windows.CppWinRT` there is no need to reference this Nuget directly.

|  Property |  Description |
|-----------|---------------|
| `ModernMidlRT` | Uses MIDL3.0. Defaults to `true`. |
